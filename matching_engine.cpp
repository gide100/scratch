#include "types.hpp"
#include "matching_engine.hpp"
#include "security_master.hpp"
#include <boost/format.hpp>


an::MatchingEngine::MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb)
             : seq_(1), startTime_(std::chrono::steady_clock::now()), exchange_(exchange), secdb_(secdb) { 
    for (const auto& sec : secdb.securities() ) {
        side_.push_back(Side{sec.symbol,Book(),Book()});
    }
}

an::MatchingEngine::~MatchingEngine() { }

std::string an::MatchingEngine::to_string() const {
    std::ostringstream os;
    os << "id     Side Time         Shares Price    Shares Time         Side" << std::endl 
       << "------+----+------------+------+--------+------+------------+----" << std::endl ;
    for (const auto& rec: side_[0].sell) {
       os << boost::format("%1$6d ") % rec.id
          << "                         " 
          << boost::format("%1$8.3f ") % rec.price
          << boost::format("%1$6d %2$12s %3$4s") % rec.shares % sinceToHHMMSS(rec.since) % an::to_string(rec.direction)
          << std::endl;
    }
    for (const auto& rec: side_[0].buy) {
       os << boost::format("%1$6d ") % rec.id
          << boost::format("%1$4s %2$12s %3$6d ") % an::to_string(rec.direction) % sinceToHHMMSS(rec.since) % rec.shares 
          << boost::format("%1$8.3f") % rec.price
          << std::endl;
    }
 
    return os.str();
}


void an::MatchingEngine::applyOrder(Execution* o) {
    BookRecord rec;
    o->pack(rec); // TODO TImestamp and sequence
    const symbol_t& symbol = o->symbol();
    auto symbol_idx = secdb_.find(symbol);
    std::cout << "SYMBOL=" << symbol << " idx=" << symbol_idx << std::endl;
    assert(symbol_idx != std::numeric_limits<std::size_t>::max()); 
    assert(side_[symbol_idx].symbol == symbol); // Using the right book
    if (isMarketable(rec)) {
         std::cout << "FOUND Marketable: " << o->to_string(); // TODO
    } else {
         // Add to queue
         if (rec.direction == an::BUY) {
              side_[symbol_idx].buy.push_back(rec);
         } else if (rec.direction == an::SELL) {
              side_[symbol_idx].sell.push_back(rec);
         } else {
             assert(false); // Unknown direction    
         }
    }
}

void an::MatchingEngine::applyOrder(CancelOrder* o) {
}

void an::MatchingEngine::applyOrder(AmendOrder* o) {
}

bool an::MatchingEngine::isMarketable(BookRecord& rec) {
    return false; // TODO
}

