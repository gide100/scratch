#include "types.hpp"
#include "matching_engine.hpp"
#include "security_master.hpp"
#include <boost/format.hpp>


an::MatchingEngine::MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb, bool bookkeep)
             : seq_(1), 
             epoch_{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() },
             exchange_(exchange), secdb_(secdb) { 
    book_.reserve(secdb.securities().size() *2);
    for (const auto& sec : secdb.securities() ) {
        book_.emplace_back(sec.symbol,epoch_, bookkeep, sec.closing_price);
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


//void an::MatchingEngine::applyOrder(std::unique_ptr<LimitOrder> o) {
//    std::unique_ptr<Execution> exe; exe.reset(o.get()); o.release();
//    applyOrder(std::move(exe));
//}
//
//void an::MatchingEngine::applyOrder(std::unique_ptr<MarketOrder> o) {
//    std::unique_ptr<Execution> exe; exe.reset(o.get()); o.release();
//    applyOrder(std::move(exe));
//}

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
bool an::Book::marketableSide(Side& side, SideRecord& newRec, Execution* newExe) {
    bool marketable = false;
    while (!side.empty() && (newRec.shares!=0)) {
        marketable = false;
        SideRecord top = side.top();
        if(!top.visible) {
            sell_.pop();
            continue;
        }
        if ( ((newRec.direction==an::BUY)  && compare3decimalplaces(newRec.price,top.price) >= 0) ||
             ((newRec.direction==an::SELL) && compare3decimalplaces(newRec.price,top.price) <= 0) ) {
            side.pop(); // Remove top
            shares_t shares = std::min(newRec.shares,top.shares);
            newRec.shares -= shares; // Reduce number of shares needed by newRec
            top.shares -= shares; // New shares are less than matched (at top of book).

            sendTradeReport(newExe, top.direction, shares, top.price);
            Execution* exeOld = findActiveOrder(top.id);
            assert(exeOld != nullptr && "marketable active order null");
            sendTradeReport(exeOld, newRec.direction, shares, top.price);
            if (top.shares == 0) {
                sendResponse(exeOld, an::COMPLETE, "Top Filled");
                removeActiveOrder(top.id);
            } else {
                sell_.push(std::move(top)); // Recover top
            }
            if (newRec.shares == 0) {
                sendResponse(newExe, an::COMPLETE, "New Filled");
                newRec.visible = false;
                marketable = true; 
            }
        } else {
            // Not marketable if order values not matching
            break;
        }
    }
    return marketable;
}

bool an::Book::marketable(SideRecord& newRec, Execution* newExe) {
    bool marketable = false;
    if (newRec.direction==an::BUY) {
        // Compare against sell
        marketable = marketableSide(sell_, newRec, newExe);
    } else {
        marketable = marketableSide(buy_, newRec, newExe);
    }
    std::cout << "id=" << newExe->orderId() << " marketable=" << marketable << std::endl;
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
        Side::container_type data;                                                                                  
        for (const auto& rec : sell_) {                                                                                      
            data.push_back(rec);                                                                                    
        }
        std::sort( data.begin(), data.end(), std::not2(Side::value_compare(true)) );                                    
        for (const auto& rec: data) { // SELL
           os << boost::format("%1$6d %2$4d %3$1c ") % rec.id % rec.seq % (rec.visible ? '*' : '.')
              << boost::format("%1$30c ") % ' '
              << boost::format("%1$8.3f ") % rec.price
              << boost::format("%1$6d %2$18s %3$4s") % rec.shares % sinceToString(rec.time) % an::to_string_book(rec.direction)
              << std::endl;
        }
        data.clear();
        for (const auto &rec : buy_) {                                                                                      
            data.push_back(rec);                                                                                    
        }
        std::sort( data.begin(), data.end(), std::not2(Side::value_compare(true)) );                                    
        for (const auto& rec: data) { // BUY
           os << boost::format("%1$6d %2$4d %3$1c ") % rec.id % rec.seq % (rec.visible ? '*' : '.')
              << boost::format("%1$4s %2$18s %3$6d ") % an::to_string_book(rec.direction) % sinceToString(rec.time) % rec.shares 
              << boost::format("%1$8.3f") % rec.price
              << std::endl;
        }
        os << std::endl;

    } else {
        os << boost::format("[%1$s]") % symbol_ << std::endl;
        Side b1(sell_);
        while (!b1.empty()) {
            os << an::to_string(b1.top(),epoch_) << std::endl;
            b1.pop();
        }
        Side b2(buy_);
        while (!b2.empty()) {
            os << an::to_string(b2.top(),epoch_) << std::endl;
            b2.pop();
        }
    }

    return os.str();
}

