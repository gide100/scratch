#include "order.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <boost/algorithm/string.hpp>

const char DELIMITOR = ':';
const char SEPERATOR = '=';

std::string an::Order::to_string() const {
    std::stringstream ss;
    ss << "id" << SEPERATOR << order_id_ << DELIMITOR  
       << "origin" << SEPERATOR << origin_ << DELIMITOR
       << "destination" << SEPERATOR << destination_ ;
    return ss.str();
}

an::Order::~Order() { }

std::ostream& operator<<(std::ostream& os, const an::Order& o) {
    return os << o.to_string() ;
}




std::string::const_iterator mySplit(std::pair<std::string,std::string>& res, 
                                    std::string::const_iterator myBegin, std::string::const_iterator myEnd) {
    auto myDelim=std::find(myBegin, myEnd, DELIMITOR); // one=a or one=a:two=b myDelim [end] or one=a[:]
    auto myEq=std::find(myBegin, myDelim, SEPERATOR); // myEq= one[=]
    if (myEq==myDelim) { // Not found, end
         std::string token(myBegin, myDelim);
         std::stringstream ss;
         ss << "Bad token missing seperator ("<<SEPERATOR<<") [" << token << "]";
         throw an::OrderError(ss.str().c_str());
    }
    res.first.assign(myBegin,myEq); //one
    res.second.assign(++myEq,myDelim); //a  

    if (myDelim != myEnd) { //one=a[:]
        ++myDelim; // [t]wo=b
    }
    return myDelim;
}

enum InputField { None, Type, Id, Origin, Destination, Symbol, Direction, Shares, Price, Last }; 
typedef std::bitset<Last> FieldFlags;
void checkFlags(FieldFlags expected, FieldFlags got) {
    if (expected != got) {
        std::stringstream ss;
        ss << "Invalid flags got " << got << " expected " << expected;
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
    an::location_t myOrigin;
    an::location_t myDestination;
    an::symbol_t mySymbol;
    an::direction_t myDirection = an::BUY;
    an::price_t myPrice = 0.0;
    an::number_shares_t myShares = 0;
    bool used = false;
    while(myBegin != myEnd) {
        used = false;
        auto myDelim = mySplit(res, myBegin, myEnd);
        myBegin = myDelim; 
        // std::cout << "MarkOrder: " << res.first << "," << res.second << std::endl;
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
        if (res.first == "origin") {
            myOrigin = res.second;
            myFlags.set(Origin);
	    used = true;
        }
        if (res.first == "destination") {
            myDestination = res.second;
            myFlags.set(Destination);
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
    FieldFlags f; f.set(Type); f.set(Id); f.set(Origin); f.set(Destination); 
    if (myType == "LIMIT") {
        f.set(Symbol); f.set(Direction); f.set(Shares); f.set(Price);
        checkFlags(f, myFlags);
        an::LimitOrder* o = new an::LimitOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares, myPrice);
        myOrder = o;
    } else if (myType == "MARKET") {
        f.set(Symbol); f.set(Direction); f.set(Shares);
        checkFlags(f, myFlags);
        an::MarketOrder* o = new an::MarketOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares);
        myOrder = o;
    } else if (myType == "CANCEL") {
        checkFlags(f, myFlags);
        an::CancelOrder* o = new an::CancelOrder(myId, myOrigin, myDestination);
        myOrder = o;
    } else if (myType == "AMEND") {
        FieldFlags f1(myFlags); f1.reset(Price); f1.reset(Shares);
        checkFlags(f, f1);
        an::AmendOrder* o = nullptr;
        if (myFlags[Price]) {
            o = new an::AmendOrder(myId, myOrigin, myDestination, myPrice);
        } else if (myFlags[Shares]) {
            o = new an::AmendOrder(myId, myOrigin, myDestination, myShares);
        } else {
            throw OrderError("Invalid amend (none given)");
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

