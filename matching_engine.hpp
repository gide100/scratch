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
        MatchingEngine(const an::location_t& exchange, SecurityDatabase& secdb,
                       bool bookkeep=true);
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
    std::string to_string(const epoch_t& epoch) const {
        std::ostringstream os;
        os << "seq=" << seq << " direction=" << an::to_string(direction) << " price=" << price << " time="
           << an::sinceToString(time, epoch);
        return os.str();
    }
    order_id_t  id;
    sequence_t  seq;
    since_t     time;
    order_t     order_type;
    direction_t direction;
    price_t     price;
    shares_t    shares;
    bool        visible;
    bool        on_book;
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
            result = lhs.price < rhs.price; // Descending
        } else if (!one_list_ && (lhs.price != rhs.price)) {
            result = ((lhs.direction==an::BUY)  && lhs.price < rhs.price) ||
                     ((lhs.direction==an::SELL) && lhs.price > rhs.price) ;
        } else if (one_list_ && (lhs.time != rhs.time)) {
            result = ((lhs.direction==an::BUY)  && (lhs.time > rhs.time)) || // Ascending times
                     ((lhs.direction==an::SELL) && (lhs.time < rhs.time)) ;  // Descending times
        } else if (!one_list_ && (lhs.time != rhs.time)) {
            result = lhs.time > rhs.time; // Ascending times
        } else {
            result = lhs.seq > rhs.seq; // Ascending sequence
        }
        return result;
    }

    bool one_list_; // Output in one list
};

static const SideRecord DefaultSideRecord =
   { .id=0, .seq=0, .time=since_t(), .order_type=an::LIMIT,
     .direction=an::BUY, .price=0.0, .shares=0, .visible=false, .on_book=false };



struct side_stat_t {
    side_stat_t() : trades(0), shares(0), value(0.0), volume(0.0) { }
    counter_t         trades; // Active trades
    shares_t          shares; // Total number of shares in active trades
    volume_t          value;  // Sum product of the active shares and prices
    volume_t          volume; // Cumulative value of shares/prices added to book
};

struct bookkeeper_stats_t {
    bookkeeper_stats_t() :
              shares_traded(0), volume(0.0), trades(0), cancels(0), amends(0), rejects(0),
              daily_high(0.0), daily_low(0.0),
              open_price(0.0), close_price(0.0),
              avg_share_price(0.0), last_trade_price(0.0), last_trade_time() {
    }
    shares_t        shares_traded;
    volume_t        volume;
    counter_t       trades; // If zero no trading, last_trade_time_ invalid, etc
    counter_t       cancels;
    counter_t       amends;
    counter_t       rejects;
    side_stat_t     buy;
    side_stat_t     sell;
    price_t         daily_high;
    price_t         daily_low;
    price_t         open_price;
    price_t         close_price;
    price_t         avg_share_price;
    price_t         last_trade_price;
    since_t         last_trade_time;
};

class Bookkeeper {
    public:
        Bookkeeper(date_t date, price_t previous_close, const epoch_t& epoch)
            : trading_date_(date), epoch_(epoch), previous_close_(previous_close), cmp_(false), bks_() {
        }
        Bookkeeper(const Bookkeeper& bk) = default;
        ~Bookkeeper() { }

        const bookkeeper_stats_t& stats() const {
            return bks_;
        }

        // Orders
        void trade(direction_t direction, shares_t shares, price_t price, since_t since) {
            bks_.shares_traded += shares;
            bks_.volume += price * shares;
            if (bks_.trades == 0) {
                bks_.daily_high = price;
                bks_.daily_low = price;
                bks_.open_price = price;
            } else {
                bks_.daily_high = std::max(bks_.daily_high,price);
                bks_.daily_low = std::min(bks_.daily_low,price);
            }
            ++bks_.trades;
            bks_.last_trade_price = price;
            bks_.last_trade_time = since;
        }
        void cancel() {
            ++bks_.cancels;
        }
        void amend() {
            ++bks_.amends;
        }
        void reject() {
            ++bks_.rejects;
        }

        // Side
        void addSide(const SideRecord& side) {
            side_stat_t* s = &bks_.sell;
            if (side.direction == BUY) {
                s = &bks_.buy;
            }
            ++s->trades;
            s->shares += side.shares;
            s->value += side.shares * side.price;
            s->volume += side.shares * side.price;
        }
        // Re-add volume of amended records
        void addSideVolume(const SideRecord& side) {
            side_stat_t* s = &bks_.sell;
            if (side.direction == BUY) {
                s = &bks_.buy;
            }
            s->volume += side.shares * side.price;
        }
        void removeSide(const SideRecord& side, bool removeVolume=false) {
            side_stat_t* s = &bks_.sell;
            if (side.direction == BUY) {
                s = &bks_.buy;
            }
            --s->trades;
            s->shares -= side.shares;
            s->value -= side.shares * side.price;
            if (removeVolume) {
                s->volume -= side.shares * side.price;
            }
        }
        void amendSide(const SideRecord& side, shares_t oldShares) {
            side_stat_t* s = &bks_.sell;
            if (side.direction == BUY) {
                s = &bks_.buy;
            }
            s->shares += (side.shares - oldShares);
            s->value  += (side.shares - oldShares) * side.price;
            s->volume += (side.shares - oldShares) * side.price;
        }

        void close() {
            bks_.close_price = bks_.last_trade_price;
            if (bks_.trades !=0) {
                bks_.avg_share_price = bks_.volume / bks_.shares_traded;
            }
        }
        std::string to_string(bool verbose = false) {
            std::ostringstream os;
            os << dateToString(trading_date_) << " prev=" << previous_close_
               << " shares=" << bks_.shares_traded << " volume=" << bks_.volume
               << " trades=" << bks_.trades << " cancels=" << bks_.cancels
               << " amends=" << bks_.amends << " rejects=" << bks_.rejects;
            if (bks_.trades != 0) {
                os << " high=" << bks_.daily_high << " low=" << bks_.daily_low
                   << " open=" << bks_.open_price << " close=" << bks_.close_price
                   << " avg=" << bks_.avg_share_price
                   << " last_price=" << bks_.last_trade_price
                   << " last_time=" << sinceToString(bks_.last_trade_time,epoch_) ;
            } else {
                os << " high=N/A low=N/A open=N/A close=N/A avg=N/A last_price=N/A last_time=N/A" ;
            }
            os << " buy(" << bks_.buy.shares << "," << bks_.buy.value << ","
               << bks_.buy.value << "," << bks_.buy.volume << ")"
               << " sell(" << bks_.sell.shares << "," << bks_.sell.value << ","
               << bks_.sell.value << "," << bks_.sell.volume << ")";
            return os.str();
        }
    private:
        date_t             trading_date_;
        const epoch_t&     epoch_;
        price_t            previous_close_;
        CompareSideRecord  cmp_;
        bookkeeper_stats_t bks_;
};


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

class Side {
    public:
        Side(Bookkeeper& bookkeeper, direction_t direction, const epoch_t& epoch) 
            : bookkeeper_(bookkeeper), direction_(direction), epoch_(epoch) {}
        Side(const Side& s) : q_(s.q_), bookkeeper_(s.bookkeeper_), 
            direction_(s.direction_), epoch_(s.epoch_) {}
        ~Side() {}

        std::string to_string(bool verbose) const;

        void add(SideRecord& rec) {
            assert(rec.visible && "Side.add record not visible");
            assert(rec.direction == direction_ && "Side.add wrong direction");
            rec.on_book = true;
            q_.push(rec);
            bookkeeper_.addSide(rec);
        }
        void addVolume(SideRecord& rec) {
            bookkeeper_.addSideVolume(rec);
        }
        void remove(SideRecord& rec, bool removeVolume = false) {
            assert(rec.direction == direction_ && "Side.remove wrong direction");
            rec.visible = false;
            normalise();
            bookkeeper_.removeSide(rec, removeVolume);
        }

        void removeTop( bool removeVolume = false ) {
            assert(!q_.empty() && "removeTop from empty queue");
            assert(q_.begin()->id == top().id && "removeTop first vector element and top same"),
            remove(*q_.begin(),removeVolume); // front()
        }

        void amendShares(SideRecord& rec, shares_t oldShares) {
            assert(rec.direction == direction_ && "Side.amend wrong direction");
            bookkeeper_.amendSide(rec, oldShares);
        }
        void amendSharesTop(shares_t diffShares) {
            assert(!q_.empty() && "amendSharesTop from empty queue");
            SideRecord& rec = *q_.begin(); // front()
            shares_t oldShares = rec.shares;
            rec.shares += diffShares;
            amendShares(rec, oldShares);
        }

        bool empty() const {
            return q_.empty();
        }

        void pop() {
            q_.pop();
            normalise();
        }

        SideRecord top() {
            return q_.top();
        }

        struct MatchOrderId { // Functor
            explicit MatchOrderId(an::order_id_t order_id) : order_id_(order_id) { }
            bool operator()(const an::SideRecord& rec) const { return rec.id == order_id_; }
            an::order_id_t order_id_;
        };
        SideRecord* findRecord(order_id_t id) {
            SideRecord* found = nullptr;
            auto it = std::find_if(q_.begin(), q_.end(), MatchOrderId(id));
            if (it != q_.end()) {
                found = &(*it);
            }
            return found;
        }
        const auto& q() const {
            return q_;
        }
        using container_type = std::deque<SideRecord>;
        using value_compare = CompareSideRecord;
        using Q = PriorityQueue<SideRecord, container_type, value_compare > ;
    private:
        // Remove non-visible elements from top
        void normalise() {
            while (!q_.empty()) {
                const SideRecord& top = q_.top();
                if(!top.visible) {
                    q_.pop();
                } else {
                    break;
                }
            }
        }
        Q q_;
        Bookkeeper& bookkeeper_;
        direction_t direction_;
        const epoch_t& epoch_;
};


class TickTable ;

class Book {
    public:
        explicit Book(symbol_t sym, epoch_t& epoch, TickTable& tt, bool bookkeep, price_t closing_price)
            : symbol_(sym), open_(false), active_order_(), epoch_(epoch), tick_table_(tt), 
              bookkeep_(bookkeep), bookkeeper_(
                std::chrono::system_clock::to_time_t(date::floor<date::days>(std::chrono::system_clock::now())),
                closing_price, epoch_), 
              buy_(bookkeeper_, an::BUY, epoch_), sell_(bookkeeper_, an::SELL, epoch_) {
        }

        Book(const Book& book)
            : symbol_(book.symbol_), epoch_(book.epoch_), tick_table_(book.tick_table_),
              bookkeep_(book.bookkeep_), bookkeeper_(book.bookkeeper_), buy_(book.buy_), sell_(book.sell_) {
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

        // Live orders, used for lookups
        void addActiveOrder(order_id_t id, std::unique_ptr<Execution> o, direction_t d) {
            active_order_.emplace(id, std::move(open_order{id, std::move(o), d}));
        }

        void addSideRecord(SideRecord& rec) {
            assert(rec.visible && "addSideRecord non-visible record added");

            // Add to queue
            if (rec.direction == an::BUY) {
                 buy_.add(rec);
            } else if (rec.direction == an::SELL) {
                 sell_.add(rec);
            } else {
                assert(false && "addSideRecord unknown direction"); // Unknown direction
            }
        }

        void cancelActiveOrder(order_id_t id, std::unique_ptr<CancelOrder> o) {
            if (!open_) {
                sendReject(o.get(), "book not open");
                return;
            }
            SideRecord* recPtr = findSideRecord(id);
            if (recPtr != nullptr) {
                if (recPtr->direction == an::BUY) {
                    buy_.remove(*recPtr);
                } else {
                    sell_.remove(*recPtr);
                }
                assert(recPtr->visible == false && "cancelActiveOrder non-visible");
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

                    if ((amended = exe->amend(amend)) == true) {
                        Side* s = &sell_;
                        if (newRec.direction == BUY) {
                            s = &buy_;
                        }

                        s->remove(*recPtr, true);
                        assert(recPtr->visible == false && "amendActiveOrder non-visible");
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
            if (findActiveOrder(rec.id) != nullptr) {
                sendReject(exe.get(), "Order id already on book");
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

        const Bookkeeper& bookkeeper() const {
            return bookkeeper_;
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

        // Can the new order (execution) be satisfied without adding it to the book
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

        SideRecord* findSideRecord(order_id_t id) {
            SideRecord* found = nullptr;
            auto search = active_order_.find(id);
            if (search != active_order_.end()) {
                if (search->second.direction == an::BUY) {
                    found = buy_.findRecord(id);
                } else {
                    found = sell_.findRecord(id);
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
        //typedef PriorityQueue<SideRecord, std::deque<SideRecord>, CompareSideRecord > Side;

        bool marketableSide(Side& side, SideRecord& newRec, Execution* newExe);

        symbol_t symbol_;
        bool open_;
        active_order_t active_order_;
        epoch_t& epoch_;
        TickTable& tick_table_;
        bool bookkeep_;
        Bookkeeper bookkeeper_;
        Side buy_;
        Side sell_;
}; // Book


} // an - namespace

#endif

