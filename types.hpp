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
#include <boost/format.hpp>

namespace an {
// Orders
typedef std::uint64_t order_id_t;
typedef std::string location_t;
typedef std::int64_t shares_t;
typedef std::uint64_t sequence_t;
typedef double price_t;
const int MAX_PRICE_PRECISION = 7;
constexpr double PRICE_EPSILON = 1.0/std::pow(10,an::MAX_PRICE_PRECISION);

enum direction_t { BUY, SELL };
enum order_t { LIMIT, MARKET, CANCEL, AMEND };
enum response_t { ACK, COMPLETE, REJECT, CANCELLED, UNKNOWN, ERROR }; 
typedef std::string text_t;
typedef double volume_t;
typedef std::int64_t counter_t;

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

} // an - namespace

#endif
