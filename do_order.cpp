#include <iostream>
#include "types.hpp"
#include "order.hpp"
#include "security_master.hpp"
#include "matching_engine.hpp"

template <typename T>
std::unique_ptr<T> make_unique_ptr(T* ptr) {
    std::unique_ptr<T> p(ptr);
    return std::move(p);
}

int main() {
    auto lim01a = std::make_unique<an::LimitOrder >(  3,"Client2", an::ME,"APPL",an::SELL,10,12.23);
    auto lim02a = std::make_unique<an::LimitOrder >(  4,"Client2", an::ME,"APPL",an::BUY,10,12.23);
    auto lim03a = std::make_unique<an::LimitOrder >( 11,"Client2", an::ME,"IBM",an::SELL,10,5.12);
    auto mkt01a = std::make_unique<an::MarketOrder>(  1,"Client1", an::ME,"APPL",an::BUY,10);
    auto mkt02a = std::make_unique<an::MarketOrder>(  2,"Client2", an::ME,"APPL",an::SELL,10);
    auto mkt01z = std::make_unique<an::MarketOrder>(100,"Client1", an::ME,"APPL",an::BUY,10);
    auto can01a = std::make_unique<an::CancelOrder>(  4,"Client1", an::ME,"APPL");
    auto amd01a = std::make_unique<an::AmendOrder >( 11,"Client2", an::ME,"APPL");
    auto amd02a = std::make_unique<an::AmendOrder >(  3,"Client2", an::ME,"APPL",123.45);
    auto amd03a = std::make_unique<an::AmendOrder >(  3,"Client2", an::ME,"APPL",an::shares_t(20));
    auto log01a = std::make_unique<an::Login      >(    "Client2", an::ME);
    auto res01a = std::make_unique<an::Response   >( mkt01a.get(), an::ACK, "OK");
    auto res02a = std::make_unique<an::Response   >( lim01a.get(), an::ERROR, "Error occurred");
    auto res03a = std::make_unique<an::Response   >(    "Client1", an::ERROR, "Error occurred");
    auto trr01a = std::make_unique<an::TradeReport>( lim01a.get(), an::BUY, 5, 12.35);
    std::cout << " *** ORDERS ***" << std::endl;
    std::cout << *mkt01a << std::endl;
    std::cout << *mkt02a << std::endl;
    std::cout << *lim03a << std::endl;
    std::cout << *can01a << std::endl;
    std::cout << *amd01a << std::endl;
    std::cout << *amd02a << std::endl;
    std::cout << *amd03a << std::endl;
    std::cout << *log01a << std::endl;
    std::cout << " *** RESPONSE ***" << std::endl;
    std::cout << *res01a << std::endl;
    std::cout << *res02a << std::endl;
    std::cout << *res03a << std::endl;
    std::cout << " *** TRADE REPORT ***" << std::endl;
    std::cout << *trr01a << std::endl;
    std::cout << " *** MAKE ***" << std::endl;
    std::unique_ptr<an::Execution> lim30a(static_cast<an::Execution*>(
        an::Message::makeOrder("type=LIMIT:id=101:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=15:price=92.0")));
    std::unique_ptr<an::Execution> lim30b(static_cast<an::Execution*>(
        an::Message::makeOrder("type=LIMIT:id=102:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=60:price=92.0")));
    std::unique_ptr<an::Execution> lim30c(static_cast<an::Execution*>(
        an::Message::makeOrder("type=LIMIT:id=201:origin=Client2:destination=ME:symbol=MSFT:direction=SELL:shares=10:price=93.0")));
    std::unique_ptr<an::Execution> lim30d(static_cast<an::Execution*>(
        an::Message::makeOrder("type=LIMIT:id=202:origin=Client2:destination=ME:symbol=MSFT:direction=SELL:shares=20:price=91.3")));
    std::unique_ptr<an::Execution> mkt20c(static_cast<an::Execution*>(
        an::Message::makeOrder("type=MARKET:id=203:origin=Client2:destination=ME:symbol=MSFT:direction=SELL:shares=100")));
    std::unique_ptr<an::Execution> mkt20d(static_cast<an::Execution*>(
        an::Message::makeOrder("type=MARKET:id=123:origin=Client3:destination=ME:symbol=MSFT:direction=SELL:shares=25")));
    std::cout << *mkt20d << std::endl;
    std::unique_ptr<an::CancelOrder> can20a(static_cast<an::CancelOrder*>(
        an::Message::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:symbol=APPL")));
    std::cout << *can20a << std::endl;
    auto can30a = make_unique_ptr(new an::CancelOrder(101,"Client1",an::ME,"ENE"));
    std::cout << *can30a << std::endl;
    auto amd10a = make_unique_ptr(new an::AmendOrder(102,"Client1",an::ME,"MSFT",95.5));
    std::cout << *amd10a << std::endl;
    auto amd10b = make_unique_ptr(new an::AmendOrder(201,"Client1",an::ME,"MSFT",94.5));
    std::cout << *amd10b << std::endl;

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
    an::MatchingEngine me(an::ME, secdb, true);
    me.applyOrder(std::move(lim30a));
    me.applyOrder(std::move(lim30b));
    me.applyOrder(std::move(lim30c));
    me.applyOrder(std::move(mkt01z));
    me.applyOrder(std::move(can20a));
    me.applyOrder(std::move(can30a));
    me.applyOrder(std::move(amd10a));
    me.applyOrder(std::move(amd10b));
    std::cout << me.to_string() << std::endl;
    me.close();
 
    std::cout << " *** PRIORITY ***" << std::endl;
    typedef std::priority_queue<an::SideRecord, std::deque<an::SideRecord>, an::CompareSideRecord> MySide;
    MySide side;
    an::SideRecord sr;
    sr = an::DefaultSideRecord; sr.seq=1; sr.price = 12.0; sr.direction = an::BUY; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=2; sr.price = 10.0; sr.direction = an::BUY; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=3; sr.price = 11.0; sr.direction = an::BUY; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=4; sr.price =  9.0; sr.direction = an::BUY; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=5; sr.price = 12.0; sr.direction = an::BUY; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=6; sr.price = 12.0; sr.direction = an::SELL; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr = an::DefaultSideRecord; sr.seq=7; sr.price = 11.0; sr.direction = an::SELL; sr.time = std::chrono::steady_clock::now();
    side.push(sr);
    sr.seq=8; side.push(sr);
    sr.seq=0; side.push(sr);

    std::cout << "===============================" << std::endl;
    while (!side.empty()) {
        std::cout << an::to_string(side.top(), me.epoch()) << std::endl;
        side.pop();
    }
    std::cout << "===============================" << std::endl;


    return 0;
}

