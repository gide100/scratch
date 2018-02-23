#include "types.hpp"
#include "fast-cpp-csv-parser/csv.h"
#include "security_master.hpp"
#include <iostream>

std::ostream& operator<<(std::ostream& os, const an::tick_table_t& tt) {
    auto oldFlags = os.flags();
    os << std::left;
    for (auto ttr : tt.rows) {
        auto oldWidth = os.width(8);
        os << ttr.lower; os.width(oldWidth);  
        os << " >= " ;
        if (ttr.have_upper) {
            os << "< " ; os.width(8);
            os << ttr.upper; os.width(oldWidth);
        } else {
            os << "          " ;
        } 
        os << " " << ttr.increment << std::endl;;
    }
    os.flags(oldFlags);
    return os ;
}

void an::SecurityDatabase::loadData(const std::string& filename) {
    io::CSVReader<10> in(filename); // 10 fields
    in.read_header(io::ignore_no_column,
        "id", "exchange", "symbol", "closing_price", "outstanding_shares",
        "born", "has_died", "died", "tradeable", "tick_ladder_id");
    an::security_id_t id; an::location_t exchange; an::symbol_t symbol; an::price_t closing_price; an::shares_t outstanding_shares;
    std::string born; std::string has_died; std::string died; std::string tradeable; an::ladder_id_t tick_ladder_id;
    while(in.read_row(id, exchange, symbol, closing_price, outstanding_shares,
                      born, has_died, died, tradeable, tick_ladder_id) ) {
       std::cout << id << ',' << exchange << '-' << symbol << ',' << an::dateToTimeT(born) << ',' << has_died << std::endl;
    }
}

void an::TickLadder::addTickLadder(const ladder_id_t id, const tick_table_t& ladder) {
    std::cout << "Tick ID = " << id << std::endl;
    std::cout << ladder << std::endl;
    ladders_.insert(std::pair<ladder_id_t, tick_table_t>(id, ladder)); 
}


void an::TickLadder::loadData(const std::string& filename) {
    io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<'|'> > in(filename); // TICKETABLE|THRESHOLD|INCREMENT
    in.read_header(io::ignore_no_column,
        "TICKETABLE","THRESHOLD","INCREMENT" );
    an::ladder_id_t id; an::price_t threshold; an::price_t increment;
    an::tick_table_t tt;
    an::ladder_id_t prev_id = 0;
    while( in.read_row(id, threshold, increment) ) {
        //std::cout << id << ',' << threshold << ',' << increment << std::endl;
        if (prev_id != id) {
            if (prev_id !=0) {
                addTickLadder(prev_id, tt);
            }
            tt.reset();
        }
        an::tick_table_row_t ttr(threshold, increment);;
        tt.add(ttr);
        prev_id = id;
    }
    if (prev_id !=0) {
	addTickLadder(id, tt);
    }
}
