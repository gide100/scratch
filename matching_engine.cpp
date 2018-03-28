#include "types.hpp"
#include "matching_engine.hpp"
#include "security_master.hpp"


an::MatchingEngine::MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb, 
                                   bool bookkeep)
             : seq_(1),
             epoch_{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() },
             exchange_(exchange), secdb_(secdb), book_(), stats_(), rejects_(0), open_(false) {
    book_.reserve(secdb.securities().size() *2);
    for (const auto& sec : secdb.securities() ) {
        TickTable* ttPtr = secdb.tickTable(sec.ladder_id);
        if (ttPtr == nullptr) {
            std::cout << sec.symbol << " skipping invalid tick_ladder_id " << sec.ladder_id << std::endl;
            continue;
        }
        book_.emplace_back(sec.symbol,epoch_, *ttPtr, bookkeep, sec.closing_price);
        if (!sec.has_died && (sec.exchange == exchange)) {
            book_.back().open();
        }
    }
    open_ = true;
}

an::MatchingEngine::~MatchingEngine() {
    assert(book_.empty() && "MatchingEngine::~MatchingEngine should be closed");
}

void an::MatchingEngine::close() {
    for (auto& book : book_) {
        book.close();
    }
    (void) stats();
    book_.clear();
    stats_.symbols = book_.size();
    open_ = false;
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
    if (exe->destination() != exchange_) {
        sendResponse(exe.get(), an::REJECT, "wrong destination");
        ++rejects_;
    } else {
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
            ++rejects_;
        }
    }
}

void an::MatchingEngine::applyOrder(std::unique_ptr<CancelOrder> o) {
    if (o->destination() != exchange_) {
        sendResponse(o.get(), an::REJECT, "wrong destination");
        ++rejects_;
    } else {
        const symbol_t& symbol = o->symbol();
        Book* book = findBook(symbol);
        if (book != nullptr) {
            order_id_t id = o->orderId();
            book->cancelActiveOrder(id, std::move(o));
        } else {
            sendResponse(o.get(), an::REJECT, "symbol not found");
            ++rejects_;
        }
    }
}

void an::MatchingEngine::applyOrder(std::unique_ptr<AmendOrder> o) {
    if (o->destination() != exchange_) {
        sendResponse(o.get(), an::REJECT, "wrong destination");
        ++rejects_;
    } else {
        const symbol_t& symbol = o->symbol();
        Book* book = findBook(symbol);
        if (book != nullptr) {
            order_id_t id = o->orderId();
            book->amendActiveOrder(id,std::move(o));
        } else {
            sendResponse(o.get(), an::REJECT, "symbol not found");
            ++rejects_;
        }
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

an::engine_stats_t an::MatchingEngine::stats() {
    if (open_) {
        stats_ = engine_stats_t();
        stats_.symbols = book_.size();
        stats_.rejects = rejects_;
        for (const auto& b : book_) {
            stats_.open_books    += b.isOpen();
            stats_.shares_traded += b.bookkeeper().stats().shares_traded;
            stats_.volume        += b.bookkeeper().stats().volume;
            stats_.trades        += b.bookkeeper().stats().trades;
            stats_.cancels       += b.bookkeeper().stats().cancels;
            stats_.amends        += b.bookkeeper().stats().amends;
            stats_.rejects       += b.bookkeeper().stats().rejects;
            stats_.buy.trades    += b.bookkeeper().stats().buy.trades;
            stats_.buy.shares    += b.bookkeeper().stats().buy.shares;
            stats_.buy.value     += b.bookkeeper().stats().buy.value;
            stats_.buy.volume    += b.bookkeeper().stats().buy.volume;
            stats_.sell.trades   += b.bookkeeper().stats().sell.trades;
            stats_.sell.shares   += b.bookkeeper().stats().sell.shares;
            stats_.sell.value    += b.bookkeeper().stats().sell.value;
            stats_.sell.volume   += b.bookkeeper().stats().sell.volume;
            stats_.active_trades += b.bookkeeper().stats().buy.trades + b.bookkeeper().stats().sell.trades;
        }
    }
    return stats_;
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
                // Add volume for shares traded
                shares_t shr = top.shares; // Save value
                price_t pr = top.price; // Save value
                
                top.shares = shares; 
                top.price = price;
                side.addVolume(top);

                top.shares = shr; // restore
                top.price = pr; // restore
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



void an::Book::cancelActiveOrder(order_id_t id, std::unique_ptr<CancelOrder> o) {
    if (!open_) {
        sendReject(o.get(), "book not open");
        return;
    }
    SideRecord* recPtr = findSideRecord(id);
    if (recPtr != nullptr) {
        assert(recPtr->visible && "cancelActiveOrder must be visible");
        Execution* exe = findActiveOrder(id);
        assert(exe != nullptr && "cancelActiveOrder order not found"); //TODO - not required
        if (exe->origin() == o->origin()) {
            if (recPtr->direction == an::BUY) {
                buy_.remove(*recPtr);
            } else {
                sell_.remove(*recPtr);
            }
            assert(recPtr->visible == false && "cancelActiveOrder non-visible");
            sendCancel(exe,"order cancel success");
            removeActiveOrder(id);
        } else {
            sendReject(o.get(), "origin mismatch");
        }
    } else {
        assert(findActiveOrder(id) == nullptr && "cancelActiveOrder active order not in a side");
        sendReject(o.get(), "order not found");
    }
}



void an::Book::amendActiveOrder(order_id_t id, std::unique_ptr<AmendOrder> o) {
    if (!open_) {
        sendReject(o.get(), "book not open");
        return;
    }
    SideRecord* recPtr = findSideRecord(id);
    if (recPtr != nullptr) {
        const auto& amend = o->amend();
        if (amend.field == PRICE) {
            bool amended = false;
            bool tick = false;
            bool mismatch = false;
            if (tick_table_.validatePrice(amend.price)) {
                SideRecord newRec(*recPtr);
                bool awayFromTouch = ((newRec.direction==BUY)  && (amend.price <= newRec.price)) ||
                                     ((newRec.direction==SELL) && (amend.price >= newRec.price)) ;
                // Away from touch, (TODO not better than touch ???)
                Execution* exe = nullptr;
                std::unique_ptr<Execution> exeNew;
                if (awayFromTouch) {
                    exe = findActiveOrder(id);
                    assert(exe != nullptr && "amendActiveOrder by price order not found");
                } else {
                    exeNew.reset(findActiveOrder(id, true)); // Find and remove
                    exe = exeNew.get();
                }
                if (exe->origin() == o->origin()) {
                    if ((amended = exe->amend(amend)) == true) {
                        Side* s = &sell_;
                        if (newRec.direction == BUY) {
                            s = &buy_;
                        }

                        s->remove(*recPtr, true); recPtr = nullptr;
                        newRec.price = amend.price;
                        newRec.visible = true;
                        if (awayFromTouch) {
                            s->add(newRec); // Just change order book price and re-add
                        } else {
                            // Towards touch, might be marketable
                            assert(exeNew != nullptr && "amendActiveOrder by price order (exeNew) not found");
                            executeOrder(newRec,std::move(exeNew));
                        }
                    }
                } else {
                    mismatch=true;
                }
            } else {
                tick = true;
            }
            if (amended) {
                sendAmend(o.get());
            } else if (tick) {
                sendReject(o.get(), "Invalid tick price" );
            } else if (mismatch) {
                sendReject(o.get(), "origin mismatch");
            } else {
                sendReject(o.get(), "Invalid amend of execution order");
            }
        } else {
            // Shares
            Execution* exe = findActiveOrder(id);
            assert(exe != nullptr && "amendActiveOrder price order not found");
            if (exe->origin() == o->origin()) {
                if (exe->amend(amend)) {
                    shares_t shr = recPtr->shares;
                    recPtr->shares = amend.shares;
                    Side* s = &sell_;
                    if (recPtr->direction == BUY) {
                        s = &buy_;
                    }
                    s->amendShares(*recPtr, shr);
                    sendAmend(o.get());
                } else {
                    sendReject(o.get(), "Invalid amend of execution order");
                }
            } else {
                sendReject(o.get(), "origin mismatch");
            }
        }
    } else {
        sendReject(o.get(), "unknown order");
    }
}


void an::Book::executeOrder(SideRecord& rec, std::unique_ptr<Execution> exe) {
    if (!open_) {
        sendReject(exe.get(),"book not open");
        return;
    }
    if ( (rec.order_type == LIMIT) && (!tick_table_.validatePrice(rec.price)) ) {
        sendReject(exe.get(), "invalid tick size (price)");
        return;
    }
    if (findActiveOrder(rec.id) != nullptr) {
        sendReject(exe.get(), "Order id already on book");
        return;
    }
    if (!marketable(rec, exe.get())) {
        if (rec.order_type == LIMIT) {
            // Add to queue
            addSideRecord(rec);
            addActiveOrder(rec.id, std::move(exe), rec.direction);
        } else if (rec.order_type == MARKET) {
            sendCancel(exe.get(), "no bid/ask for market order");
        } else {
            assert(false && "executeOrder unknown order_type");
        }
    }
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
    //std::cout << "Book::marketable" << " id=" << newExe->orderId() << " marketable=" << marketable << std::endl;
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

