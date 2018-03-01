#include <iostream>
#include "types.hpp"
#include "order.hpp"
#include "security_master.hpp"
#include "matching_engine.hpp"

int main() {
    an::MarketOrder ord1a( 1,"Client1",an::ME,"APPL",an::BUY,10);
    an::MarketOrder ord1b( 2,"Client2",an::ME,"APPL",an::SELL,10);
    an::LimitOrder  ord1c( 3,"Client2",an::ME,"APPL",an::SELL,10,12.23);
    an::LimitOrder  ord1d( 4,"Client2",an::ME,"APPL",an::BUY,10,12.23);
    an::LimitOrder  ord2(11,"Client2",an::ME,"IBM",an::SELL,10,5.12);
    an::CancelOrder ord3(10,"Client1",an::ME);
    an::AmendOrder  ord4(11,"Client2",an::ME);
    an::AmendOrder  ord5(11,"Client2",an::ME, 123.45);
    an::AmendOrder  ord6(11,"Client2",an::ME,an::shares_t(20));
    an::Login       ord7("Client2",an::ME);
    std::cout << ord1a << std::endl;
    std::cout << ord1b << std::endl;
    std::cout << ord2 << std::endl;
    std::cout << ord3 << std::endl;
    std::cout << ord4 << std::endl;
    std::cout << ord5 << std::endl;
    std::cout << ord6 << std::endl;
    std::cout << ord7 << std::endl;
    try {
        an::Message* o10 = an::Message::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92.0");
        std::cout << *o10 << std::endl;
        an::Message* o11 = an::Message::makeOrder("type=MARKET:id=123:origin=Client3:destination=ME:symbol=MSFT:direction=SELL:shares=25");
        std::cout << *o11 << std::endl;
        an::Message* o12 = an::Message::makeOrder("type=AMEND:id=123:origin=Client3:destination=ME:shares=30");
        std::cout << *o12 << std::endl;
        an::Message* o13 = an::Message::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:");
        std::cout << *o13 << std::endl;
    } catch(an::OrderError& e) {
       std::cout << "ERROR " << e.what() << std::endl;
    }

    an::SecurityDatabase secdb(an::ME);
    secdb.loadData("security_database.csv");
    for (auto const& s : secdb.securities()) {
        std::cout << s.to_string() << std::endl;
    }

    an::TickLadder tickdb;
    tickdb.loadData("NXT_ticksize.txt");

    an::MatchingEngine me(an::ME, secdb);
    ord1a.applyOrder(me);
    ord1b.applyOrder(me);
    ord1c.applyOrder(me);
    ord1d.applyOrder(me);
 
    std::cout << me.to_string() << std::endl;

    return 0;
}

