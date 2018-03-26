#include "order.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <boost/algorithm/string.hpp>

const char DELIMITOR = ':';
const char SEPERATOR = '=';

std::string an::Message::to_string() const {
    std::ostringstream os;
    os << "origin" << SEPERATOR << (reverse_direction_ ? destination_ : origin_ )  << DELIMITOR
       << "destination" << SEPERATOR << (reverse_direction_ ? origin_ : destination_ ) ;
    return os.str();
}
an::Message::~Message() { }



std::string an::Login::to_string() const {
    std::stringstream os;
    os << "type" << SEPERATOR << "LOGIN" << DELIMITOR
       << Message::to_string();
    return os.str();
}

an::Login::~Login() { }

std::string an::Order::to_string() const {
    std::stringstream ss;
    ss << "id" << SEPERATOR << order_id_ << DELIMITOR
       << Message::to_string() << DELIMITOR
       << "symbol" << SEPERATOR << symbol_ ;
    return ss.str();
}

an::Order::~Order() { }

std::ostream& operator<<(std::ostream& os, const an::Message& msg) {
    return os << msg.to_string() ;
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

an::Message* an::Message::makeOrder(const std::string& input) {
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
    an::shares_t myShares = 0;
    bool used = false;
    while(myBegin != myEnd) {
        used = false;
        auto myDelim = mySplit(res, myBegin, myEnd);
        myBegin = myDelim;
        //std::cout << "MarkOrder: " << res.first << "," << res.second << std::endl;
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
            char* stop;
            long myLong = std::strtol(res.second.c_str(), &stop, 10);
            if (*stop != '\0') {
                throw OrderError("Invalid shares could not convert to long");
            }
            if ((myLong > (long)an::MAX_OUTSTANDING_SHARES) || (myLong < 0)) {
                throw OrderError("Invalid shares - out of range");
            }
            myShares = myLong;
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

    Message* myOrder = nullptr;
    FieldFlags f; f.set(Type); f.set(Id); f.set(Origin); f.set(Destination); f.set(Symbol);
    if (myType == "LIMIT") {
        f.set(Direction); f.set(Shares); f.set(Price);
        checkFlags(f, myFlags);
        an::LimitOrder* o = new an::LimitOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares, myPrice);
        myOrder = o;
    } else if (myType == "MARKET") {
        f.set(Direction); f.set(Shares);
        checkFlags(f, myFlags);
        an::MarketOrder* o = new an::MarketOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares);
        myOrder = o;
    } else if (myType == "CANCEL") {
        checkFlags(f, myFlags);
        an::CancelOrder* o = new an::CancelOrder(myId, myOrigin, myDestination, mySymbol);
        myOrder = o;
    } else if (myType == "AMEND") {
        FieldFlags f1(myFlags); f1.reset(Price); f1.reset(Shares);
        checkFlags(f, f1);
        an::AmendOrder* o = nullptr;
        if (myFlags[Price]) {
            o = new an::AmendOrder(myId, myOrigin, myDestination, mySymbol, myPrice);
        } else if (myFlags[Shares]) {
            if (myShares <= 0) {
                throw OrderError("Invalid amend number of shares too small");
            }
            o = new an::AmendOrder(myId, myOrigin, myDestination, mySymbol, myShares);
        } else {
            throw OrderError("Invalid amend (none given)");
        }
        myOrder = o;
    } else if (myType == "LOGIN") {
        FieldFlags f1; f1.set(Type); f1.set(Origin); f1.set(Destination);
        checkFlags(f1, myFlags);
        an::Login* o = new an::Login(myOrigin, myDestination);
        myOrder = o;
    } else {
        std::stringstream ss;
        ss << "Invalid order type [" << myType << "]";
        throw OrderError(ss.str());
    }

    if ((myType != "LOGIN") && (myId == 0)) {
        throw OrderError("Invalid id set to 0");
    }

    return myOrder;
}


void an::Execution::pack(an::SideRecord& rec) const {
    rec = DefaultSideRecord;
    rec.id = order_id_;
    rec.direction = direction_;
    rec.shares = shares_;
}

std::string an::Execution::to_string() const {
    std::stringstream ss;
    ss << Order::to_string() << DELIMITOR
       << "direction" << SEPERATOR << an::to_string(direction_) << DELIMITOR
       << "shares" << SEPERATOR << shares_;
    return ss.str();
}

an::Execution::~Execution() { }


void an::LimitOrder::applyOrder(MatchingEngine& me) {
    me.applyOrder(std::move(std::unique_ptr<LimitOrder>(this)));
}

void an::LimitOrder::pack(an::SideRecord& rec) const {
    Execution::pack(rec); rec.order_type = an::LIMIT;
    rec.price = price_;
}


std::string an::LimitOrder::to_string() const {
    std::stringstream os;
    os << "type" << SEPERATOR << "LIMIT" << DELIMITOR << Execution::to_string() << DELIMITOR
       << "price" << SEPERATOR << floatDecimalPlaces(price_,MAX_PRICE_PRECISION) ;
    return os.str();
}
an::LimitOrder::~LimitOrder() { }


bool an::LimitOrder::amend(amend_t a) {
    bool ok = true;
    switch (a.field) {
        case NONE:   break;
        case PRICE:  price_ = a.price;
                     break;
        case SHARES: shares_ = a.shares; break;
        default:
            ok = false;
            assert(false && "LimitOrder::amend invalid");
    }
    return ok;
}


void an::MarketOrder::applyOrder(MatchingEngine& me) {
    me.applyOrder(std::move(std::unique_ptr<MarketOrder>(this)));
}

void an::MarketOrder::pack(an::SideRecord& rec) const {
    Execution::pack(rec); rec.order_type = an::MARKET;
    if (direction_ == an::BUY) {
        rec.price = MAX_SHARE_PRICE; // Are 0.0 priced limit SELL or high priced limit BUY
    }
}

std::string an::MarketOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "MARKET" << DELIMITOR << Execution::to_string() ;
    return ss.str();
}
an::MarketOrder::~MarketOrder() { }

bool an::MarketOrder::amend(amend_t a) {
    bool ok = true;
    switch (a.field) {
        case NONE:   break;
        case PRICE:  ok = false; break;
        case SHARES: shares_ = a.shares; break;
        default:
            ok = false;
            assert(false && "MarketOrder::amend invalid");
    }
    return ok;
}



void an::CancelOrder::applyOrder(MatchingEngine& me) {
    me.applyOrder(std::move(std::unique_ptr<CancelOrder>(this)));
}

std::string an::CancelOrder::to_string() const {
    std::stringstream ss;
    ss << "type" << SEPERATOR << "CANCEL" << DELIMITOR << Order::to_string() ;
    return ss.str();
}

an::CancelOrder::~CancelOrder() { }



void an::AmendOrder::applyOrder(MatchingEngine& me) {
    me.applyOrder(std::move(std::unique_ptr<AmendOrder>(this)));
}

std::string an::AmendOrder::to_string() const {
    std::ostringstream os;
    os << "type" << SEPERATOR << "AMEND" << DELIMITOR << Order::to_string();
    if (amend_.field != NONE) {
        os << DELIMITOR << amend_.get_field_name() << SEPERATOR ;
        switch (amend_.field) {
            case NONE:
                os << "none"; break;
            case PRICE:
                os << floatDecimalPlaces(amend_.price,MAX_PRICE_PRECISION); break;
            case SHARES:
                os << amend_.shares; break;
            default:
                os << "unknown:field_t";
        }
    }
    return os.str();
}

an::AmendOrder::~AmendOrder() { }



std::string an::Response::to_string() const {
    std::ostringstream os;
    if (message_ != nullptr) {
        os << message_->to_string() << DELIMITOR
           << "response" << SEPERATOR << an::to_string(response_) << DELIMITOR
           << "text" << SEPERATOR << text_;
    } else {
        os << Message::to_string() << DELIMITOR
           << "response" << SEPERATOR << an::to_string(response_) << DELIMITOR
           << "text" << SEPERATOR << text_;
    }
    return os.str();
}

an::Response::~Response() { }



std::string an::TradeReport::to_string() const {
    std::ostringstream os;
    os << "type" << SEPERATOR << "TRADE" << DELIMITOR
       << Message::to_string() << DELIMITOR
       << "orig_order_id" << SEPERATOR << orig_order_id_ << DELIMITOR
       << "symbol" << SEPERATOR << symbol_ << DELIMITOR
       << "direction" << SEPERATOR << an::to_string(direction_) << DELIMITOR
       << "shares" << SEPERATOR << shares_ << DELIMITOR
       << "price" << SEPERATOR << floatDecimalPlaces(price_,MAX_PRICE_PRECISION) ;
    return os.str();
}

an::TradeReport::~TradeReport() { }

