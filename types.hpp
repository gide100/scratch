#ifndef AN_TYPES_HPP
#define AN_TYPES_HPP


#include <string>
#include <cstdint>
#include <ctime>

namespace an {
// Orders
typedef std::uint64_t order_id_t;
typedef std::string location_t;
typedef std::uint64_t shares_t;
typedef double price_t;
typedef enum { BUY, SELL } direction_t;

typedef std::string symbol_t;
const location_t ME = "ME"; // Matching Engine

// Security data
typedef std::time_t date_t;
typedef std::uint32_t ladder_id_t;

// Constants
const price_t MAX_SHARE_PRICE = 1000000.0;
const shares_t MAX_OUTSTANDING_SHARES = 10000000000;

} // an - namespace

#endif
