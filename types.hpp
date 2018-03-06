#ifndef AN_TYPES_HPP
#define AN_TYPES_HPP


#include <string>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <limits>
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <set>
#include <exception>
#include "date.h"

namespace an {
// Orders
typedef std::uint64_t order_id_t;
typedef std::string location_t;
typedef std::uint64_t shares_t;
typedef std::uint64_t sequence_t;
typedef double price_t;
enum direction_t { BUY, SELL };
enum order_t { LIMIT, MARKET, CANCEL, AMEND };
enum response_t { ACK, REJECT, UNKNOWN, ERROR }; 
typedef std::string text_t;

typedef std::string symbol_t;
const location_t ME = "ME"; // Matching Engine

// Security data
typedef std::uint32_t security_id_t;
typedef std::time_t date_t;
typedef std::uint32_t ladder_id_t;
const   std::time_t MY_MAX_DATE = 0x7FFFFFFF;

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

inline const char* to_string(response_t r) {
     switch (r) {
        case ACK: return "ACK";
        case REJECT: return "REJECT";
        case UNKNOWN: return "UNKNOWN";
        case ERROR: return "ERROR";
        default:
           assert(false);
     }
}


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

inline std::string dateToString(const std::time_t& t) {
   auto d = date::floor<date::days>(std::chrono::system_clock::from_time_t(t));
   std::stringstream ss; ss << date::format("%F",d);
   return ss.str();
   // return timeTtoString(t,"%F");
}

} // an - namespace

#endif
