#ifndef AN_SECURITY_MASTER_HPP
#define AN_SECURITY_MASTER_HPP

#include "types.hpp"
#include <ctime>

using namespace an;

struct security_record_t {
    location_t   exchange;
    symbol_t     symbol;
    price_t      closing_price;
    shares_t     outstanding_shares;
    date_t       born; // Date
    date_t       died; // Date
    bool         tradeable;
    tick_table_t tick_ladder_name;
};

struct tick_table_row_t {
    price_t lower; // [
    price_t upper; // ) less than
    price_t increment;
};

struct tick_table_t {
    std::vector<tick_table_row_t> rows;
};

class TickLadder {
    public:
        TickLadder() {}
        addTickLadder(const ladder_id_t& id, const tick_table_t& ladder);

    private:
        std::map<std::string, tick_table_t> ladders_;
};

#endif
