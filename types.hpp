#ifndef AN_TYPES_HPP
#define AN_TYPES_HPP

#include <string>
#include <sstream>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <limits>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <deque>
#include <set>
#include <exception>
#include <cmath>
#include <memory>
#include "date.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/operators.hpp>
#include <iostream>

namespace an {
// Orders
typedef std::uint64_t order_id_t;
typedef std::string location_t;
typedef std::int64_t shares_t;
typedef std::uint64_t sequence_t;
typedef double price_t;
const int MAX_PRICE_PRECISION = 7;
const int VOLUME_OUTPUT_PRECISION = 2;
constexpr double PRICE_EPSILON = 1.0/std::pow(10,an::MAX_PRICE_PRECISION);
typedef std::chrono::steady_clock::time_point since_t;

enum direction_t { BUY, SELL };
enum order_t { LIMIT, MARKET, CANCEL, AMEND };
enum response_t { ACK, COMPLETE, REJECT, CANCELLED, UNKNOWN, ERROR };
typedef std::string text_t;
typedef double volume_t;
typedef std::int64_t counter_t;

typedef std::string timestamp_t;

typedef std::string symbol_t;
const location_t ME = "ME"; // Matching Engine

// Security data
typedef std::uint32_t security_id_t;
typedef std::time_t date_t;
typedef std::uint32_t ladder_id_t;
const   date_t MY_MAX_DATE = 0x7FFFFFFF;

const std::string MY_LOCALE = "en_GB.UTF-8";

// Constants
const price_t MAX_SHARE_PRICE = 1000000.0;
const shares_t MAX_OUTSTANDING_SHARES = 10000000000;

// Transport
typedef std::string transport_msg_t ;

// Market Data
struct market_data_t {
    sequence_t      seq; // Id
    location_t      origin;
    symbol_t        symbol;
    bool            have_bid;
    price_t         bid;
    shares_t        bid_size;
    bool            have_ask;
    price_t         ask;
    shares_t        ask_size;
    bool            have_last_trade;
    price_t         last_trade_price;
    shares_t        last_trade_shares;
    timestamp_t     trade_time;
    timestamp_t     quote_time;
    volume_t        volume;
};

// Old school Pascal ord (ordinal) function, converts from an enum to its underlying type (e.g. size_t or int)
template <typename E>
constexpr auto ord(E enumerator) noexcept {
    return static_cast< std::underlying_type_t<E> >(enumerator);
}

inline const char* to_string(direction_t d) {
     switch (d) {
        case BUY: return "BUY";
        case SELL: return "SELL";
        default:
           assert(false);
     }
}

inline const char* to_string_book(direction_t d) {
     switch (d) {
        case BUY: return "BID";
        case SELL: return "ASK";
        default:
           assert(false);
     }
}

inline const char* to_string(response_t r) {
     switch (r) {
        case ACK:       return "ACK";
        case COMPLETE:  return "COMPLETE";
        case REJECT:    return "REJECT";
        case CANCELLED: return "CANCELLED";
        case UNKNOWN:   return "UNKNOWN";
        case ERROR:     return "ERROR";
        default:
           assert(false);
     }
}

#define GOT_HERE printf("Got here Line %s, %d\n",__FILE__,__LINE__)

inline std::time_t stringToTimeT(const std::string& dateStr, const char* const dateFormat) {
    std::stringstream str(dateStr);
    str.imbue( std::locale(MY_LOCALE) );
    std::chrono::time_point< std::chrono::system_clock, std::chrono::seconds > result;
    date::from_stream( str, dateFormat, result );
    return result.time_since_epoch().count() ;
}

inline std::time_t dateToTimeT(const std::string& dateStr) {
    return stringToTimeT(dateStr,"%Y-%m-%d"); // 1999-12-31
}

inline std::string timeTtoString(const std::time_t& t,  const char* const dateFormat) {
   auto dt = date::floor<std::chrono::milliseconds>(std::chrono::system_clock::from_time_t(t));
   std::stringstream ss; ss << date::format(dateFormat,dt);
   return ss.str();
}

inline std::string dateToString(date_t myDate) {
   auto d = date::floor<date::days>(std::chrono::system_clock::from_time_t(myDate));
   std::stringstream ss; ss << date::format("%F",d);
   return ss.str();
   // return timeTtoString(t,"%F");
}


inline double roundTo(double price, int decimal_places) {
    double multiplier = std::pow(10,decimal_places);
    price*=multiplier;
    if( price >= 0.0 ) {
        price = std::floor(price + 0.5);
    }
    price = std::ceil(price - 0.5);
    return price / multiplier;
}

inline double roundToAny(double number, double round_value) {
    assert(round_value >= 0.0 && "roundToAny negative round_value not allowed");
    if (round_value != 0.0 && number != 0.0) {
        double sign = (number > 0.0) ? 1.0 : -1.0;
        number *= sign;
        number /= round_value;
        //long fixedPoint = (long) std::floor(number+0.5);
        number = std::floor(number+0.5) * round_value;
        number *= sign;
    }
    //std::cout << "roundToAny=" << number << std::endl;
    return number;
}


inline double truncate(double d) {
    return (d>0) ? std::floor(d) : std::ceil(d) ;
}


inline int compare3decimalplaces(double lhs, double rhs) {
    constexpr double pow10 = std::pow(10,3); // 3 decimal places
    const double res_lhs = truncate(pow10 * lhs);
    const double res_rhs = truncate(pow10 * rhs);
    int result = 0;
    if (res_lhs > res_rhs) {
        result = 1;
    } else if (res_lhs < res_rhs) {
        result = -1;
    } else {
        result = 0;
    }
    return result;
}


inline std::string floatFormat(double num, int width) {
    if (num == 0.0) {
        return "0.0";
    }
    int d = (int)std::ceil(std::log10(num < 0 ? -num : num)); /*digits before decimal point*/
    double order = std::pow(10.0, width - d);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(std::max(width - d, 0)) << std::round(num * order) / order;
    return ss.str();
}

inline std::string floatDecimalPlaces(double value, int precision) {
    assert(precision >= 0 && "floatDecimalPlaces negative precision not allowed");
    // https://stackoverflow.com/questions/900326/how-do-i-elegantly-format-string-in-c-so-that-it-is-rounded-to-6-decimal-place
    // https://stackoverflow.com/questions/2475642/how-to-achieve-the-following-c-output-formatting
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    std::string s(ss.str());
    if (precision) {
        s.resize(s.find_last_not_of("0") + 1);
        if (s.back() == '.') {
            s+='0';
        } else if (s.back() == '.') {
           s.pop_back();
        }
    }
    return s;
}

//// https://stackoverflow.com/questions/17946124/most-simple-way-to-get-string-containing-time-interval
//inline std::string sinceToHHMMSS(since_t since) {
//    std::ostringstream os;
//    auto hh = std::chrono::duration_cast<std::chrono::hours>(since);
//    since -= hh;
//    auto mm = std::chrono::duration_cast<std::chrono::minutes>(since);
//    since -= mm;
//    auto ss = std::chrono::duration_cast<std::chrono::seconds>(since);
//    since -= ss;
//    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(since);
//
//    os << std::setfill('0') << std::setw(2) << hh.count() << ':'
//       << std::setfill('0') << std::setw(2) << mm.count() << ':'
//       << std::setfill('0') << std::setw(2) << ss.count() << '.'
//       << std::setfill('0') << std::setw(3) << ms.count() ;
//    return os.str();
//}
// ************************ PSTRING ************************
// Pascal like string
// https://stackoverflow.com/questions/7n648947/declaring-pascal-style-strings-in-c
struct PString : public boost::totally_ordered< PString > {
    public:
        static const std::int32_t MAX_PSTRING_LEN = (1<<15)-1; //32k

        PString() : length_(0), str_(nullptr) {
            //std::cout << "PString " << std::endl;
        }
        explicit PString(const char* s) : length_(s==nullptr? 0 : strnlen(s,MAX_PSTRING_LEN+1)), str_(s) {
            //std::cout << "PString::char* " << length_ << std::endl;
            if (length_ > MAX_PSTRING_LEN) {
                throw std::runtime_error("PString too long");
            }
        }
        explicit PString(const char* s, std::int32_t len) : length_(s==nullptr? 0 : len), str_(s) {
            //std::cout << "PString::char* len " << length_ << std::endl;
            if (length_ > MAX_PSTRING_LEN) {
                throw std::runtime_error("PString too long");
            }
        }
        explicit PString(const std::string& s) : length_(s.size()), str_(s.c_str()) {
            //std::cout << "PString::string " << length_ << std::endl;
            if (s.length() > MAX_PSTRING_LEN) {
                throw std::runtime_error("PString too long");
            }
        }
        PString(const PString& p) noexcept : length_(p.length_), str_(p.str_) {
            //std::cout << "PString::const PString " << std::endl;
        }
        explicit PString(PString&& p) noexcept : length_(p.length_), str_(p.str_) {
            //std::cout << "PString::PString&& " << std::endl;
        }
        //explicit PString(std::string&& s) noexcept : length_(s.size()), str_(s.c_str()) {
        //    std::cout << "PString::string&& " << std::endl;
        //}

        ~PString() { length_=0; str_=nullptr; }

        void assign(const char* s, std::int32_t len) {
            if (len > MAX_PSTRING_LEN) {
                throw std::runtime_error("PString too long");
            }
            str_ = s; length_ = len;
        }

        std::string to_string() const {
            std::string s(str_,length_);
            return s;
        }

        friend inline bool operator==(const PString& lhs, const PString& rhs);
        friend inline bool operator<(const PString& lhs, const PString& rhs);

        PString& operator=(const PString& rhs) {
            // check for self-assignment
            if(&rhs == this) {
                return *this;
            }
            length_ = rhs.length_;
            str_ = rhs.str_;
            return *this;
        }

        template <typename INT> friend bool pstring2int(INT* res, PString s);

        std::size_t hash() const {
            return hash_c_string(str_, length_);
        }


        static constexpr std::size_t hash_c_string(const char* p, size_t s) {
            std::size_t result = 0;
            const std::size_t prime = 31;
            for (size_t i = 0; i < s; ++i) {
                result = p[i] + (result * prime);
            }
            return result;
        }
        std::int32_t    length_;
        const char*     str_; // Null terminated. No ownership
};

inline bool operator==(const PString& lhs, const PString& rhs) {
    return (lhs.length_ == rhs.length_) && (std::strncmp(lhs.str_, rhs.str_, lhs.length_) == 0);
}

inline bool operator<(const PString& lhs, const PString& rhs) {
    auto len = std::min(lhs.length_,rhs.length_);
    int i = 0;
    while (i < len) {
        if (lhs.str_[i] != rhs.str_[i]) {
            return lhs.str_[i] < rhs.str_[i];
        }
        ++i;
    }
    return (i == lhs.length_) && (i != rhs.length_);
}

template <typename INT>
bool pstring2int(INT* res, PString s) {
    if ((s.str_ == nullptr) || (s.length_ == 0)) {
        return 0;
    }
    bool neg = false;
    int32_t i = 0;
    if (s.str_[i] == '-') {
        neg = true;
        ++i;
    } else if (s.str_[i] == '+') {
        ++i;
    }
    INT ret = 0;
    for(; i < s.length_; ++i) {
        if (!std::isdigit(s.str_[i])) {
            return false;
        }
        ret = ret * 10 + (s.str_[i] - '0');
        if ((i+1) != s.length_) { // Not at end 
            if ( ret > (std::numeric_limits<INT>::max() / INT(10)) ) {
                return false;
            }
        }
    }
    if (neg) {
        *res = -ret;
    } else {
        *res = ret;
    }
    return true;
}

} // an - namespace

namespace std {
    template <> struct hash<an::PString> {
        size_t operator()(const an::PString& s) const {
            return s.hash();
        }
    };
}

#endif
