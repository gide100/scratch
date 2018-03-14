#include "types.hpp"
#include "fast-cpp-csv-parser/csv.h"
#include "security_master.hpp"
#include <iostream>


std::string  an::security_record_t::to_string() const {
    std::stringstream ss;
    ss << id << ',' << symbol << ',' << closing_price << ',' << outstanding_shares
       << ',' << dateToString(born) ;
    if (has_died) {
        ss << ",Y," << dateToString(died) ;
    } else {
        ss << ",N,0000-00-00" ;
    }
    ss << ',' << (tradeable ? 'Y' : 'N') << ',' << ladder_id ;

    return ss.str();
}


std::string an::tick_table_row_t::to_string() const {
    std::stringstream ss;
    ss << std::left;
    auto oldWidth = ss.width(8);
    ss << lower; ss.width(oldWidth);
    ss << " >= " ;
    if (have_upper) {
        ss << "< " ; ss.width(8);
        ss << upper; ss.width(oldWidth);
    } else {
        ss << "          " ;
    }
    ss << " " << increment ;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const an::tick_table_t& tt) {
    for (auto ttr : tt.rows) {
        os << ttr.to_string() << std::endl;
    }
    return os ;
}

void an::SecurityDatabase::loadData(const std::string& filename) {
    io::CSVReader<10> in(filename); // 10 fields
    in.read_header(io::ignore_no_column,
        "id", "exchange", "symbol", "closing_price", "outstanding_shares",
        "born", "has_died", "died", "tradeable", "tick_ladder_id");
    std::string born; char has_died; std::string died; char tradeable;
    an::security_record_t sr;
    while(in.read_row(sr.id, sr.exchange, sr.symbol, sr.closing_price, sr.outstanding_shares,
                      born, has_died, died, tradeable, sr.ladder_id) ) {
       sr.born = an::dateToTimeT(born);
       sr.has_died = has_died == 'Y';
       if (sr.has_died) {
           sr.died = an::dateToTimeT(died);
       } else {
           sr.died = an::MY_MAX_DATE;
       }
       sr.tradeable = tradeable == 'Y';
       securities_.push_back(sr);
       // std::cout << sr.to_string() << std::endl;
    }
    updateMaps();
}

void an::SecurityDatabase::updateMaps() {
    security_id_loc_.clear();
    symbol_loc_.clear();
    std::size_t i = 0;
    for (const auto& sec: securities_) {
        // Just index live securities on this exchange
        if (!sec.has_died && (sec.exchange == exchange_) ) {
            security_id_loc_.emplace(sec.id, i);
            symbol_loc_.emplace(sec.symbol, i);
        }
        ++i;
    }
    std::cout << "DEBUG MSFT=" <<  symbol_loc_.find("MSFT")->second << std::endl ;
}

void an::TickLadder::addTickLadder(const ladder_id_t id, const tick_table_t& ladder) {
    //std::cout << "Tick ID = " << id << std::endl;
    //std::cout << ladder << std::endl;
    ladders_.emplace(id, ladder);
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
    // Add the last id in list
    if (prev_id !=0) {
        addTickLadder(id, tt);
    }
}
