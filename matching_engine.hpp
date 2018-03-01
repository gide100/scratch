#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "types.hpp"
#include "order.hpp"
#include <vector>

namespace an {

typedef std::chrono::microseconds since_t;

struct BookRecord {
    order_id_t  id;
    sequence_t  seq;
    since_t     since;
    order_t     order_type;
    direction_t direction;
    price_t     price;
    shares_t    shares;
    bool        visible;
    bool        matched;
};

static const BookRecord DefaultBookRecord =  
   { .id=0, .seq=0, .since=since_t(0), .order_type=an::LIMIT, 
     .direction=an::BUY, .price=0.0, .shares=0, .visible=false, .matched=false }; 
    
class SecurityDatabase ;

class MatchingEngine {
    public:
        MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb);
        ~MatchingEngine();

        std::string to_string() const;

        void applyOrder(Execution* o);
        void applyOrder(CancelOrder* o);
        void applyOrder(AmendOrder* o);
    private:
        bool isMarketable(BookRecord& rec);
     
        sequence_t  seq_;
        std::chrono::steady_clock::time_point startTime_;
        an::location_t exchange_;
        an::SecurityDatabase& secdb_;

        inline static std::string sinceToHHMMSS(since_t since) {
            std::ostringstream os;
            auto hh = std::chrono::duration_cast<std::chrono::hours>(since);
            since -= hh;
            auto mm = std::chrono::duration_cast<std::chrono::minutes>(since);
            since -= mm;
            auto ss = std::chrono::duration_cast<std::chrono::seconds>(since);
            since -= ss;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(since);
            
            os << std::setfill('0') << std::setw(2) << hh.count() << ':'
               << std::setfill('0') << std::setw(2) << mm.count() << ':' 
               << std::setfill('0') << std::setw(2) << ss.count() << '.' 
               << std::setfill('0') << std::setw(3) << ms.count() ; 
            return os.str();
        }

        typedef std::vector<BookRecord> Book;
        //std::priority_queue<BookRecordOrder, std::deque<BookRecord>,
        struct Side {
            symbol_t symbol;
            Book buy;
            Book sell;
        } ;
        std::vector<Side> side_;
};


} // an - namespace

#endif

