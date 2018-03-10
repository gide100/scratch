#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "types.hpp"
#include "order.hpp"

namespace an {

class Book;
class SecurityDatabase ;
//
//typedef std::chrono::microseconds since_t;
typedef std::chrono::steady_clock::time_point since_t;

// ************************** MATCHING ENGINE ******************************

class MatchingEngine {
    public:
        MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb);
        ~MatchingEngine();

        void close();
        std::string to_string() const;

        void applyOrder(std::unique_ptr<Execution> o);
        //void applyOrder(std::unique_ptr<MarketOrder> o);
        //void applyOrder(std::unique_ptr<LimitOrder> o);
        void applyOrder(std::unique_ptr<CancelOrder> o);
        void applyOrder(std::unique_ptr<AmendOrder> o);

        inline static std::string sinceToString(
                since_t time, 
                std::chrono::steady_clock::time_point steadyClockStartTime,
                std::chrono::system_clock::time_point systemClockStartTime) {
            std::ostringstream os;
            // https://stackoverflow.com/questions/18361638/converting-steady-clocktime-point-to-time-t
            // https://stackoverflow.com/questions/35282308/convert-between-c11-clocks
            //const auto system_now = system_clock::now() ;
            //const auto steady_now = steady_clock::now();
            std::chrono::system_clock::time_point myTime = systemClockStartTime + (time - steadyClockStartTime);
            //system_clock::time_point myTime = time_point_cast<system_clock::duration>(systemClockStartTime_ + (time - steadyClockStartTime_));
            os << date::format("%T",myTime);
            // os << "HH:MM:SS.SSSS";
            return os.str();
        }

        static void sendTradeReport(Order* o, direction_t d, shares_t s, price_t p);
        static void sendResponse(Message* o, response_t r, text_t t);
        const std::chrono::steady_clock::time_point& steadyClockStartTime() const {
            return steadyClockStartTime_;
        }
        const std::chrono::system_clock::time_point& systemClockStartTime() const {
            return systemClockStartTime_;
        }
    private:
        //void applyOrder(std::unique_ptr<Execution> o);
        Book* findBook(const symbol_t& symbol);
     
        sequence_t  seq_;
        std::chrono::steady_clock::time_point steadyClockStartTime_;
        std::chrono::system_clock::time_point systemClockStartTime_;
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
        

        std::vector<Book> book_;
};

// ************************** BOOK ******************************


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

// In the priority queue SELL (ASK) should be ascending price/time, conversely 
// in BUY (BID) the price should descending price/ascedning time.
// For convenience added price_desc_ as sometimes we want to show output in descending price. 
struct CompareSideRecord {
    CompareSideRecord(bool one_list=false) : one_list_(one_list) {}
    typedef const SideRecord& first_argument_type;
    typedef const SideRecord& second_argument_type;
    bool operator()(const SideRecord& lhs, const SideRecord& rhs) const {
        bool result = false;
        if (lhs.direction != rhs.direction) {
            result = lhs.direction < rhs.direction;
        } else if (one_list_  && (lhs.price != rhs.price)) {
            result = lhs.price < rhs.price;
        } else if (!one_list_ && (lhs.price != rhs.price)) {
            result = ((lhs.direction==an::BUY)  && lhs.price < rhs.price) ||
                     ((lhs.direction==an::SELL) && lhs.price > rhs.price) ;
        } else if (one_list_ && (lhs.time != rhs.time)) {
            result = lhs.time > rhs.time; // Ascending times
        } else if (!one_list_ && (lhs.time != rhs.time)) {
            result = ((lhs.direction==an::BUY)  && (lhs.time > rhs.time)) || // Ascending times
                     ((lhs.direction==an::SELL) && (lhs.time < rhs.time)) ;  // Descending times
        } else {
            result = lhs.seq > rhs.seq; // Ascending sequence
        }
        return result;
    }

    bool one_list_; // Output in one list
};

static const SideRecord DefaultSideRecord =  
   { .id=0, .seq=0, .time=since_t(), .order_type=an::LIMIT, 
     .direction=an::BUY, .price=0.0, .shares=0, .visible=false, .matched=false }; 


// Priority queue with accessible Container type.
template<class T, class C = std::vector<T>, class P = std::less<typename C::value_type> >                     
struct PriorityQueue : std::priority_queue<T,C,P> {                                                           
    //using std::priority_queue<T,C,P>::priority_queue;                                                       
    typename C::iterator begin() { return std::priority_queue<T, C, P>::c.begin(); }                          
    typename C::iterator end() { return std::priority_queue<T, C, P>::c.end(); }                              
    typename C::const_iterator begin() const { return std::priority_queue<T, C, P>::c.cbegin(); }            
    typename C::const_iterator end() const { return std::priority_queue<T, C, P>::c.cend(); }                
    typename C::const_iterator cbegin() const { return std::priority_queue<T, C, P>::c.cbegin(); }            
    typename C::const_iterator cend() const { return std::priority_queue<T, C, P>::c.cend(); }                
};
                                                                                                              
class Book {
    public:
        explicit Book(symbol_t sym, 
                      std::chrono::steady_clock::time_point& steadyClockStartTime,                                     
                      std::chrono::system_clock::time_point& systemClockStartTime) 
            : symbol_(sym), open_(false), active_order_(), buy_(), sell_(),
             steadyClockStartTime_(steadyClockStartTime), systemClockStartTime_(systemClockStartTime) {
        }
        Book(const Book& book) : symbol_(book.symbol_), 
             steadyClockStartTime_(book.steadyClockStartTime_), systemClockStartTime_(book.systemClockStartTime_) {
            assert(book.open_!=true && "Cannot copy open book");
        }
        Book& operator=(const Book& book) = delete;
        ~Book() { assert(!open_); };

        std::string to_string(bool verbose = false) const;

        void open() { assert(!open_); open_ = true; }
        void close() { 
            if (open_) {
                closeBook(); open_ = false; 
            }
            assert(active_order_.empty() && "active orders empty after closeBook()");
        }
        bool matchSymbol(const symbol_t& symbol) { return symbol == symbol_; }
        // Activate Orders
        void addActiveOrder(order_id_t id, std::unique_ptr<Execution> o, direction_t d) {
            active_order_.emplace(id, std::move(open_order{id, std::move(o), d}));                                                 
        }
        void addSideRecord(SideRecord& rec) {
            // Add to queue                                                                                   
            if (rec.direction == an::BUY) {                                                                   
                 buy_.push(rec);                                                                          
            } else if (rec.direction == an::SELL) {                                                           
                 sell_.push(rec);
            } else {
                assert(false); // Unknown direction
            }
        }
        void cancelActiveOrder(order_id_t id, std::unique_ptr<CancelOrder> o) {
            if (!open_) {
                sendResponse(o.get(), REJECT, "book not open");
                return;
            }
            SideRecord* recPtr = findSideRecord(id);
            if (recPtr != nullptr) {
                recPtr->visible = false;
                Order* exe = findActiveOrder(id);
                assert(exe != nullptr && "cancelActiveOrder order not found"); //TODO - not required
                sendResponse(exe, an::COMPLETE,"cancel success"); 
                removeActiveOrder(id);
            } else {
                assert(findActiveOrder(id) == nullptr && "cancelActiveOrder active order not in a side");
                sendResponse(o.get(), REJECT, "order not found");
            }
        }
        void amendActiveOrder(order_id_t id, std::unique_ptr<AmendOrder> o) {
            if (!open_) {
                sendResponse(o.get(), an::REJECT, "book not open");
                return;
            }
            SideRecord* recPtr = findSideRecord(id);
            if (recPtr != nullptr) {
                const auto& amend = o->amend();
                if (amend.field == PRICE) {
                    bool amended = false;
                    SideRecord newRec(*recPtr);
                    if ( ((newRec.direction==BUY)  && (amend.price <= newRec.price)) ||
                         ((newRec.direction==SELL) && (amend.price >= newRec.price)) ) {
                        // Away from touch, (TODO not better than touch ???)
                        Execution* exe = findActiveOrder(id);
                        if ((amended = exe->amend(amend)) == true) {
                            newRec.price = amend.price;
                            recPtr->visible = false;
                            newRec.visible = true;
                            addSideRecord(newRec);
                        }
                    } else {
                        // Towards touch, might be marketable
                        std::unique_ptr<Execution> exe;
                        exe.reset(findActiveOrder(id, true));
                        if ((amended = exe->amend(amend)) == true) {
                            newRec.price = amend.price;
                            // As it's an existing order check whether it is marketable
                            recPtr->visible = false;
                            newRec.visible = true;
                            executeOrder(newRec,std::move(exe));
                        }
                    }
                    if (!amended) {
                        sendResponse(o.get(), REJECT, "Invalid amend of execution order");
                    }
                } else {
                    // Shares
                    Execution* exe = findActiveOrder(id);
                    if (!exe->amend(amend)) {
                        sendResponse(o.get(), REJECT, "Invalid amend of execution order");
                    } else {
                        recPtr->shares = amend.shares;
                        sendResponse(o.get(), COMPLETE, "amend success");
                    }
                }
            } else {
                sendResponse(o.get(), REJECT, "unknown order");
            }
        }
        void executeOrder(SideRecord& rec, std::unique_ptr<Execution> exe) {
            if (!open_) {
                sendResponse(exe.get(), REJECT, "book not open");
                return;
            }
            if (!marketable(rec, exe.get())) {                                                                      
                if (rec.order_type == LIMIT) {
                    // Add to queue                                                                                   
                    addSideRecord(rec);                                                                          
                    addActiveOrder(rec.id, std::move(exe), rec.direction);                                                    
                } else if (rec.order_type == MARKET) {
                    sendResponse(exe.get(), CANCELLED, "no bid/ask for market order");
                } else {
                    assert(false && "marketableOrBook Unknown order_type");
                }
            }           
        }
    private:
        void sendTradeReport(Order* o, direction_t d, shares_t s, price_t p) {
            MatchingEngine::sendTradeReport(o,d,s,p);
        }
        void sendResponse(Message* o, response_t r, text_t t) {
            MatchingEngine::sendResponse(o, r, t);
        }
        bool marketable(SideRecord& newRec, Execution* exe);

        void closeBook();
        Execution* findActiveOrder(order_id_t id, bool release=false) {
            Execution* found = nullptr;
            auto search = active_order_.find(id);
            if (search != active_order_.end()) {
                found = search->second.order.get();
                if (release) {
                    search->second.order.release();
                    (void) active_order_.erase(search);
                    //assert(count==1 && "findActiveOrder id not found");
                }
            }
            return found;
        }
        void removeActiveOrder(order_id_t id) {
            auto count = active_order_.erase(id);
            assert(count==1 && "removeActiveOrder id not found");
        }

        struct MatchOrderId { // Functor                                                                              
            explicit MatchOrderId(an::order_id_t order_id) : order_id_(order_id) { }                                  
            bool operator()(const an::SideRecord& rec) const { return rec.id == order_id_; }                          
            an::order_id_t order_id_;                                                                                 
        };
        SideRecord* findSideRecord(order_id_t id) {
            SideRecord* found = nullptr;
            auto search = active_order_.find(id);
            if (search != active_order_.end()) {
                Side::container_type::iterator it;
                if (search->second.direction == an::BUY) {
                    it = std::find_if(buy_.begin(), buy_.end(), MatchOrderId(id));
                    if (it != buy_.end()) {
                        found = &(*it);
                    }
                } else {
                    it = std::find_if(sell_.begin(), sell_.end(), MatchOrderId(id));
                    if (it != sell_.end()) {
                        found = &(*it);
                    }
                }
            }
            return found;
        }
        std::string sinceToString(since_t time) const {
            return MatchingEngine::sinceToString(time, steadyClockStartTime_, systemClockStartTime_);
        }

        struct open_order {
            order_id_t                  id;
            std::unique_ptr<Execution>  order;
            direction_t                 direction;
        };
        //typedef std::vector<SideRecord> Side;
        typedef std::unordered_map<order_id_t, open_order> active_order_t;
        typedef PriorityQueue<SideRecord, std::deque<SideRecord>, CompareSideRecord > Side; 

        bool marketableSide(Side& side, SideRecord& newRec, Execution* newExe);

        symbol_t symbol_;
        bool open_;
        active_order_t active_order_;
        Side buy_;
        Side sell_;
        std::chrono::steady_clock::time_point& steadyClockStartTime_;
        std::chrono::system_clock::time_point& systemClockStartTime_;
}; // Book


inline std::string to_string(const SideRecord& sr, 
        const std::chrono::steady_clock::time_point& steadyClockStartTime, 
        const std::chrono::system_clock::time_point& systemClockStartTime) {
    std::ostringstream os;
    os << "seq=" << sr.seq << " direction=" << to_string(sr.direction) << " price=" << sr.price << " time=" 
       << MatchingEngine::sinceToString(sr.time, steadyClockStartTime, systemClockStartTime);
    return os.str();
}

} // an - namespace

#endif

