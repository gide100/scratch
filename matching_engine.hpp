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
struct epoch_t {
    std::chrono::steady_clock::time_point steadyClockStartTime;
    std::chrono::system_clock::time_point systemClockStartTime;
};

inline static std::string sinceToString(since_t time, const epoch_t& epoch) {
    std::ostringstream os;
    // https://stackoverflow.com/questions/18361638/converting-steady-clocktime-point-to-time-t
    // https://stackoverflow.com/questions/35282308/convert-between-c11-clocks
    std::chrono::system_clock::time_point myTime = epoch.systemClockStartTime + (time - epoch.steadyClockStartTime);
    os << date::format("%T",myTime); // "HH:MM:SS.SSSS";
    return os.str();
}

// ************************** MATCHING ENGINE ******************************

class MatchingEngine {
    public:
        MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb, bool bookkeep=true);
        ~MatchingEngine();

        void close();
        std::string to_string() const;

        void applyOrder(std::unique_ptr<Execution> o);
        //void applyOrder(std::unique_ptr<MarketOrder> o);
        //void applyOrder(std::unique_ptr<LimitOrder> o);
        void applyOrder(std::unique_ptr<CancelOrder> o);
        void applyOrder(std::unique_ptr<AmendOrder> o);

        static void sendTradeReport(Order* o, direction_t d, shares_t s, price_t p);
        static void sendResponse(Message* o, response_t r, text_t t);
        const epoch_t& epoch() const {
            return epoch_;
        }
    private:
        //void applyOrder(std::unique_ptr<Execution> o);
        Book* findBook(const symbol_t& symbol);
     
        sequence_t  seq_;
        epoch_t     epoch_;
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


class Bookkeeper { 
    public:
        Bookkeeper(date_t date, price_t previous_close, const epoch_t& epoch) 
            : trading_date_(date), epoch_(epoch), previous_close_(previous_close),
              shares_traded_(0), volume_(0.0), trades_(0), cancels_(0), amends_(0), rejects_(0), 
              daily_high_(0.0), daily_low_(0.0), 
              open_price_(0.0), close_price_(0.0),
              avg_share_price_(0.0), last_trade_price_(0.0), last_trade_time_() {
        }
        ~Bookkeeper() { }

        void trade(direction_t direction, shares_t shares, price_t price, since_t since) {
            shares_traded_ += shares;
            volume_ += price * shares;
            if (trades_ == 0) {
                daily_high_ = price;
                daily_low_ = price;
                open_price_ = price;
            } else {
                daily_high_ = std::max(daily_high_,price);
                daily_low_ = std::min(daily_low_,price);
            }
            ++trades_;
            last_trade_price_ = price;
            last_trade_time_ = since;
        }
        void cancel() {
            ++cancels_;
        }
        void amend() {
            ++amends_;
        }
        void reject() {
            ++rejects_;
        }
        void close() {
            close_price_ = last_trade_price_;
            if (trades_ !=0) {
                avg_share_price_ = volume_ / shares_traded_;
            }
        }
        std::string to_string(bool verbose = false) {
            std::ostringstream os;
            os << dateToString(trading_date_) << " prev=" << previous_close_ 
               << " shares=" << shares_traded_ << " volume=" << volume_
               << " trades=" << trades_ << " cancels=" << cancels_ 
               << " amends=" << amends_ << " rejects=" << rejects_;
                if (trades_ != 0) {
                    os << " high=" << daily_high_ << " low=" << daily_low_ 
                       << " open=" << open_price_ << " close=" << close_price_
                       << " avg=" << avg_share_price_
                       << " last_price=" << last_trade_price_ 
                       << " last_time=" << sinceToString(last_trade_time_,epoch_);
                } else {
                    os << " high=N/A low=N/A open=N/A close=N/A avg=N/A last_price=N/A last_time=N/A";
                }
            return os.str();
        }
    private:
        date_t          trading_date_;
        const epoch_t&  epoch_;
        price_t         previous_close_;
        shares_t        shares_traded_; 
        volume_t        volume_; 
        counter_t       trades_; // If zero no trading, last_trade_time_ invalid, etc
        counter_t       cancels_;
        counter_t       amends_;
        counter_t       rejects_;
        price_t         daily_high_;
        price_t         daily_low_;
        price_t         open_price_;
        price_t         close_price_;
        price_t         avg_share_price_;
        price_t         last_trade_price_;
        since_t         last_trade_time_;
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
        explicit Book(symbol_t sym, epoch_t& epoch, bool bookkeep, price_t closing_price) 
            : symbol_(sym), open_(false), active_order_(), buy_(), sell_(), epoch_(epoch),
              bookkeep_(bookkeep), 
              bookkeeper_(
                std::chrono::system_clock::to_time_t(date::floor<date::days>(std::chrono::system_clock::now())),
                closing_price, epoch_) {
        }
        Book(const Book& book) 
            : symbol_(book.symbol_), epoch_(book.epoch_), 
              bookkeep_(book.bookkeep_), bookkeeper_(book.bookkeeper_) { 
            assert(book.open_!=true && "Cannot copy open book");
        }
        Book& operator=(const Book& book) = delete;
        ~Book() { assert(!open_); };

        std::string to_string(bool verbose = false) const;

        void open() { assert(!open_); open_ = true; }
        void close() { 
            if (open_) {
                bookkeeper_.close();
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
                sendReject(o.get(), "book not open");
                return;
            }
            SideRecord* recPtr = findSideRecord(id);
            if (recPtr != nullptr) {
                recPtr->visible = false;
                Execution* exe = findActiveOrder(id);
                assert(exe != nullptr && "cancelActiveOrder order not found"); //TODO - not required
                sendCancel(exe,"order cancel success"); 
                removeActiveOrder(id);
            } else {
                assert(findActiveOrder(id) == nullptr && "cancelActiveOrder active order not in a side");
                sendReject(o.get(), "order not found");
            }
        }
        void amendActiveOrder(order_id_t id, std::unique_ptr<AmendOrder> o) {
            if (!open_) {
                sendReject(o.get(), "book not open");
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
                        assert(exe != nullptr && "amendActiveOrder price order not found");
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
                        assert(exe != nullptr && "amendActiveOrder price order not found");
                        if ((amended = exe->amend(amend)) == true) {
                            newRec.price = amend.price;
                            // As it's an existing order check whether it is marketable
                            recPtr->visible = false;
                            newRec.visible = true;
                            executeOrder(newRec,std::move(exe));
                        }
                    }
                    if (amended) {
                        sendAmend(o.get());
                    } else {
                        sendReject(o.get(), "Invalid amend of execution order");
                    }
                } else {
                    // Shares
                    Execution* exe = findActiveOrder(id);
                    assert(exe != nullptr && "amendActiveOrder price order not found");
                    if (exe->amend(amend)) {
                        recPtr->shares = amend.shares;
                        sendAmend(o.get());
                    } else {
                        sendReject(o.get(), "Invalid amend of execution order");
                    }
                }
            } else {
                sendReject(o.get(), "unknown order");
            }
        }
        void executeOrder(SideRecord& rec, std::unique_ptr<Execution> exe) {
            if (!open_) {
                sendReject(exe.get(),"book not open");
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
                    assert(false && "marketableOrBook Unknown order_type");
                }
            }           
        }
    private:
        void sendResponse(Message* o, response_t r, text_t t) {
            MatchingEngine::sendResponse(o, r, t);
        }
        void sendTradeReport(Order* o, direction_t d, shares_t s, price_t p) {
            if (bookkeep_) {
                bookkeeper_.trade(d, s, p, std::chrono::steady_clock::now());
            }
            MatchingEngine::sendTradeReport(o,d,s,p);
        }
        void sendCancel(Execution* exe, const text_t& text) {
            if (bookkeep_) {
                bookkeeper_.cancel();
            }
            sendResponse(exe, an::CANCELLED, text); 
        }
        void sendAmend(Order* o) {
            if (bookkeep_) {
                bookkeeper_.amend();
            }
            sendResponse(o, an::COMPLETE,"amend success"); 
        }
        void sendReject(Order* o, const text_t& text) {
            if (bookkeep_) {
                bookkeeper_.reject();
            }
            sendResponse(o, an::REJECT, text); 
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
            return an::sinceToString(time, epoch_);
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
        epoch_t& epoch_;
        bool bookkeep_;
        Bookkeeper bookkeeper_;
}; // Book


inline std::string to_string(const SideRecord& sr, const epoch_t& epoch) {
    std::ostringstream os;
    os << "seq=" << sr.seq << " direction=" << to_string(sr.direction) << " price=" << sr.price << " time=" 
       << sinceToString(sr.time, epoch);
    return os.str();
}

} // an - namespace

#endif

