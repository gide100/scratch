#include "order.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <boost/algorithm/string.hpp>
#include <boost/operators.hpp>
#include <boost/functional/hash.hpp>

const char DELIMITOR = ':';
const char SEPERATOR = '=';


an::Message::Message(location_t origin, location_t dest)   
    : origin_(origin), destination_(dest), reverse_direction_(false) {
}

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

using StrIter = std::string::const_iterator;

StrIter mySplit(std::pair<std::string,std::string>& res, StrIter myBegin, StrIter myEnd) {
    StrIter myDelim=std::find(myBegin, myEnd, DELIMITOR); // one=a or one=a:two=b myDelim [end] or one=a[:]
    StrIter myEq=std::find(myBegin, myDelim, SEPERATOR); // myEq= one[=]
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

using an::PString;

const std::int32_t MAX_TAG_SIZE = 20;
const std::int32_t MAX_VALUE_SIZE = 100;

struct SplitResult {
    SplitResult() : tag(),value() {}
    PString tag;
    PString value;
};

StrIter mySplit2(SplitResult& res, StrIter myBegin, StrIter myEnd) {
    assert(sizeof(std::size_t) == sizeof(StrIter::difference_type) && "size_t and difference_type are the same size");
    StrIter myDelim=std::find(myBegin, myEnd, DELIMITOR); // one=a or one=a:two=b myDelim [end] or one=a[:]
    StrIter myEq=std::find(myBegin, myDelim, SEPERATOR); // myEq= one[=]
    if (myEq==myDelim) { // Not found, end
         std::string token(myBegin, myDelim);
         std::ostringstream os;
         os << "Bad token missing seperator (" <<SEPERATOR<< ") [" << token << "]";
         throw an::OrderError(os.str());
    }
    int32_t len = std::distance(myBegin,myEq);
    if (len > MAX_TAG_SIZE) {
         std::string token(myBegin, myEq);
         std::ostringstream os;
         os << "Bad tag too long (>" << MAX_TAG_SIZE << ") [" << token << "]";
         throw an::OrderError(os.str());
    }
    res.tag.assign(&*myBegin,len);  // one
    len = std::distance(++myEq,myDelim);
    if (len > MAX_VALUE_SIZE) {
         std::string token(myEq, myDelim);
         std::ostringstream os;
         os << "Bad value too long (>" << MAX_VALUE_SIZE << ") [" << token << "]";
         throw an::OrderError(os.str());
    }
    res.value.assign(&*myEq,len); // a
    if (myDelim != myEnd) { //one=a[:]
        ++myDelim; // [t]wo=b
    }
    return myDelim; // [t]wo or end
}


enum class Tag : std::size_t { 
    None, Type, Id, Origin, Destination, Symbol,
    // Order
    Direction, Shares, Price,
    // MarketData
// type=MARKETDATA:origin=ME:destination=:symbol=MSFT:bid=100.0:bid_size=100:ask=101.0:ask_size=10:last_trade_price=100.1:last_trade_shares=50:trade_time=2018-01-01 12:00:00.00000:quote_time=2018-01-01 12:01:00.00000:volume=5005.0
    Bid, BidSize, Ask, AskSize, 
    LastTradePrice, LastTradeShares, TradeTime,
    QuoteTime, 
    Volume,
    // End
    Last };

const std::string& to_string(Tag t) {
    static const std::vector<std::string> TAG_NAME {
        "None", "Type", "Id/Seq", "Origin", "Destination", "Symbol",
        // Order
        "Direction", "Shares", "Price",
        // MarketData
        "Bid", "BidSize", "Ask", "AskSize",
        "LastTradePrice", "LastTradeShares", "TradeTime",
        "QuoteTime", 
        "Volume"
        // End
        //"Last" 
    };
    assert(TAG_NAME.size() == an::ord(Tag::Last));
    return TAG_NAME[an::ord(t)];
}
typedef std::bitset<an::ord(Tag::Last)> TagFlags;

void checkFlags(TagFlags expected, TagFlags got) {
    if (expected != got) {
        std::stringstream ss;
        ss << "Invalid flags" ;
        for (size_t i = 0; i < got.size(); ++i) {
            if (expected[i] != got[i]) {
                ss << ((expected[i]) ? " +" : " -") << to_string(Tag(i));
                ss << "," ;
            }
        }
        ss.seekp(-1, ss.cur); 
        ss << '\0';
        throw an::OrderError(ss.str());
    }
}

enum class convert_t { INTEGER, UINTEGER, POSITIVE_INTEGER, NATURAL_UINTEGER, NATURAL_INTEGER, STRING, PSTRING, FLOAT, DIRECTION }; 
typedef bool indicator_t;

class Reader {
    public:
        Reader(PString description, Tag field, std::int64_t* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::INTEGER, bool select = false ) 
            : description_(description), field_(field), integer(res), 
              indicator_(resIndicator), convert_(convert) { }
        Reader(PString description, Tag field, std::uint64_t* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::POSITIVE_INTEGER, bool select = false ) 
            : description_(description), field_(field), uinteger(res), 
              indicator_(resIndicator), convert_(convert) { }
        Reader(PString description, Tag field, PString* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::PSTRING, bool select = false ) 
            : description_(description), field_(field), pstring(res), 
              indicator_(resIndicator), convert_(convert) { }
        Reader(PString description, Tag field, std::string* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::STRING, bool select = false ) 
            : description_(description), field_(field), str(res), 
              indicator_(resIndicator), convert_(convert) { }
        Reader(PString description, Tag field, double* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::FLOAT, bool select = false ) 
            : description_(description), field_(field), number(res), 
              indicator_(resIndicator), convert_(convert) { }
        Reader(PString description, Tag field, an::direction_t* res, bool* resIndicator = nullptr,
               convert_t convert = convert_t::DIRECTION, bool select = false ) 
            : description_(description), field_(field), direction(res), 
              indicator_(resIndicator), convert_(convert) { }
        ~Reader() {
            integer = nullptr; 
            indicator_ = nullptr;
            convert_ = convert_t::INTEGER;
        }

        const PString& description() const {
            return description_;
        }
        bool to(TagFlags& flags, PString value) const {
            assert((integer != nullptr) && "pointer set in union"); // Any field in results union is ok
            bool ok = false;
            char* stop = nullptr;
            an::direction_t myDirection = an::BUY;
            switch (convert_) {
                case convert_t::UINTEGER:
                case convert_t::NATURAL_UINTEGER:
                    {
                        std::uint64_t myULong = 0;
                        if (!an::pstring2int<uint64_t>(&myULong,value)) {
                            break; // Error
                        }
                        if ((convert_ == convert_t::NATURAL_UINTEGER) && (myULong < 1)) {
                            break;
                        }
                        *uinteger = myULong; ok = true;
                    }
                    break;
                case convert_t::INTEGER:
                case convert_t::POSITIVE_INTEGER: 
                case convert_t::NATURAL_INTEGER:
                    {
                        std::int64_t myLong = 0;
                        if (!an::pstring2int<int64_t>(&myLong,value)) {
                            break; // Error
                        }
                        if ((convert_ == convert_t::POSITIVE_INTEGER) && (myLong < 0)) {
                            break;
                        } else if ((convert_ == convert_t::NATURAL_INTEGER) && (myLong < 1)) {
                            break;
                        }
                        *integer = myLong; ok = true;
                    }
                    break;
                case convert_t::STRING:
                    {
                        (*str) = value.to_string(); ok = true;
                    }
                    break;
                case convert_t::PSTRING:
                    *pstring = value; ok = true;
                    break;
                case convert_t::FLOAT:
                    {
                        double myFloat = std::strtod(value.to_string().c_str(), &stop); // TODO
                        if (*stop != '\0') {
                            break; // Error
                        }
                        *number = myFloat; ok = true;
                    }
                    break;
                case convert_t::DIRECTION:
                    if (value == PString("BUY")) {
                        myDirection = an::BUY;
                    } else if (value == PString("SELL")) {
                        myDirection = an::SELL;
                    } else {
                        break; // Error
                    }
                    *direction = myDirection; ok = true;
                    break;
                default:
                    assert(false && "to invalid conversion");
            }
            if (ok) {
                flags.set(an::ord(field_));
                if (indicator_ != nullptr) {
                    *indicator_ = true;
                }
            }
            return ok;
        }
    private:
        const PString    description_;        // type
        const Tag        field_;              // Direction
     
        union {                               // results
            std::int64_t*       integer;
            std::uint64_t*      uinteger;
            PString*            pstring;
            std::string*        str;
            double*             number;
            an::direction_t*    direction;
        };
        indicator_t*     indicator_;          // Was result set, if nullptr don't apply
        convert_t        convert_;            // Conversion
};

class an::Result {
    public:
        Result() :  myFlags(), myType() {}
        virtual an::Message* dispatch(an::Author& res) const = 0;
        virtual void reset() {
            myFlags = 0;
        }

        TagFlags         myFlags;
        PString          myType;
    protected:
        Order* createOrder(Author& a, const OrderResult& res) const {
            return a.createOrder(res);
        }
        MarketData* createMarketData(Author& a, const MarketDataResult& res) const {
            return a.createMarketData(res);
        }
        Login* createLogin(Author& a, const LoginResult& res) const {
            return a.createLogin(res);
        }

};

class an::OrderResult : public an::Result {
    public:
        OrderResult() : myId(0), myOrigin(), myDestination(), mySymbol(),
                        myDirection(an::BUY), myPrice(0.0), myShares(0) { }

        virtual an::Message* dispatch(an::Author& a) const {
            return createOrder(a,*this);
        }
        virtual void reset() {
            Result::reset();
        }
    public:
        order_id_t       myId;
        location_t       myOrigin;
        location_t       myDestination;
        symbol_t         mySymbol;
        direction_t      myDirection;
        price_t          myPrice;
        shares_t         myShares;
};

class an::MarketDataResult : public an::Result {
    public:
        MarketDataResult() : 
            mySeq(0), myOrigin(), myDestination(), mySymbol(),
            myBid(0.0), myBidInd(false), myBidSize(0),  myAsk(0.0), myAskInd(false), myAskSize(0),
            myLastTradePrice(0.0), myLastTradePriceInd(false), myLastTradeShares(0), myTradeTime(),
            myQuoteTime(), myVolume(0.0)
            { }
        virtual an::Message* dispatch(an::Author& a) const {
            return createMarketData(a,*this);
        }
        virtual void reset() {
            Result::reset();
            myBidInd = myAskInd = myLastTradePriceInd = false;
        }
    public:
        sequence_t          mySeq;
        location_t          myOrigin;
        location_t          myDestination;
        symbol_t            mySymbol;
        price_t             myBid;
        indicator_t         myBidInd;
        an::shares_t        myBidSize;
        price_t             myAsk;
        indicator_t         myAskInd;
        shares_t            myAskSize;
        price_t             myLastTradePrice;
        indicator_t         myLastTradePriceInd;
        shares_t            myLastTradeShares;
        PString             myTradeTime;
        PString             myQuoteTime;
        volume_t            myVolume;
};

class an::LoginResult : public an::Result {
    public:
        LoginResult() : 
            myOrigin(), myDestination()
            { }
        virtual an::Login* dispatch(an::Author& a) const {
            return createLogin(a,*this);
        }
        virtual void reset() {
            Result::reset();
        }
    public:
        location_t          myOrigin;
        location_t          myDestination;
};


void toMd(an::market_data_t& md, const an::MarketDataResult& res) {
    md.seq = res.mySeq;
    md.origin = res.myOrigin;
    //md.destination = res.myDestination;
    md.symbol = res.mySymbol;
    md.have_bid = res.myBidInd != 0;
    if (md.have_bid) {
        md.bid = res.myBid;
        md.bid_size = res.myBidSize;
    } else {
        md.bid = 0; md.bid_size = 0;
    }
    md.have_ask = res.myAskInd != 0;
    if (md.have_ask) {
        md.ask = res.myAsk;
        md.ask_size = res.myAskSize;
    } else {
        md.ask = 0; md.ask_size = 0;
    }
    md.have_last_trade = res.myLastTradePriceInd != 0;
    if (md.have_last_trade) {
        md.last_trade_price = res.myLastTradePrice;
        md.last_trade_shares = res.myLastTradeShares;
        md.trade_time = res.myTradeTime.to_string();
    } else {
        md.last_trade_price = 0.0; md.last_trade_shares = 0; md.trade_time = "";
    }
    md.quote_time = res.myQuoteTime.to_string();
    md.volume = res.myVolume;
}

struct Creator {
    explicit Creator(const PString name) : name_(name) { }

    an::Order* operator()(const an::OrderResult& res) const {
        an::Order* myOrder = nullptr;
        if (res.myType == PString("LIMIT")) {
            an::LimitOrder*  o = new an::LimitOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myDirection, res.myShares, res.myPrice);
            myOrder = o;
        } else if (res.myType == PString("MARKET")) {
            an::MarketOrder* o = new an::MarketOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myDirection, res.myShares);
            myOrder = o;
        } else if (res.myType == PString("CANCEL")) {
            an::CancelOrder* o = new an::CancelOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol);
            myOrder = o;
        } else if (res.myType == PString("AMEND")) {
            an::AmendOrder*  o = nullptr;
            if (res.myFlags[an::ord(Tag::Price)]) {
                o = new an::AmendOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myPrice);
            } else if (res.myFlags[an::ord(Tag::Shares)]) {
                o = new an::AmendOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myShares);
            } else {
                throw an::OrderError("Invalid amend (none given)");
            }
            myOrder = o;
        } else {
            std::stringstream ss;
            ss << "Invalid order type [" << res.myType.to_string() << "]";
            throw an::OrderError(ss.str());
        }
        return myOrder;
    }
    an::MarketData* operator()(const an::MarketDataResult& res) const {
        an::MarketData* myMsg = nullptr;
        if (res.myType == PString("MARKETDATA")) {
            an::market_data_t md;
            toMd(md,res);
            myMsg = new an::MarketData(res.myOrigin, md);
        }
        return myMsg;
    }
    an::Login* operator()(const an::LoginResult& res) const {
        an::Login* myLogin = nullptr;
        if (res.myType == PString("LOGIN")) {
            an::Login* o = new an::Login(res.myOrigin, res.myDestination);
            myLogin = o;
        }
        return myLogin;
    }
    const PString name_;
};

struct TagHandler {
    Creator     create;     // Creates Messsage object
    TagFlags    flags;      // Which fields (flags) must be present
    TagFlags    select;     // Which fields are grouped together (price or shares in amend).
    std::vector<TagFlags> optional;   // Which fields are optional
};
using NamedTagHandler = std::unordered_map< an::PString, TagHandler>;

namespace an {
    std::size_t hash_value(const an::PString& p) {
            return std::hash<an::PString>()(p);
    }
}

using an::ord;
static constexpr TagFlags STD_FLAGS = TagFlags { (1 << ord(Tag::Type))   | (1 << ord(Tag::Id)) | 
                                                 (1 << ord(Tag::Origin)) | (1 << ord(Tag::Destination)) };

struct an::AuthorImpl {
    AuthorImpl() : 
        orderFields_{
            { PString("type"),         Reader(PString("type"),        Tag::Type,        &orderRes_.myType) }
           ,{ PString("id"),           Reader(PString("id"),          Tag::Id,          &orderRes_.myId,
                                                      nullptr,        convert_t::NATURAL_UINTEGER ) }
           ,{ PString("origin"),       Reader(PString("origin"),      Tag::Origin,      &orderRes_.myOrigin,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("destination"),  Reader(PString("destination"), Tag::Destination, &orderRes_.myDestination,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("symbol"),       Reader(PString("symbol"),      Tag::Symbol,      &orderRes_.mySymbol) }
           ,{ PString("direction"),    Reader(PString("direction"),   Tag::Direction,   &orderRes_.myDirection) }
           ,{ PString("price"),        Reader(PString("price"),       Tag::Price,       &orderRes_.myPrice) }
           ,{ PString("shares"),       Reader(PString("shares"),      Tag::Shares,      &orderRes_.myShares,
                                                      nullptr,        convert_t::NATURAL_INTEGER ) }
        }, orderType_{
            { PString("LIMIT"),
              TagHandler{ 
                .create   = Creator(PString("LIMIT")),
                .flags    = TagFlags( STD_FLAGS | TagFlags((1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) 
                                                | (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price))) ),
                .select   = TagFlags(0),
                .optional = {} } 
            },
            { PString("MARKET"),
              TagHandler{ 
                .create   = Creator(PString("MARKET")),
                .flags    = TagFlags( STD_FLAGS | TagFlags((1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) | (1 << ord(Tag::Shares)) ) ),
                .select   = TagFlags(0),
                .optional = {} } 
            },
            { PString("CANCEL"),
              TagHandler{ 
                .create   = Creator(PString("CANCEL")),
                .flags    = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
                .select   = TagFlags(0),
                .optional = {} } 
            },
            { PString("AMEND"),
              TagHandler{ 
                .create   = Creator(PString("AMEND")),
                .flags    = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
                .select   = TagFlags( (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price)) ),
                .optional = {} } 
            }
        }, orderRes_(),
        //type=MARKETDATA:seq=1:origin=ME:destination=<all>:symbol=MSFT:bid=100.0:bid_size=100:ask=101.0:ask_size=10:last_trade_price=100.1:last_trade_shares=50:trade_time=2018-01-01 12.00.00.00000:quote_time=2018-01-01 12.01.00.00000:volume=5005.0
        marketDataFields_ {
            { PString("type"),         Reader(PString("type"),        Tag::Type,        &mdRes_.myType) }
           ,{ PString("seq"),          Reader(PString("seq"),         Tag::Id,          &mdRes_.mySeq,
                                                      nullptr,        convert_t::NATURAL_UINTEGER ) }
           ,{ PString("origin"),       Reader(PString("origin"),      Tag::Origin,      &mdRes_.myOrigin,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("destination"),  Reader(PString("destination"), Tag::Destination, &mdRes_.myDestination,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("symbol"),       Reader(PString("symbol"),      Tag::Symbol,      &mdRes_.mySymbol) }
           ,{ PString("bid"),          Reader(PString("bid"),         Tag::Bid,         &mdRes_.myBid,
                                                    &mdRes_.myBidInd, convert_t::FLOAT) }
           ,{ PString("bid_size"),     Reader(PString("bid_size"),    Tag::BidSize,     &mdRes_.myBidSize,
                                                      nullptr,        convert_t::NATURAL_INTEGER) }
           ,{ PString("ask"),          Reader(PString("ask"),         Tag::Ask,         &mdRes_.myAsk,
                                                    &mdRes_.myAskInd, convert_t::FLOAT) }
           ,{ PString("ask_size"),     Reader(PString("ask_size"),    Tag::AskSize,     &mdRes_.myAskSize,
                                                      nullptr,        convert_t::NATURAL_INTEGER) }
           ,{ PString("last_trade_price"), Reader(PString("last_trade_price"),  Tag::LastTradePrice, &mdRes_.myLastTradePrice,
                                                    &mdRes_.myLastTradePriceInd, convert_t::FLOAT) }
           ,{ PString("last_trade_shares"), Reader(PString("last_trade_shares"),  Tag::LastTradeShares, &mdRes_.myLastTradeShares,
                                                      nullptr,        convert_t::NATURAL_INTEGER) }
           ,{ PString("trade_time"),   Reader(PString("trade_time"),  Tag::TradeTime, &mdRes_.myTradeTime,
                                                      nullptr,        convert_t::PSTRING) }
           ,{ PString("quote_time"),   Reader(PString("quote_time"),  Tag::QuoteTime, &mdRes_.myQuoteTime,
                                                      nullptr,        convert_t::PSTRING) }
           ,{ PString("volume"),       Reader(PString("volume"),      Tag::Volume, &mdRes_.myVolume,
                                                      nullptr,        convert_t::FLOAT) }
        }, marketDataType_ {
            { PString("MARKETDATA"),
              TagHandler{ 
                .create  = Creator(PString("MARKETDATA")),
                .flags   = TagFlags( (1 << ord(Tag::Type))          | (1 << ord(Tag::Id)) | 
                                     (1 << ord(Tag::Origin))        | (1 << ord(Tag::Destination)) |
                                     (1 << ord(Tag::Symbol))        | 
                                     (1 << ord(Tag::Bid))           | (1 << ord(Tag::BidSize)) |
                                     (1 << ord(Tag::Ask))           | (1 << ord(Tag::AskSize)) |
                                     (1 << ord(Tag::LastTradePrice))| (1 << ord(Tag::LastTradeShares)) |
                                     (1 << ord(Tag::TradeTime))     | (1 << ord(Tag::QuoteTime)) |
                                     (1 << ord(Tag::Volume))
                                 ),
                .select   = TagFlags(1 << ord(Tag::Destination)),
                .optional = { 
                                TagFlags( (1 << ord(Tag::Bid)) | (1 << ord(Tag::BidSize)) )  
                               ,TagFlags( (1 << ord(Tag::Ask)) | (1 << ord(Tag::AskSize)) )  
                               ,TagFlags( (1 << ord(Tag::LastTradePrice)) | (1 << ord(Tag::LastTradeShares)) 
                                        | (1 << ord(Tag::TradeTime)) )  
                            } 
                       }
            },
        }, mdRes_(),
        loginFields_ {
            { PString("type"),         Reader(PString("type"),        Tag::Type,        &loginRes_.myType) }
           ,{ PString("origin"),       Reader(PString("origin"),      Tag::Origin,      &loginRes_.myOrigin,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("destination"),  Reader(PString("destination"), Tag::Destination, &loginRes_.myDestination,
                                                      nullptr,        convert_t::STRING ) }
        }, loginType_ {
            { PString("LOGIN"),
              TagHandler{ 
                .create   = Creator(PString("LOGIN")),
                .flags    = TagFlags(  (1 << ord(Tag::Type)) | (1 << ord(Tag::Origin)) | (1 << ord(Tag::Destination)) ),
                .select   = TagFlags(0),
                .optional = {}
              }
            }
        }, loginRes_() { 
    }
    void parse(Result& res, const std::string& input, const std::unordered_map<PString,Reader>& inputFields);

    // Messages
    std::unordered_map<PString,Reader> orderFields_;
    NamedTagHandler orderType_;
    OrderResult orderRes_;
    // MarketData
    std::unordered_map<PString,Reader> marketDataFields_;
    NamedTagHandler marketDataType_;
    MarketDataResult mdRes_;
    // Login
    std::unordered_map<PString,Reader> loginFields_;
    NamedTagHandler loginType_;
    LoginResult loginRes_;
};

an::Author::Author() : impl_(new an::AuthorImpl)  {
}

an::Author::~Author() {
}



void an::AuthorImpl::parse(an::Result& inRes, const std::string& input, const std::unordered_map<PString,Reader>& inputFields) {
    StrIter myEnd = input.cend();
    StrIter myBegin = input.cbegin();
    SplitResult split;
    TagFlags myPrevFlags;

    while(myBegin != myEnd) {
        auto myDelim = mySplit2(split, myBegin, myEnd);
        //std::cout << "Split - [" << res.tag.to_string() << ',' << res.value.to_string() << "]" << std::endl;
        auto iter = inputFields.find(split.tag);
        if (iter != inputFields.end()) {
            auto& reader = iter->second;
            if (!reader.to(inRes.myFlags, split.value)) {
                std::ostringstream os;
                os << "Reader [" << reader.description().to_string() << "] did not process [" 
                   << split.tag.to_string() << ',' << split.value.to_string() << "]";
                throw OrderError(os.str());
            }
        } else {
            std::ostringstream os;
            os << "Unused token [" << split.tag.to_string() << ',' << split.value.to_string() << "]";
            throw OrderError(os.str());
        }
        if (inRes.myFlags == myPrevFlags) {
            std::ostringstream os;
            os << "Repeated token [" << split.tag.to_string() << ',' << split.value.to_string() << "]";
            throw OrderError(os.str());
        }
        myPrevFlags = inRes.myFlags;
        myBegin = myDelim;
    }
}

an::Message* an::Author::create(const an::Result& res) { 
    return res.dispatch(*this);
}

void validateNamedTagHandler(NamedTagHandler& tagHandler) {
    assert( (tagHandler.find(an::PString("")) == tagHandler.end()) && "Empty is an invalid type");
    assert( (tagHandler.find(an::PString(nullptr)) == tagHandler.end()) && "Empty is an invalid type");
}

NamedTagHandler::iterator findHandler(NamedTagHandler& tagHandler, const PString& name, bool haveType) {
    auto found = tagHandler.find(name);
    if (!haveType) {
        std::stringstream os;
        os << "Unset order type [" << name.to_string() << "]";
        throw an::OrderError(os.str());
    } else if (found == tagHandler.end()) {
        std::stringstream os;
        os << "Invalid order type [" << name.to_string() << "]";
        throw an::OrderError(os.str());
    } // else all good
    return found;
}

void validateCoreFlags(TagFlags flags, TagFlags select, const std::vector<TagFlags>& optional, TagFlags myFlags) {
    // Check Flags
    // Mask the selectable flags
    select.flip();
    flags &= select;
    myFlags &= select;
    for (TagFlags opt: optional) {
        opt.flip();
        flags &= opt;
        myFlags &= opt;
    }
    checkFlags(flags,myFlags); // Throws
}

void validateSelectable(TagFlags select, TagFlags myFlags) {
    // Make sure we have one of the selectable flags
    myFlags &= select;
    if (select.any()) {
        if (myFlags.count()==0) {
            std::string s("Message missing selectable fields");
            throw an::OrderError(s);
        } else if (myFlags.count() != 1) {
            std::string s("Message too many selectable fields");
            throw an::OrderError(s);
        }
    }
}

void validateOptional(std::vector<TagFlags>& optional, TagFlags myFlags) {
    for (TagFlags opt: optional) {
        TagFlags o = opt & myFlags;
        if (o != 0) {
            checkFlags(opt,o);
        }
    }
}

TagHandler& validate(NamedTagHandler& tagHandler, const an::Result& res) {
    validateNamedTagHandler(tagHandler);
    auto found = findHandler(tagHandler, an::PString(res.myType), res.myFlags[ord(Tag::Type)]); //Throws
    TagHandler& th = found->second;
    validateCoreFlags(th.flags, th.select, th.optional, res.myFlags);
    validateSelectable(th.select, res.myFlags);
    validateOptional(th.optional, res.myFlags);
    return th;
}

an::Order* an::Author::createOrder(const an::OrderResult& res) const {
    TagHandler& th = validate(impl_->orderType_, res);
    Order* orderPtr = th.create(res);
    return orderPtr;
}

an::Login* an::Author::createLogin(const an::LoginResult& res) const {
    TagHandler& th = validate(impl_->loginType_, res);
    Login* loginPtr = th.create(res);
    return loginPtr;
}

an::MarketData* an::Author::createMarketData(const an::MarketDataResult& res) const {
    TagHandler& th = validate(impl_->marketDataType_, res);
    MarketData* mdPtr = th.create(res);
    return mdPtr;
}


an::Order* an::Author::makeOrder(const std::string& input) {
    impl_->orderRes_.reset();
    impl_->parse(impl_->orderRes_, input, impl_->orderFields_); 
    return createOrder(impl_->orderRes_);
}

an::Login* an::Author::makeLogin(const std::string& input) {
    impl_->loginRes_.reset();
    impl_->parse(impl_->loginRes_, input, impl_->loginFields_); 
    return createLogin(impl_->loginRes_);
}

void an::Author::setMarketData(market_data_t& md, const std::string& input) {
    impl_->mdRes_.reset();

    impl_->parse(impl_->mdRes_, input, impl_->marketDataFields_); 
    (void) validate(impl_->marketDataType_, impl_->mdRes_);
    toMd(md, impl_->mdRes_);
}

an::MarketData* an::Author::makeMarketData(const std::string& input) {
    impl_->mdRes_.reset();
    impl_->parse(impl_->mdRes_, input, impl_->marketDataFields_); 
    return createMarketData(impl_->mdRes_);
}


an::Message* an::Message::makeOrder(const std::string& input) {
    using an::ord;

    auto myEnd = input.cend();
    auto myBegin = input.cbegin();
    std::pair<std::string,std::string> res;

    TagFlags myFlags;
    TagFlags myPrevFlags;
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
            myFlags.set(ord(Tag::Type));
	        used = true;
        }
        if (res.first == "id") {
            myId = std::stoull(res.second);
            myFlags.set(ord(Tag::Id));
	        used = true;
        }
        if (res.first == "origin") {
            myOrigin = res.second;
            myFlags.set(ord(Tag::Origin));
	        used = true;
        }
        if (res.first == "destination") {
            myDestination = res.second;
            myFlags.set(ord(Tag::Destination));
	        used = true;
        }
        if (res.first == "symbol") {
            mySymbol = res.second;
            myFlags.set(ord(Tag::Symbol));
	        used = true;
        }
        if (res.first == "direction") {
            if (res.second == "BUY") {
                myDirection = an::BUY;
                myFlags.set(ord(Tag::Direction));
		        used = true;
            } else if (res.second == "SELL") {
                myDirection = an::SELL;
                myFlags.set(ord(Tag::Direction));
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
            myFlags.set(ord(Tag::Shares));
	        used = true;
        }
        if (res.first == "price") {
            myPrice = std::stod(res.second);
            myFlags.set(ord(Tag::Price));
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
    TagFlags f; f.set(ord(Tag::Type)); f.set(ord(Tag::Id)); f.set(ord(Tag::Origin)); 
                  f.set(ord(Tag::Destination)); f.set(ord(Tag::Symbol));
    if (myType == "LIMIT") {
        f.set(ord(Tag::Direction)); f.set(ord(Tag::Shares)); f.set(ord(Tag::Price));
        checkFlags(f, myFlags);
        an::LimitOrder* o = new an::LimitOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares, myPrice);
        myOrder = o;
    } else if (myType == "MARKET") {
        f.set(ord(Tag::Direction)); f.set(ord(Tag::Shares));
        checkFlags(f, myFlags);
        an::MarketOrder* o = new an::MarketOrder(myId, myOrigin, myDestination, mySymbol, myDirection, myShares);
        myOrder = o;
    } else if (myType == "CANCEL") {
        checkFlags(f, myFlags);
        an::CancelOrder* o = new an::CancelOrder(myId, myOrigin, myDestination, mySymbol);
        myOrder = o;
    } else if (myType == "AMEND") {
        TagFlags f1(myFlags); f1.reset(ord(Tag::Price)); f1.reset(ord(Tag::Shares));
        checkFlags(f, f1);
        an::AmendOrder* o = nullptr;
        if (myFlags[ord(Tag::Price)]) {
            o = new an::AmendOrder(myId, myOrigin, myDestination, mySymbol, myPrice);
        } else if (myFlags[ord(Tag::Shares)]) {
            if (myShares <= 0) {
                throw OrderError("Invalid amend number of shares too small");
            }
            o = new an::AmendOrder(myId, myOrigin, myDestination, mySymbol, myShares);
        } else {
            throw OrderError("Invalid amend (none given)");
        }
        myOrder = o;
    } else if (myType == "LOGIN") {
        TagFlags f1; f1.set(ord(Tag::Type)); f1.set(ord(Tag::Origin)); f1.set(ord(Tag::Destination));
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


an::Reply::~Reply() { }
std::string an::Reply::to_string() const { return Message::to_string(); }

std::string an::Response::to_string() const {
    std::ostringstream os;
    if (message_ != nullptr) {
        os << message_->to_string() << DELIMITOR
           << "response" << SEPERATOR << an::to_string(response_) << DELIMITOR
           << "text" << SEPERATOR << text_;
    } else {
        os << "type" << SEPERATOR << "REPLY" << DELIMITOR
           << Message::to_string() << DELIMITOR
           << "response" << SEPERATOR << an::to_string(response_) << DELIMITOR
           << "text" << SEPERATOR << text_;
    }
    return os.str();
}

an::Response::~Response() { }



std::string an::TradeReport::to_string() const {
    std::ostringstream os;
    os << "type" << SEPERATOR << "TRADE" << DELIMITOR
       << Reply::to_string() << DELIMITOR
       << "orig_order_id" << SEPERATOR << orig_order_id_ << DELIMITOR
       << "symbol" << SEPERATOR << symbol_ << DELIMITOR
       << "direction" << SEPERATOR << an::to_string(direction_) << DELIMITOR
       << "shares" << SEPERATOR << shares_ << DELIMITOR
       << "price" << SEPERATOR << floatDecimalPlaces(price_,MAX_PRICE_PRECISION) ;
    return os.str();
}

an::TradeReport::~TradeReport() { }

std::string an::MarketData::to_string() const {
    std::ostringstream os;
    os << "type" << SEPERATOR << "MARKETDATA" << DELIMITOR
       << "seq" << SEPERATOR << md_.seq << DELIMITOR
       << Reply::to_string() << DELIMITOR
       << "symbol" << SEPERATOR << md_.symbol << DELIMITOR ;
        if (md_.have_bid) {
            os << "bid" << SEPERATOR << floatDecimalPlaces(md_.bid,MAX_PRICE_PRECISION) << DELIMITOR
               << "bid_size" << SEPERATOR << md_.bid_size << DELIMITOR ;
        }
        if (md_.have_ask) {
            os << "ask" << SEPERATOR << floatDecimalPlaces(md_.ask,MAX_PRICE_PRECISION) << DELIMITOR
               << "ask_size" << SEPERATOR << md_.ask_size << DELIMITOR ;
        }
        if (md_.have_last_trade) {
            os << "last_trade_price" << SEPERATOR << floatDecimalPlaces(md_.last_trade_price,MAX_PRICE_PRECISION) << DELIMITOR
               << "last_trade_shares" << SEPERATOR << md_.last_trade_shares << DELIMITOR
               << "trade_time" << SEPERATOR << md_.trade_time << DELIMITOR ;
        }
    os << "quote_time" << SEPERATOR << md_.quote_time << DELIMITOR
       << "volume" << SEPERATOR << floatDecimalPlaces(md_.volume,VOLUME_OUTPUT_PRECISION) ;
    return os.str();
}

an::MarketData::~MarketData() { }

