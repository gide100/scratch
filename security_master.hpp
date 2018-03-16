#ifndef AN_SECURITY_MASTER_HPP
#define AN_SECURITY_MASTER_HPP

#include "types.hpp"
#include <iostream>

namespace an {

using std::runtime_error;
class SecurityError : public runtime_error {
    public:
        SecurityError(const std::string& msg) : runtime_error(msg) {}
};

struct tick_table_row_t {
    tick_table_row_t(price_t l, price_t u, price_t i) : lower(l), upper(u), have_upper(true), tick_size(i) { check(); }
    tick_table_row_t(price_t threshold, price_t i) : lower(threshold), upper(0.0), have_upper(false), tick_size(i) { check(); }
    void setUpper(price_t u) {
        //std::cout << "setUpper " << u << std::endl;
        assert(have_upper && "setUpper have_upper already set");
        have_upper = true;
        upper = u; check();
    }

    std::string to_string() const;

    void check() {
        if (lower < 0.0) {
            throw SecurityError("lower < 0.0");
        }
        if (tick_size <= 0.0) {
            throw SecurityError("Invalid tick size");
        }
        if (have_upper && (lower >= upper)) {
            throw SecurityError("Tick lower >= upper");
        }
        if (have_upper && (tick_size > (upper - lower)) ) {
            throw SecurityError("Increment greater than lower-upper");
        }
    }
    price_t lower; // [
    price_t upper; // ) less than
    bool have_upper;
    price_t tick_size;
};



class TickTable {
    public:
        std::string to_string() const;

        void add(tick_table_row_t tr) {
            tr.check();
            if (!rows_.empty()) {
                auto& lastTr = rows_.back();
                if (!lastTr.have_upper) {
                    lastTr.setUpper(tr.lower);
                }
                if (tr.lower > lastTr.upper) {
                    throw SecurityError("Gap in tick table");
                } else if (tr.lower < lastTr.upper) {
                    throw SecurityError("Overlapping ranges in tick table");
                } else {
                    // All good, ranges are contiguous
                }
            }
            rows_.push_back(tr);
        }
        void reset() { rows_.clear(); }

        static price_t roundTo(price_t price, int decimal_places) {
            price_t multiplier = std::pow(10,decimal_places);
            price*=multiplier;
            if( price >= 0.0 ) {
                price = std::floor(price + 0.5);
            }
            price = std::ceil(price - 0.5);
            return price / multiplier;
        }

        static price_t roundToAny(price_t number, price_t tick_size) {
            assert(tick_size >= 0.0 && "roundToAny negative tick_size not allowed");
            if (tick_size != 0.0 && number != 0.0) {
                price_t sign = (number > 0.0) ? 1.0 : -1.0;
                number *= sign;
                number /= tick_size;
                //long fixedPoint = (long) std::floor(number+0.5);
                number = std::floor(number+0.5) * tick_size;
                number *= sign;
            }
            //std::cout << "roundToAny=" << number << std::endl;
            return number;
        }


        bool validatePrice(price_t price) const {
            return validatePriceAndRound(price, false);
        }
        bool validatePriceAndRound(price_t& price, bool round=true ) const {
            assert(price >= 0 && "validatePrice negative not allowed");
            constexpr price_t EPSILON = std::numeric_limits<price_t>::epsilon() * 100.0;
            for (const auto& ttr : rows_) {
                if ( ( ttr.have_upper && (price >= ttr.lower) && (price < ttr.upper)) ||
                     (!ttr.have_upper && (price >= ttr.lower) ) ) {
                    // Found row, now check ladder
                    const price_t div = price / ttr.tick_size;
                    //std::cout << "price=" << price << " inc=" << ttr.tick_size << " div=" << div << " floor=" << std::floor(div) 
                    //          << " minus=" << std::min(div - std::floor(div),std::abs(div - std::ceil(div)))
                    //          << " epilson=" << EPSILON 
                    //          << " res=" << (std::min(div - std::floor(div),std::abs(div - std::ceil(div))) < EPSILON) << std::endl;
                    bool res = std::min(div - std::floor(div),std::abs(div - std::ceil(div))) < EPSILON;
                    if (res && round) {
                        price = roundToAny(price,ttr.tick_size);
                    }
                    return res;
                //} else {
                //    std::cout << "SKIPPED - " << ttr.to_string() << std::endl;
                }
            }
            return false;
        }
    private:
        std::vector<tick_table_row_t> rows_;
};


// https://www.euronext.com/fr/it-documentation/market-data
// Euronext cash tick sizes PROD, https://www.euronext.com/sites/www.euronext.com/files/ftp/NXT_ticksize.txt
class TickLadder {
    public:
        TickLadder() {}
        void addTickLadder(const ladder_id_t id, const TickTable& ladder);
        void loadData(const std::string& filename);
        TickTable* find(ladder_id_t ladder_id) {
            TickTable* ttPtr = nullptr;
            auto search = ladders_.find(ladder_id);
            if (search != ladders_.end()) {
                ttPtr = &(search->second);
            }
            return ttPtr;
        }
        std::string to_string() const;
    private:
        std::map<ladder_id_t, TickTable> ladders_;
};

// **************** SECURITY DATABSE ************************

struct security_record_t {
    security_id_t id;
    location_t    exchange;
    symbol_t      symbol;
    price_t       closing_price;
    shares_t      outstanding_shares;
    date_t        born; // Date
    bool          has_died; // If true next field is non zero.
    date_t        died; // Date
    bool          tradeable;
    ladder_id_t   ladder_id;

    std::string to_string() const;
};

class SecurityDatabase {
    public:
        static const std::size_t npos = std::numeric_limits<std::size_t>::max();

        SecurityDatabase(location_t exchange, TickLadder& tickLadder) 
            : exchange_(exchange), tick_ladder_(tickLadder) {}
        void loadData(const std::string& filename);
        const std::vector<security_record_t>& securities() const {
            return securities_;
        }
        std::size_t find(const symbol_t& symbol) const {
            auto search = symbol_loc_.find(symbol);
            if (search != symbol_loc_.end()) {
                return search->second;
            }
            return npos;
        }
        TickTable* tickTable(std::size_t symbol_idx) {
            assert(symbol_idx < securities_.size() && "symbol_idx out of range");
            return tick_ladder_.find(securities_[symbol_idx].ladder_id);
        }
    private:
        void updateMaps() ;
        location_t exchange_;
        std::vector<security_record_t> securities_;
        std::unordered_map<security_id_t, std::size_t> security_id_loc_;
        std::unordered_map<symbol_t, std::size_t> symbol_loc_;
        TickLadder& tick_ladder_;
};


} // an - namespace

#endif

