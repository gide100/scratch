#include "order.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <boost/algorithm/string.hpp>

std::string an::Order::to_string() const {
    std::stringstream ss;
    ss << "id=" << order_id_ ;
    return ss.str();
}

an::Order::~Order() { }

const char DELIMITOR = ':';
const char SEPERATOR = '=';

std::string::const_iterator mySplit(std::pair<std::string,std::string>& res, 
                                    std::string::const_iterator myBegin, std::string::const_iterator myEnd) {
    auto myDelim=std::find(myBegin, myEnd, DELIMITOR);
    auto myEq=std::find(myBegin, myDelim, SEPERATOR);
    if (myEq==myDelim) {
         std::string token(myBegin, myDelim);
         std::stringstream ss;
         ss << "Bad token missing seperator [" << token << "]";
         throw an::OrderError(ss.str().c_str());
    }
    std::string first(myBegin,myEq);
    std::string second(++myEq,myDelim);

    if (myDelim != myEnd) {
        ++myDelim;
    }
    res.first = first; res.second=second;
    return myDelim;
}

enum InputField { None, Type, Id, Symbol, Direction, Shares, Price, Last }; 
typedef std::bitset<Last> FieldFlags;
void checkFlags(FieldFlags expected, FieldFlags got) {
    if (expected != got) {
        std::stringstream ss;
        ss << "Invalid flags got " << got << " excepted " << expected;
        throw an::OrderError(ss.str());
    }
}

an::Order* an::Order::makeOrder(const std::string& input) {
    auto myEnd = input.cend();
    auto myBegin = input.cbegin();
    std::pair<std::string,std::string> res;

    FieldFlags myFlags;
    FieldFlags myPrevFlags;
    std::string myType;
    an::order_id_t myId = 0;
    an::symbol_t mySymbol;
    an::direction_t myDirection = an::BUY;
    an::price_t myPrice = 0.0;
    an::number_shares_t myShares = 0;
    bool used = false;
    while(myBegin != myEnd) {
        used = false;
        auto myDelim = mySplit(res, myBegin, myEnd);
        myBegin = myDelim; 
        std::cout << "TEST " << res.first << "," << res.second << std::endl;
        if (res.first == "type") {
            myType = res.second;
            myFlags.set(Type);
	    used = true;
        }
        if (res.first == "id") {
            myId = std::stoull(res.second);
            myFlags.set(Id);
	    used = true;
        }
        if (res.first == "symbol") {
            mySymbol = res.second;
            myFlags.set(Symbol);
	    used = true;
        }
        if (res.first == "direction") {
            if (res.second == "BUY") {
                myDirection = an::BUY;
                myFlags.set(Direction);
		used = true;
            } else if (res.second == "SELL") {
                myDirection = an::SELL;
                myFlags.set(Direction);
		used = true;
            } else {
                std::stringstream ss; 
                ss << "Invalid direction [" << res.second << "]";
                throw OrderError(ss.str());
            }
        }
        if (res.first == "shares") {
            myShares = std::stoul(res.second);
            myFlags.set(Shares);
	    used = true;
        }
        if (res.first == "price") {
            myPrice = std::stod(res.second);
            myFlags.set(Price);
	    used = true;
        }
        if (!used) {
            std::stringstream ss; 
            ss << "Unused token [" << res.first << ',' << res.second << "]";
            throw OrderError(ss.str());
	}
        if (myFlags == myPrevFlags) {
            std::stringstream ss; 
            ss << "Repeated token [" << res.first << ',' << res.second << "]";
            throw OrderError(ss.str());
	}
        myPrevFlags = myFlags;
    }

    Order* myOrder = nullptr;
    if (myType == "LIMIT") {
        FieldFlags f; f.set(Type), f.set(Id); f.set(Symbol); f.set(Direction); f.set(Shares); f.set(Price);
        checkFlags(f, myFlags);
        an::LimitOrder* o = new an::LimitOrder(myId, mySymbol, myDirection, myShares, myPrice);
        myOrder = o;
    } else if (myType == "MARKET") {
        FieldFlags f; f.set(Type), f.set(Id); f.set(Symbol); f.set(Direction); f.set(Shares);
        checkFlags(f, myFlags);
        an::MarketOrder* o = new an::MarketOrder(myId, mySymbol, myDirection, myShares);
        myOrder = o;
    } else if (myType == "CANCEL") {
        FieldFlags f; f.set(Type), f.set(Id);
        checkFlags(f, myFlags);
        an::CancelOrder* o = new an::CancelOrder(myId);
        myOrder = o;
    } else if (myType == "AMEND") {
        FieldFlags f; f.set(Type), f.set(Id);
        FieldFlags f1(myFlags); f1.reset(Price); f1.reset(Shares);
        checkFlags(f, f1);
        an::AmendOrder* o = nullptr;
        if (myFlags[Price]) {
            o = new an::AmendOrder(myId, myPrice);
        } else if (myFlags[Shares]) {
            o = new an::AmendOrder(myId, myShares);
        } else {
            throw OrderError("Invalid amend (non given)");
        }
        myOrder = o;
    } else {
        std::stringstream ss; 
        ss << "Invalid order type [" << myType << "]";
        throw OrderError(ss.str());
    }
         
    return myOrder;
}


std::string an::Execution::to_string() const {
    std::stringstream ss;
    ss << Order::to_string() << DELIMITOR << "symbol" << SEPERATOR << symbol_ << DELIMITOR 
       << "direction" << SEPERATOR << ((direction_==BUY) ? "BUY" : "SELL") << DELIMITOR << "shares" << SEPERATOR << shares_;
    return ss.str();
}

an::Execution::~Execution() { }


std::string an::LimitOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "LIMIT" << DELIMITOR << Execution::to_string() << DELIMITOR << "price" << SEPERATOR  << price_;
    return ss.str();
}
an::LimitOrder::~LimitOrder() { }


std::string an::MarketOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "MARKET" << DELIMITOR << Execution::to_string() ;
    return ss.str();
}
an::MarketOrder::~MarketOrder() { }

std::string an::CancelOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "CANCEL" << DELIMITOR << Order::to_string() ;
    return ss.str();
}

an::CancelOrder::~CancelOrder() { }

std::string an::AmendOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "AMEND" << DELIMITOR << Order::to_string();
    if (amend_.field != NONE) {
       ss << DELIMITOR << amend_.get_field_name() << SEPERATOR ;
        switch (amend_.field) {
            case NONE: ss << "none"; break;
            case PRICE: ss << amend_.price; break;
            case SHARES: ss << amend_.shares; break;
            default:
                ss << "unknown:field_t";
        }
    }
    return ss.str();
}

an::AmendOrder::~AmendOrder() { }


int main() {
    an::MarketOrder ord1(10,"APPL",an::BUY,10);
    an::LimitOrder ord2(11,"IBM",an::SELL,10,5.12);
    an::CancelOrder ord3(10);
    an::AmendOrder ord4(11);
    an::AmendOrder ord5(11, 123.45);
    an::AmendOrder ord6(11, an::number_shares_t(20));
    std::cout << ord1 << std::endl;
    std::cout << ord2 << std::endl;
    std::cout << ord3 << std::endl;
    std::cout << ord4 << std::endl;
    std::cout << ord5 << std::endl;
    std::cout << ord6 << std::endl;
    try {
        an::Order* o10 = an::Order::makeOrder("type=LIMIT:id=123:symbol=MSFT:direction=BUY:shares=50:price=92.0");
	std::cout << *o10 << std::endl;
        an::Order* o11 = an::Order::makeOrder("type=MARKET:id=123:symbol=MSFT:direction=SELL:shares=25");
	std::cout << *o11 << std::endl;
        an::Order* o12 = an::Order::makeOrder("type=AMEND:id=123:shares=30");
	std::cout << *o12 << std::endl;
        an::Order* o13 = an::Order::makeOrder("type=CANCEL:id=123");
	std::cout << *o13 << std::endl;
    } catch(an::OrderError& e) {
       std::cout << "ERROR " << e.what() << std::endl;
    }
    return 0;
}
