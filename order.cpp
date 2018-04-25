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


enum class Tag : std::size_t { None, Type, Id, Origin, Destination, Symbol, Direction, Shares, Price, Last };

typedef std::bitset<an::ord(Tag::Last)> TagFlags;
void checkFlags(TagFlags expected, TagFlags got) {
    if (expected != got) {
        std::stringstream ss;
        ss << "Invalid flags got " << got << " expected " << expected;
        throw an::OrderError(ss.str());
    }
}

enum class convert_t { INTEGER, UINTEGER, POSITIVE_INTEGER, NATURAL_UINTEGER, NATURAL_INTEGER, STRING, PSTRING, FLOAT, DIRECTION }; 

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
        bool to(TagFlags& flags, PString value) {
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
     
        union {                             // results
            std::int64_t*       integer;
            std::uint64_t*      uinteger;
            PString*            pstring;
            std::string*        str;
            double*             number;
            an::direction_t*    direction;
        };
        bool*       indicator_;             // Was result set, if nullptr don't apply
        convert_t   convert_;               // Conversion
};


struct an::InputResult {
    InputResult() : myFlags(), myType(), myId(0), myOrigin(), myDestination(), mySymbol(),
                    myDirection(an::BUY), myPrice(0.0), myShares(0) { }
    TagFlags         myFlags;
    PString          myType;
    an::order_id_t   myId;
    an::location_t   myOrigin;
    an::location_t   myDestination;
    an::symbol_t     mySymbol;
    an::direction_t  myDirection;
    an::price_t      myPrice;
    an::shares_t     myShares;
};

struct Creator {
    explicit Creator(const PString name) : name_(name) { }

    an::Message* operator()(const an::InputResult& res) const {
        an::Message* myOrder = nullptr;
        if (res.myType == PString("LIMIT")) {
            an::LimitOrder* o = new an::LimitOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myDirection, res.myShares, res.myPrice);
            myOrder = o;
        } else if (res.myType == PString("MARKET")) {
            an::MarketOrder* o = new an::MarketOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myDirection, res.myShares);
            myOrder = o;
        } else if (res.myType == PString("CANCEL")) {
            an::CancelOrder* o = new an::CancelOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol);
            myOrder = o;
        } else if (res.myType == PString("AMEND")) {
            an::AmendOrder* o = nullptr;
            if (res.myFlags[an::ord(Tag::Price)]) {
                o = new an::AmendOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myPrice);
            } else if (res.myFlags[an::ord(Tag::Shares)]) {
                o = new an::AmendOrder(res.myId, res.myOrigin, res.myDestination, res.mySymbol, res.myShares);
            } else {
                throw an::OrderError("Invalid amend (none given)");
            }
            myOrder = o;
        } else if (res.myType == PString("LOGIN")) {
            an::Login* o = new an::Login(res.myOrigin, res.myDestination);
            myOrder = o;
        } else {
            std::stringstream ss;
            ss << "Invalid order type [" << res.myType.to_string() << "]";
            throw an::OrderError(ss.str());
        }
        return myOrder;
    }
    const PString name_;
};

struct Factory {
    Creator     create;     // Creates Messsage object
    TagFlags    flags;      // Which fields (flags) must be present
    TagFlags    select;     // Which fields are grouped together (price/shares amend currently).
    TagFlags    optional;   // Which fields are optional
};

namespace an {
    std::size_t hash_value(const an::PString& p) {
            return std::hash<an::PString>()(p);
    }
}

using an::ord;
static constexpr TagFlags STD_FLAGS = TagFlags { (1 << ord(Tag::Type))   | (1 << ord(Tag::Id)) | 
                                                 (1 << ord(Tag::Origin)) | (1 << ord(Tag::Destination)) };

struct an::AuthorImpl {
    AuthorImpl() : all_fields_{
            { PString("type"),         Reader(PString("type"),        Tag::Type,        &in_.myType) }
           ,{ PString("id"),           Reader(PString("id"),          Tag::Id,          &in_.myId,
                                                      nullptr,        convert_t::NATURAL_UINTEGER ) }
           ,{ PString("origin"),       Reader(PString("origin"),      Tag::Origin,      &in_.myOrigin,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("destination"),  Reader(PString("destination"), Tag::Destination, &in_.myDestination,
                                                      nullptr,        convert_t::STRING ) }
           ,{ PString("symbol"),       Reader(PString("symbol"),      Tag::Symbol,      &in_.mySymbol) }
           ,{ PString("direction"),    Reader(PString("direction"),   Tag::Direction,   &in_.myDirection) }
           ,{ PString("price"),        Reader(PString("price"),       Tag::Price,       &in_.myPrice) }
           ,{ PString("shares"),       Reader(PString("shares"),      Tag::Shares,      &in_.myShares,
                                                      nullptr,        convert_t::NATURAL_INTEGER ) }
        }, orderType_{
            { PString("LIMIT"),
              Factory{ .create = Creator(PString("LIMIT")),
                .flags = TagFlags( STD_FLAGS | TagFlags((1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) 
                                                      | (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price))) ),
                .select = TagFlags(0),
                .optional = TagFlags(0)  } 
            },
            { PString("MARKET"),
              Factory{ .create = Creator(PString("MARKET")),
                .flags = TagFlags( STD_FLAGS | TagFlags((1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) | (1 << ord(Tag::Shares)) ) ),
                .select = TagFlags(0),
                .optional = TagFlags(0)  } 
            },
            { PString("CANCEL"),
              Factory{ .create = Creator(PString("CANCEL")),
                .flags = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
                .select = TagFlags(0),
                .optional = TagFlags(0)  } 
            },
            { PString("AMEND"),
              Factory{ .create = Creator(PString("AMEND")),
                .flags = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
                .select = TagFlags( (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price)) ),
                .optional = TagFlags(0)  } 
            },
            { PString("LOGIN"), 
              Factory{ .create = Creator(PString("LOGIN")),
                .flags = TagFlags( (1 << ord(Tag::Type)) | (1 << ord(Tag::Origin)) | (1 << ord(Tag::Destination)) ),
                .select = TagFlags(0),
                .optional = TagFlags(0)  } 
            }
        }, in_() { 
    }

    std::unordered_map<PString,Reader> all_fields_;
    std::unordered_map< an::PString, Factory > orderType_;
    //std::unordered_map< std::pair<an::PString,ulong>, Factory, boost::hash<std::pair<an::PString,ulong> > > orderType_;
    an::InputResult in_;
};

an::Author::Author() : impl_(new an::AuthorImpl)  {
}

an::Author::~Author() {
}



void an::Author::parse(an::InputResult& inRes, const std::string& input) {
    std::unordered_map<PString,Reader>& all_fields = impl_->all_fields_;

    StrIter myEnd = input.cend();
    StrIter myBegin = input.cbegin();
    SplitResult res;
    TagFlags myPrevFlags;

    while(myBegin != myEnd) {
        auto myDelim = mySplit2(res, myBegin, myEnd);
        //std::cout << "Split - [" << res.tag.to_string() << ',' << res.value.to_string() << "]" << std::endl;
        auto iter = all_fields.find(res.tag);
        if (iter != all_fields.end()) {
            auto& reader = iter->second;
            if (!reader.to(inRes.myFlags, res.value)) {
                std::stringstream ss;
                ss << "Reader [" << reader.description().to_string() << "] did not process [" 
                   << res.tag.to_string() << ',' << res.value.to_string() << "]";
                throw OrderError(ss.str());
            }
        } else {
            std::stringstream ss;
            ss << "Unused token [" << res.tag.to_string() << ',' << res.value.to_string() << "]";
            throw OrderError(ss.str());
        }
        if (inRes.myFlags == myPrevFlags) {
            std::stringstream ss;
            ss << "Repeated token [" << res.tag.to_string() << ',' << res.value.to_string() << "]";
            throw OrderError(ss.str());
        }
        myPrevFlags = inRes.myFlags;
        myBegin = myDelim;
    }
}


an::Message* an::Author::create(const an::InputResult& res) {
    /*
     constexpr TagFlags STD_FLAGS( (1 << ord(Tag::Type)) | (1 << ord(Tag::Id)) | (1 << ord(Tag::Origin))
                                | (1 << ord(Tag::Destination)) );
    static const std::unordered_map< std::pair<an::PString,ulong>, Factory, boost::hash<std::pair<an::PString,ulong> > > orderType {
        { std::make_pair(PString("LIMIT"), 0),
          Factory{ .create = Creator(PString("LIMIT")),
            .flags = TagFlags( STD_FLAGS | TagFlags((1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) 
                                                  | (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price))) ),
            .select = TagFlags(0),
            .optional = TagFlags(0)  } 
        },
        { std::make_pair(PString("MARKET"), 0),
          Factory{ .create = Creator(PString("MARKET")),
            .flags = TagFlags( STD_FLAGS | TagFlags( (1 << ord(Tag::Symbol)) | (1 << ord(Tag::Direction)) | (1 << ord(Tag::Shares)) ) ),
            .select = TagFlags(0),
            .optional = TagFlags(0)  } 
        },
        { std::make_pair(PString("CANCEL"), 0),
          Factory{ .create = Creator(PString("CANCEL")),
            .flags = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
            .select = TagFlags(0),
            .optional = TagFlags(0)  } 
        },
        { std::make_pair(PString("AMEND"), 0),
          Factory{ .create = Creator(PString("AMEND")),
            .flags = TagFlags( STD_FLAGS | TagFlags(1 << ord(Tag::Symbol)) ),
            .select = TagFlags( (1 << ord(Tag::Shares)) | (1 << ord(Tag::Price)) ),
            .optional = TagFlags(0)  } 
        },
        { std::make_pair(PString("LOGIN"), 0), 
          Factory{ .create = Creator(PString("LOGIN")),
            .flags = TagFlags( (1 << ord(Tag::Type)) | (1 << ord(Tag::Origin)) | (1 << ord(Tag::Destination)) ),
            .select = TagFlags(0),
            .optional = TagFlags(0)  } 
        }
    };
    */
    std::unordered_map< an::PString, Factory>& orderType = impl_->orderType_;

    assert( (orderType.find(an::PString("")) == orderType.end()) && "Empty is an invalid type");
    assert( (orderType.find(an::PString(nullptr)) == orderType.end()) && "Empty is an invalid type");

    auto found = orderType.find(an::PString(res.myType));
    if (!res.myFlags[ord(Tag::Type)] || (found == orderType.end())) {
        std::stringstream os;
        os << "Invalid/unset order type [" << res.myType.to_string() << "]";
        throw OrderError(os.str());
    }
    // Check Flags
    TagFlags flags = found->second.flags; // Mask the selectable flags
    TagFlags select = found->second.select; select.flip();
    flags &= select;
    TagFlags myFlags = res.myFlags;
    myFlags &= select;
    checkFlags(flags,myFlags);

    myFlags = res.myFlags; // Make sure we have one of the selectable flags
    myFlags &= found->second.select;
    if (found->second.select.any() && (myFlags.count()!=1) ) {
        std::stringstream os;
        os << "Order type [" << res.myType.to_string() << "] missing selectable fields";
        throw OrderError(os.str());
    }
    Message* messagePtr = found->second.create(res);
/*
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
    */
    return messagePtr;
}

an::Message* an::Author::makeOrder(const std::string& input) {
    impl_->in_.myFlags.reset();
    parse(impl_->in_, input); 
    return create(impl_->in_);
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
       << Reply::to_string() << DELIMITOR
       << "symbol" << SEPERATOR << md_.symbol << DELIMITOR ;
        if (md_.have_bid) {
            os << "bid" << SEPERATOR << floatDecimalPlaces(md_.bid,MAX_PRICE_PRECISION) << DELIMITOR
               << "bid_size" << SEPERATOR << md_.bid_size << DELIMITOR ;
        } else {
            os << "bid=N/A:bid_size=N/A:";
        }
        if (md_.have_ask) {
            os << "ask" << SEPERATOR << floatDecimalPlaces(md_.ask,MAX_PRICE_PRECISION) << DELIMITOR
               << "ask_size" << SEPERATOR << md_.ask_size << DELIMITOR ;
        } else {
            os << "ask=N/A:ask_size=N/A:";
        }
        if (md_.have_last_trade) {
            os << "last_trade_price" << SEPERATOR << floatDecimalPlaces(md_.last_trade_price,MAX_PRICE_PRECISION) << DELIMITOR
               << "last_trade_shares" << SEPERATOR << md_.last_trade_shares << DELIMITOR
               << "trade_time" << SEPERATOR << md_.trade_time << DELIMITOR ;
        } else {
            os << "last_trade_price=N/A:last_trade_shares=N/A:trade_time=N/A:" ;
        }

       os << "quote_time" << SEPERATOR << md_.quote_time << DELIMITOR
          << "volume" << SEPERATOR << floatDecimalPlaces(md_.volume,VOLUME_OUTPUT_PRECISION) ;
    return os.str();
}

an::MarketData::~MarketData() { }

