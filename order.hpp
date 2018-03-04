#ifndef AN_ORDER_HPP
#define AN_ORDER_HPP

#include <iostream>
#include <sstream>
#include <exception>
#include "types.hpp"

namespace an {

using std::runtime_error; 
class OrderError : public runtime_error {
    public:
        OrderError(const std::string& msg) : runtime_error(msg) { }
};
 
typedef enum { NONE, PRICE, SHARES } field_t;
struct amend_t {
    field_t field;
    union {
        price_t price;
        shares_t shares;
    };

    const char* get_field_name() const {
        switch (field) {
          case NONE: return "none"; break;
          case PRICE: return "price"; break;
          case SHARES: return "shares"; break;
          default:
            return "unknown:field_t";
        }
    }    
};



class MatchingEngine ;
struct SideRecord ;

class Message {
    public:
        Message(location_t origin, location_t dest = ME) 
            : origin_(origin), destination_(dest), reverse_direction_(false) {}

        virtual std::string to_string() const = 0;
        virtual ~Message() = 0; 
        
        // Factory and string parser
        static Message* makeOrder(const std::string& input);

        const location_t& origin() const { return origin_; }
        const location_t& destination() const { return destination_; }

        void reverse_direction() { reverse_direction_ = !reverse_direction_; }
    protected:
        location_t origin_;
        location_t destination_;
        bool reverse_direction_;
};

class Login : public Message {
    public:
        Login(location_t origin, location_t dest = ME) : Message(origin, dest) { }
        virtual std::string to_string() const;

        virtual ~Login(); 
};


class Order : public Message {
    public:
        Order(order_id_t id, location_t origin, location_t dest, symbol_t sym) 
             : Message(origin, dest), order_id_(id), symbol_(sym) //, origin_(origin), destination_(dest) 
             { }

        virtual std::string to_string() const = 0;
        virtual ~Order() = 0; 
        virtual void applyOrder(MatchingEngine& me) = 0;

        order_id_t orderId() const { return order_id_; }
        const symbol_t& symbol() const { return symbol_; }
    protected:
        order_id_t order_id_;
        symbol_t symbol_;
        
};


class Execution : public Order {
    public:
        Execution(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s) 
            : Order(id, o, dest, sym), direction_(d), shares_(s) { }
        virtual std::string to_string() const = 0;
        virtual ~Execution() = 0;

        virtual void applyOrder(MatchingEngine& me) = 0;
        virtual void pack(SideRecord& rec) const = 0;
    protected:
        direction_t direction_;
        shares_t shares_;
};

class LimitOrder : public Execution {
    public:
        LimitOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s, price_t p) 
            : Execution(id, o, dest, sym, d, s), price_(p) {}

        virtual std::string to_string() const;
        virtual ~LimitOrder() ;   
        virtual void applyOrder(MatchingEngine& me) ;

        virtual void pack(SideRecord& rec) const ;
    protected:
        price_t price_;
};


class MarketOrder : public Execution {
    public:
        MarketOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s) 
            : Execution(id,o,dest,sym,d,s) {}

        virtual std::string to_string() const;
        virtual ~MarketOrder() ;
        virtual void applyOrder(MatchingEngine& me) ;

        virtual void pack(SideRecord& rec) const ;
    protected:
};

class CancelOrder : public Order {
    public:
        CancelOrder(order_id_t id, location_t o, location_t dest, symbol_t sym) : Order(id, o, dest, sym) {}

        virtual std::string to_string() const;
        virtual ~CancelOrder() ;
        virtual void applyOrder(MatchingEngine& me) ;
    protected:
};

class AmendOrder : public Order {
    public:
        explicit AmendOrder(order_id_t id, location_t o, location_t dest, symbol_t sym) 
            : Order(id, o, dest, sym) { 
                amend_.field = NONE; amend_.price=0.0; 
        }
        AmendOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, price_t p) 
            : Order(id, o, dest, sym), amend_({PRICE, {.price = p}}) { }
        AmendOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, shares_t s) 
            : Order(id, o, dest, sym) {
            amend_.field = SHARES; amend_.shares = s;
        }

        virtual std::string to_string() const;
        virtual ~AmendOrder() ;
        virtual void applyOrder(MatchingEngine& me) ;

        an::amend_t& amend() { return amend_; }
    protected:
        amend_t amend_;
      
};

// Decorator to responde to messages
class Response : public Message {
    public:
        explicit Response(Message* m, response_t response=ERROR, text_t text = "")
             : Message("",""), message_(m), response_(response), text_(text) {
            if (m == nullptr) {
                throw OrderError("nullptr in Response Message");
            }
            if ((text_.find(':')!=text_t::npos) || (text_.find('=')!=text_t::npos)) {
                throw OrderError("Cannot have [:|=] in Response text");
            }
            m->reverse_direction();
        }

        virtual std::string to_string() const;
        virtual ~Response();
    protected:
        Message* message_;
        response_t response_;
        text_t text_;
};

class TradeReport : public Message {
    public:
        explicit TradeReport(Order* o, direction_t d, shares_t s, price_t p) 
            : Message(o->destination(), o->origin()), orig_order_id_(o->orderId()), symbol_(o->symbol()), 
              direction_(d), shares_(s), price_(p) {
        } 

        virtual std::string to_string() const;
        virtual ~TradeReport();
    protected:
        order_id_t orig_order_id_; 
        symbol_t symbol_;
        direction_t direction_;
        shares_t shares_;
        price_t price_;
};



} // an namespace

std::ostream& operator<<(std::ostream& os, const an::Message& msg);

#endif

