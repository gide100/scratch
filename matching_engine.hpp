#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "types.hpp"
#include "order.hpp"

namespace an {

//typedef std::chrono::microseconds since_t;
typedef std::chrono::steady_clock::time_point since_t;

struct SideRecord {
    order_id_t  id;
    sequence_t  seq;
    since_t     time;
    order_t     order_type;
    direction_t direction;
    price_t     price;
    shares_t    shares;
    bool        visible;
    bool        matched;
};

static const SideRecord DefaultSideRecord =  
   { .id=0, .seq=0, .time=since_t(), .order_type=an::LIMIT, 
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
        bool isMarketable(SideRecord& rec);
        SideRecord* lookupSymbol(const symbol_t& symbol, order_id_t id) const;
     
        sequence_t  seq_;
        std::chrono::steady_clock::time_point startTime_;
        an::location_t exchange_;
        an::SecurityDatabase& secdb_;

//        // https://stackoverflow.com/questions/17946124/most-simple-way-to-get-string-containing-time-interval
//        inline static std::string sinceToHHMMSS(since_t since) {
//            std::ostringstream os;
//            auto hh = std::chrono::duration_cast<std::chrono::hours>(since);
//            since -= hh;
//            auto mm = std::chrono::duration_cast<std::chrono::minutes>(since);
//            since -= mm;
//            auto ss = std::chrono::duration_cast<std::chrono::seconds>(since);
//            since -= ss;
//            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(since);
//            
//            os << std::setfill('0') << std::setw(2) << hh.count() << ':'
//               << std::setfill('0') << std::setw(2) << mm.count() << ':' 
//               << std::setfill('0') << std::setw(2) << ss.count() << '.' 
//               << std::setfill('0') << std::setw(3) << ms.count() ; 
//            return os.str();
//        }
        
        inline static std::string sinceToString(since_t time) {
            using namespace date;
            using namespace std::chrono;
            std::ostringstream os;
            // https://stackoverflow.com/questions/18361638/converting-steady-clocktime-point-to-time-t
            // https://stackoverflow.com/questions/35282308/convert-between-c11-clocks
            const auto system_now = system_clock::now() ;
            const auto steady_now = steady_clock::now();
            system_clock::time_point myTime = time_point_cast<system_clock::duration>(time - steady_now + system_now);
            os << format("%T",myTime);
            // os << "HH:MM:SS.SSSS";
            return os.str();
        }

        typedef std::vector<SideRecord> Side;
        typedef std::unordered_map<order_id_t, an::direction_t> active_order_t;
        //std::priority_queue<SideRecordOrder, std::deque<SideRecord>,
        struct Book {
            symbol_t symbol;
            active_order_t active_order;
            Side buy;
            Side sell;
        } ;
        std::vector<Book> book_;
};


} // an - namespace

#endif

