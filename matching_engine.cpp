#include "types.hpp"
#include "matching_engine.hpp"
#include "security_master.hpp"


an::MatchingEngine::MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb, 
                                   bool bookkeep)
             : seq_(1),
             epoch_{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() },
             exchange_(exchange), secdb_(secdb) {
    book_.reserve(secdb.securities().size() *2);
    for (const auto& sec : secdb.securities() ) {
        TickTable* ttPtr = secdb.tickTable(sec.ladder_id);
        if (ttPtr == nullptr) {
            std::cout << sec.symbol << " skipping invalid tick_ladder_id " << sec.ladder_id << std::endl;
            continue;
        }
        book_.emplace_back(sec.symbol,epoch_, *ttPtr, bookkeep, sec.closing_price);
        if (!sec.has_died) {
            book_.back().open();
        }
    }
}

an::MatchingEngine::~MatchingEngine() {
    assert(book_.empty() && "MatchingEngine::~MatchingEngine should be closed");
}

void an::MatchingEngine::close() {
    for (auto& book : book_) {
        book.close();
    }
    book_.clear();
}

std::string an::MatchingEngine::to_string() const {
    std::ostringstream os;
    for (const auto& book: book_) {
        os << book.to_string(true);
        os << std::endl;
    }
    return os.str();
}


void an::MatchingEngine::applyOrder(std::unique_ptr<Execution> exe) {
    SideRecord rec;
    exe->pack(rec);
    rec.time = std::chrono::steady_clock::now();
    rec.seq = seq_++;
    rec.visible = true;
    const symbol_t& symbol = exe->symbol();
    Book* book = findBook(symbol);
    if (book != nullptr) {
        book->executeOrder(rec, std::move(exe));
    } else {
        sendResponse(exe.get(), an::REJECT, "symbol not found");
    }
}

void an::MatchingEngine::applyOrder(std::unique_ptr<CancelOrder> o) {
    const symbol_t& symbol = o->symbol();
    Book* book = findBook(symbol);
    if (book != nullptr) {
        order_id_t id = o->orderId();
        book->cancelActiveOrder(id, std::move(o));
    } else {
        sendResponse(o.get(), an::REJECT, "symbol not found");
    }
}

void an::MatchingEngine::applyOrder(std::unique_ptr<AmendOrder> o) {
    const symbol_t& symbol = o->symbol();
    Book* book = findBook(symbol);
    if (book != nullptr) {
        order_id_t id = o->orderId();
        book->amendActiveOrder(id,std::move(o));
    } else {
        sendResponse(o.get(), an::REJECT, "symbol not found");
    }
}

an::Book* an::MatchingEngine::findBook(const symbol_t& symbol) {
    Book* bookPtr = nullptr;
    auto symbol_idx = secdb_.find(symbol);
    if (symbol_idx != SecurityDatabase::npos) { // FOUND
        // std::cout << "lookuoBook=" << symbol << " idx=" << symbol_idx << std::endl;
        bookPtr = &book_[symbol_idx];
    }
    return bookPtr;
}

void an::MatchingEngine::sendTradeReport(Order* o, direction_t d, shares_t s, price_t p) {
    std::unique_ptr<TradeReport> tradeRep1(new TradeReport(o, d, s, p));
    std::cout << *tradeRep1 << std::endl;
}

void an::MatchingEngine::sendResponse(Message* o, response_t r, text_t t) {
    std::unique_ptr<Response> response1(new Response(o, r, t));
    std::cout << *response1 << std::endl;
}


// ********************************* BOOK *****************************************
std::string an::Side::to_string(bool verbose, bool one_list) const {
    std::ostringstream os;
    if (verbose) {
        Side::container_type data;
        for (const auto &rec : q_) {
            data.push_back(rec);
        }
        std::sort( data.begin(), data.end(), std::not2(Side::value_compare(one_list)) );
        for (const auto& rec: data) {
            os << boost::format("%1$6d %2$4d %3$1c ") % rec.id % rec.seq % (rec.visible ? '*' : '.');
            if (direction_ == BUY) {
                os  << boost::format("%1$4s %2$18s %3$6d ")
                        % an::to_string_book(rec.direction) % sinceToString(rec.time,epoch_) % rec.shares
                    << boost::format("%1$8s") % floatDecimalPlaces(rec.price,MAX_PRICE_PRECISION) ;
            } else {
                os  << boost::format("%1$30c ") % ' '
                    << boost::format("%1$8s ") % floatDecimalPlaces(rec.price,MAX_PRICE_PRECISION)
                    << boost::format("%1$6d %2$18s %3$4s") 
                        % rec.shares % sinceToString(rec.time, epoch_) % an::to_string_book(rec.direction) ;
            }
            os << std::endl;
        }
    } else {
        Q q(q_);
        while (!q.empty()) {
            os << q.top().to_string(epoch_) << std::endl;
            q.pop();
        }
    }

    return os.str();
}


bool an::Book::marketableSide(Side& side, SideRecord& newRec, Execution* newExe) {
    bool marketable = false; // Can the new order be satisfied without adding it to the book
    // Is there something to compare against and our new order isn't complete
    while (!side.empty() && (newRec.shares!=0)) {
        marketable = false;
        SideRecord top = side.top();
        assert(top.visible && "top should be visible");
        assert(newRec.visible && "new record should be visible");
        if ( ((newRec.direction==an::BUY)  && compare3decimalplaces(newRec.price,top.price) >= 0) ||
             ((newRec.direction==an::SELL) && compare3decimalplaces(newRec.price,top.price) <= 0) ) {
            shares_t shares = std::min(newRec.shares,top.shares);
            price_t price = top.price;

            // Find active order (from book)
            Execution* exeOld = findActiveOrder(top.id);
            assert(exeOld != nullptr && "marketable active order null");

            sendTradeReport(exeOld, newRec.direction, shares, price);
            sendTradeReport(newExe, top.direction, shares, price);
            if (top.shares == shares) {
                sendResponse(exeOld, an::COMPLETE, "Top Filled");
                side.removeTop(); // Remove top
                removeActiveOrder(top.id);
            } else {
                side.amendSharesTop(-shares); // top.shares -= shares
            }
            if (newRec.shares == shares) {
                sendResponse(newExe, an::COMPLETE, "New Filled");
                if (newRec.on_book) {
                    price_t pr = newRec.price; // Save value
                    newRec.price = price;
                    side.addVolume(newRec);
                    newRec.price = pr; // restore
                }
                newRec.visible = false;
                marketable = true; // New order satistfied 
            } else {
                if (newRec.on_book) {
                    // Add volume for shares traded
                    shares_t shr = newRec.shares; // Save value
                    price_t pr = newRec.price; // Save value
                    
                    newRec.shares = shares; 
                    newRec.price = price;
                    side.addVolume(newRec);

                    newRec.shares = shr; // restore
                    newRec.price = pr; // restore
                }
            }
            newRec.shares -= shares; // Reduce number of shares needed by newRec
        } else {
            // Not marketable if order values not matching
            break;
        }
    }
    return marketable;
}

bool an::Book::marketable(SideRecord& newRec, Execution* newExe) {
    assert(newRec.visible && "Book::marketable new record should be visible");
    bool marketable = false;
    if (newRec.direction==an::BUY) {
        // Compare against sell
        marketable = marketableSide(sell_, newRec, newExe);
    } else {
        marketable = marketableSide(buy_, newRec, newExe);
    }
    std::cout << "Book::marketable" << " id=" << newExe->orderId() << " marketable=" << marketable << std::endl;
    return marketable;
}

void an::Book::closeBook() {
    open_ = false;
    for (auto it = active_order_.begin(); it != active_order_.end(); ) {
        sendCancel(it->second.order.get(), "closing down");
        it = active_order_.erase(it);
    }
    if (bookkeep_) {
        bookkeeper_.close();
        std::cout << symbol_ << " " << bookkeeper_.to_string() << std::endl;
    }
    assert(active_order_.empty());
}

// https://stackoverflow.com/questions/13112062/which-are-the-order-matching-algorithms-most-commonly-used-by-electronic-financi
std::string an::Book::to_string(bool verbose) const {
    std::ostringstream os;
    if (verbose) {
        os << boost::format("[%1$s]") % symbol_ << std::endl
           << "    Id Seq  V Side Time               Shares Price    Shares Time               Side" << std::endl
           << "------+----+-+----+------------------+------+--------+------+------------------+----" << std::endl ;
    } else {
        os << boost::format("[%1$s]") % symbol_ << std::endl;
    }
    os << sell_.to_string(verbose, true);
    os << buy_.to_string(verbose, true);
    os << std::endl;

    return os.str();
}

