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
    an::LimitOrder  ord2( 11,"Client2",an::ME,"IBM",an::SELL,10,5.12);
    an::CancelOrder ord3(  4,"Client1",an::ME,"APPL");
    an::AmendOrder  ord4( 11,"Client2",an::ME,"APPL");
    an::AmendOrder  ord5(  3,"Client2",an::ME,"APPL",123.45);
    an::AmendOrder  ord6(  2,"Client2",an::ME,"APPL",an::shares_t(20));
    an::Login       ord7( "Client2",an::ME);
    an::Response    res1( &ord1a, an::ACK, "OK");
    an::Response    res2( &ord1c, an::ERROR, "Error occurred");
    an::TradeReport tr1( &ord1c, an::BUY, 5, 12.35);
    std::cout << " *** ORDERS ***" << std::endl;
    std::cout << ord1a << std::endl;
    std::cout << ord1b << std::endl;
    std::cout << ord2 << std::endl;
    std::cout << ord3 << std::endl;
    std::cout << ord4 << std::endl;
    std::cout << ord5 << std::endl;
    std::cout << ord6 << std::endl;
    std::cout << ord7 << std::endl;
    std::cout << " *** RESPONSE ***" << std::endl;
    std::cout << res1 << std::endl;
    std::cout << res2 << std::endl;
    std::cout << " *** TRADE REPORT ***" << std::endl;
    std::cout << tr1 << std::endl;
    try {
        std::cout << " *** MAKE ***" << std::endl;
        an::Message* o10 = an::Message::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92.0");
        std::cout << *o10 << std::endl;
        an::Message* o11 = an::Message::makeOrder("type=MARKET:id=123:origin=Client3:destination=ME:symbol=MSFT:direction=SELL:shares=25");
        std::cout << *o11 << std::endl;
        an::Message* o12 = an::Message::makeOrder("type=AMEND:id=123:origin=Client3:destination=ME:symbol=APPL:shares=30");
        std::cout << *o12 << std::endl;
        an::Message* o13 = an::Message::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:symbol=APPL");
        std::cout << *o13 << std::endl;
    } catch(an::OrderError& e) {
       std::cout << "ERROR " << e.what() << std::endl;
    }

    std::cout << " *** SECDB ***" << std::endl;
    an::SecurityDatabase secdb(an::ME);
    secdb.loadData("security_database.csv");
    for (auto const& s : secdb.securities()) {
        std::cout << s.to_string() << std::endl;
    }

    std::cout << " *** TICK ***" << std::endl;
    an::TickLadder tickdb;
    tickdb.loadData("NXT_ticksize.txt");

    std::cout << " *** ME ***" << std::endl;
    an::MatchingEngine me(an::ME, secdb);
    ord1d.applyOrder(me);
    ord1a.applyOrder(me);
    ord1b.applyOrder(me);
    ord1c.applyOrder(me);
    ord3.applyOrder(me);
    ord5.applyOrder(me);
    ord6.applyOrder(me);
    ord2.applyOrder(me);
 
    std::cout << me.to_string() << std::endl;

    return 0;
}

