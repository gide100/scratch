#ifndef AN_SECURITY_MASTER_HPP
#define AN_SECURITY_MASTER_HPP

#include "types.hpp"

namespace an {

using std::runtime_error;
class SecurityError : public runtime_error {
    public:
        SecurityError(const std::string& msg) : runtime_error(msg) {}
};

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

struct tick_table_row_t {
    tick_table_row_t(price_t l, price_t u, price_t i) : lower(l), upper(u), have_upper(true), increment(i) { check(); }
    tick_table_row_t(price_t threshold, price_t i) : lower(threshold), upper(0.0), have_upper(false), increment(i) { check(); }
    void setUpper(price_t u) {
        //std::cout << "setUpper " << u << std::endl;
        have_upper = true;
        upper = u; check(); 
    }

    std::string to_string() const;

    void check() {
        if (lower < 0.0) {
            throw SecurityError("lower < 0.0");
        }
        if (increment <= 0.0) {
            throw SecurityError("Invalid increment");
        }
        if (have_upper && (lower >= upper)) {
            throw SecurityError("Tick lower >= upper");
        }
        if (have_upper && (increment > (upper - lower)) ) {
            throw SecurityError("increment greater than lower-upper");
        }
        if ((!have_upper && upper !=0.0) || (have_upper && upper==0.0)) {
            throw SecurityError("upper invalid");
        }
    }
    price_t lower; // [
    price_t upper; // ) less than
    bool have_upper;
    price_t increment;
};



struct tick_table_t {
    void add(tick_table_row_t tr) { 
        if (!rows.empty()) {
            auto& lastTr = rows.back();
            lastTr.setUpper(tr.lower);
        }
        rows.push_back(tr); 
    }
    void reset() { rows.clear(); }
    std::vector<tick_table_row_t> rows;
};


class SecurityDatabase {
    public:
        static const std::size_t npos = std::numeric_limits<std::size_t>::max();

        SecurityDatabase(location_t exchange) : exchange_(exchange) {}
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
    private:
        void updateMaps() ;
        location_t exchange_;
        std::vector<security_record_t> securities_;
        std::unordered_map<security_id_t, std::size_t> security_id_loc_;
        std::unordered_map<symbol_t, std::size_t> symbol_loc_;
};

// https://www.euronext.com/fr/it-documentation/market-data
// Euronext cash tick sizes PROD, https://www.euronext.com/sites/www.euronext.com/files/ftp/NXT_ticksize.txt
class TickLadder {
    public:
        TickLadder() {}
        void addTickLadder(const ladder_id_t id, const tick_table_t& ladder);
        void loadData(const std::string& filename);
    private:
        std::map<ladder_id_t, tick_table_t> ladders_;
};

} // an - namespace

std::ostream& operator<<(std::ostream& os, const an::tick_table_t& tt);

#endif
