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
        OrderError(const std::string& msg) : runtime_error(msg) {}
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

class Order {
    public:
        Order(order_id_t id, location_t origin, location_t dest = ME) 
             : order_id_(id), origin_(origin), destination_(dest) { }
        //friend std::ostream& operator<<(std::ostream& os, const Order& o) ;
        virtual std::string to_string() const = 0;

        virtual ~Order() = 0; 
        
        // Factory and string parser
        static Order* makeOrder(const std::string& input);
    private:
        order_id_t order_id_;
        location_t origin_;
        location_t destination_;
        
};


class Execution : public Order {
    public:
        Execution(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s) : Order(id, o, dest), symbol_(sym), direction_(d), shares_(s) { }
        virtual std::string to_string() const = 0;
        virtual ~Execution() = 0;
    private:
        symbol_t symbol_;
        direction_t direction_;
        shares_t shares_;
};

class LimitOrder : public Execution {
    public:
        LimitOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s, price_t p) : Execution(id, o, dest, sym, d, s), price_(p) {}
        virtual std::string to_string() const;
        virtual ~LimitOrder() ;   
    private:
        price_t price_;
};


class MarketOrder : public Execution {
    public:
        MarketOrder(order_id_t id, location_t o, location_t dest, symbol_t sym, direction_t d, shares_t s) : Execution(id,o,dest,sym,d,s) {}
        virtual std::string to_string() const;
        virtual ~MarketOrder() ;
    private:
};

class CancelOrder : public Order {
    public:
        CancelOrder(order_id_t id, location_t o, location_t dest) : Order(id, o, dest) {}
        virtual std::string to_string() const;
        virtual ~CancelOrder() ;
    private:
};

class AmendOrder : public Order {
    public:
        explicit AmendOrder(order_id_t id, location_t o, location_t dest) : Order(id, o , dest) { amend_.field = NONE; amend_.price=0.0; }
        AmendOrder(order_id_t id, location_t o, location_t dest, price_t p) : Order(id, o, dest), amend_({PRICE, {.price = p}}) { }
        AmendOrder(order_id_t id, location_t o, location_t dest, shares_t s) : Order(id, o, dest) {
            amend_.field = SHARES; amend_.shares = s;
        }
        virtual std::string to_string() const;
        virtual ~AmendOrder() ;
    private:
      amend_t amend_;
      
};



} // an namespace

std::ostream& operator<<(std::ostream& os, const an::Order& o);




#endif

