#include "types.hpp"
#include "matching_engine.hpp"
#include "security_master.hpp"
#include <boost/format.hpp>


an::MatchingEngine::MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb)
             : seq_(1), startTime_(std::chrono::steady_clock::now()), exchange_(exchange), secdb_(secdb) { 
    for (const auto& sec : secdb.securities() ) {
        book_.push_back(Book{sec.symbol,active_order_t(),Side(),Side()});
    }
}

an::MatchingEngine::~MatchingEngine() { }

std::string an::MatchingEngine::to_string() const {
    std::ostringstream os;
    os << "id     Seq  V Side Time               Shares Price    Shares Time               Side" << std::endl 
       << "------+----+-+----+------------------+------+--------+------+------------------+----" << std::endl ;
    for (const auto& rec: book_[0].sell) {
       os << boost::format("%1$6d %2$4d %3$1c ") % rec.id % rec.seq % (rec.visible ? '*' : ' ')
          << boost::format("%1$30c ") % ' '
          << boost::format("%1$8.3f ") % rec.price
          << boost::format("%1$6d %2$18s %3$4s") % rec.shares % sinceToString(rec.time) % an::to_string(rec.direction)
          << std::endl;
    }
    for (const auto& rec: book_[0].buy) {
       os << boost::format("%1$6d %2$4d %3$1c ") % rec.id % rec.seq % (rec.visible ? '*' : ' ')
          << boost::format("%1$4s %2$18s %3$6d ") % an::to_string(rec.direction) % sinceToString(rec.time) % rec.shares 
          << boost::format("%1$8.3f") % rec.price
          << std::endl;
    }
 
    return os.str();
}


void an::MatchingEngine::applyOrder(Execution* o) {
    SideRecord rec;
    o->pack(rec); 
    rec.time = std::chrono::steady_clock::now();
    rec.seq = seq_++;
    rec.visible = true;
    const symbol_t& symbol = o->symbol();
    auto symbol_idx = secdb_.find(symbol);
    std::cout << "SYMBOL=" << symbol << " idx=" << symbol_idx << std::endl;
    assert(symbol_idx != std::numeric_limits<std::size_t>::max()); 
    assert(book_[symbol_idx].symbol == symbol); // Using the right book
    if (isMarketable(rec)) {
         std::cout << "FOUND Marketable: " << o->to_string(); // TODO
    } else {
         // Add to queue
         if (rec.direction == an::BUY) {
              book_[symbol_idx].buy.push_back(rec);
         } else if (rec.direction == an::SELL) {
              book_[symbol_idx].sell.push_back(rec);
         } else {
             assert(false); // Unknown direction    
         }
         book_[symbol_idx].active_order.emplace(rec.id, rec.direction);
    }
}

void an::MatchingEngine::applyOrder(CancelOrder* o) {
    const symbol_t& symbol = o->symbol();
    order_id_t id = o->orderId();
    SideRecord* recPtr = lookupSymbol(symbol, id);
    recPtr->visible = false; // TODO - add more cancel stuff, cancel thread?
}

void an::MatchingEngine::applyOrder(AmendOrder* o) {
    const symbol_t& symbol = o->symbol();
    order_id_t id = o->orderId();
    SideRecord* recPtr = lookupSymbol(symbol, id);
    if (recPtr != nullptr) {
        const auto& amend = o->amend();
        switch (amend.field) {
            case NONE:   break;
            case PRICE:  if (recPtr->order_type == LIMIT) {
                             recPtr->price = amend.price; 
                         } else {
                             throw OrderError("Invalid amend of market order");
                         }
                         break;
            case SHARES: recPtr->shares = amend.shares; break;
            default:
                throw OrderError("Invalid amend");
        }
    } else {
        // TODO- Not found
    } 
}

bool an::MatchingEngine::isMarketable(SideRecord& rec) {
    return false; // TODO
}

struct MatchOrderId { // Functor
    explicit MatchOrderId(an::order_id_t order_id) : order_id_(order_id) { }
    bool operator()(const an::SideRecord& rec) const { return rec.id == order_id_; }
    an::order_id_t order_id_;
};

an::SideRecord* an::MatchingEngine::lookupSymbol(const symbol_t& symbol, order_id_t id) const {
    auto symbol_idx = secdb_.find(symbol);
    const SideRecord* found = nullptr;
    if (symbol_idx != std::numeric_limits<std::size_t>::max()) { // FOUND
        std::cout << "LOOKUP=" << symbol << " idx=" << symbol_idx << std::endl;
        auto search = book_[symbol_idx].active_order.find(id);
        if (search != book_[symbol_idx].active_order.end()) {
	    Side::const_iterator it ;
            if (search->second == an::BUY) {
                it = std::find_if(book_[symbol_idx].buy.cbegin(),  book_[symbol_idx].buy.cend(),  MatchOrderId(id)); 
                if (it != book_[symbol_idx].buy.cend()) {
                    found = &(*it);
                }
            } else {
                it = std::find_if(book_[symbol_idx].sell.cbegin(), book_[symbol_idx].sell.cend(), MatchOrderId(id)); 
                if (it != book_[symbol_idx].sell.cend()) {
                    found = &(*it);
                }
            }
        }
    }
    return const_cast<SideRecord*>(found);
}
