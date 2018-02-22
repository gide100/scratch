#include <iostream>
#include "types.hpp"
#include "order.hpp"
#include "fast-cpp-csv-parser/csv.h"

int main() {
    an::MarketOrder ord1(10,"Client1",an::ME,"APPL",an::BUY,10);
    an::LimitOrder  ord2(11,"Client2",an::ME,"IBM",an::SELL,10,5.12);
    an::CancelOrder ord3(10,"Client1",an::ME);
    an::AmendOrder  ord4(11,"Client2",an::ME);
    an::AmendOrder  ord5(11,"Client2",an::ME, 123.45);
    an::AmendOrder  ord6(11,"Client2",an::ME,an::shares_t(20));
    std::cout << ord1 << std::endl;
    std::cout << ord2 << std::endl;
    std::cout << ord3 << std::endl;
    std::cout << ord4 << std::endl;
    std::cout << ord5 << std::endl;
    std::cout << ord6 << std::endl;
    try {
        an::Order* o10 = an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92.0");
        std::cout << *o10 << std::endl;
        an::Order* o11 = an::Order::makeOrder("type=MARKET:id=123:origin=Client3:destination=ME:symbol=MSFT:direction=SELL:shares=25");
        std::cout << *o11 << std::endl;
        an::Order* o12 = an::Order::makeOrder("type=AMEND:id=123:origin=Client3:destination=ME:shares=30");
        std::cout << *o12 << std::endl;
        an::Order* o13 = an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:");
        std::cout << *o13 << std::endl;
    } catch(an::OrderError& e) {
       std::cout << "ERROR " << e.what() << std::endl;
    }


    io::CSVReader<8> in("security_database.csv");
    in.read_header(io::ignore_no_column, 
        "exchange", "symbol", "closing_price", "outstanding_shares", 
        "born", "died", "tradeable", "tick_ladder_id");
    an::location_t exchange; an::symbol_t symbol; an::price_t closing_price; an::shares_t outstanding_shares; 
    std::string born; std::string died; std::string tradeable; an::ladder_id_t tick_ladder_id;
    while(in.read_row(exchange, symbol, closing_price, outstanding_shares,
                      born, died, tradeable, tick_ladder_id) ) {
       std::cout << exchange << '-' << symbol << std::endl;
    }
    return 0;
}

