#ifndef AN_ORDER_HPP
#define AN_ORDER_HPP

#include <iostream>
#include <sstream>
#include <exception>

namespace an {

using std::runtime_error; 
class OrderError : public runtime_error {
    public:
        OrderError(const std::string& msg) : runtime_error(msg) {}
};
 
typedef std::uint64_t order_id_t; 
typedef std::uint32_t number_shares_t; 
typedef double price_t; 
typedef enum { BUY, SELL } direction_t;
typedef enum { NONE, PRICE, SHARES } field_t;
struct amend_t {
    field_t field;
    union {
        price_t price;
        number_shares_t shares;
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

typedef std::string symbol_t;

class Order {
    public:
        explicit Order(order_id_t id) : order_id_(id) { }
        friend std::ostream& operator<<(std::ostream& os, const Order& o) ;
        virtual std::string to_string() const = 0;

        virtual ~Order() = 0; 
        
        // Factory and string parser
        static Order* makeOrder(const std::string& input);
    private:
        order_id_t order_id_;
};

std::ostream& operator<<(std::ostream& os, const Order& o) {
    return os << o.to_string() ;
}

class Execution : public Order {
    public:
        Execution(order_id_t id, symbol_t sym, direction_t d, number_shares_t s) : Order(id), symbol_(sym), direction_(d), shares_(s) { }
        virtual std::string to_string() const = 0;
        virtual ~Execution() = 0;
    private:
        symbol_t symbol_;
        direction_t direction_;
        number_shares_t shares_;
};

class LimitOrder : public Execution {
    public:
        LimitOrder(order_id_t id, symbol_t sym, direction_t d, number_shares_t s, price_t p) : Execution(id, sym, d, s), price_(p) {}
        virtual std::string to_string() const;
        virtual ~LimitOrder() ;   
    private:
        price_t price_;
};


class MarketOrder : public Execution {
    public:
        MarketOrder(order_id_t id, symbol_t sym, direction_t d, number_shares_t s) : Execution(id,sym,d,s) {}
        virtual std::string to_string() const;
        virtual ~MarketOrder() ;
    private:
};

class CancelOrder : public Order {
    public:
        CancelOrder(order_id_t id) : Order(id) {}
        virtual std::string to_string() const;
        virtual ~CancelOrder() ;
    private:
};

class AmendOrder : public Order {
    public:
        explicit AmendOrder(order_id_t id) : Order(id) { amend_.field = NONE; amend_.price=0.0; }
        AmendOrder(order_id_t id, price_t p) : Order(id), amend_({PRICE, {.price = p}}) { }
        AmendOrder(order_id_t id, number_shares_t s) : Order(id) {
            amend_.field = SHARES; amend_.shares = s;
        }
        virtual std::string to_string() const;
        virtual ~AmendOrder() ;
    private:
      amend_t amend_;
      
};

} // an namespace

#endif

