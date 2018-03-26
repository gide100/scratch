#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Matching Engine

#include <boost/test/unit_test.hpp>
#include "types.hpp"
#include "order.hpp"
#include "security_master.hpp"
#include "matching_engine.hpp"


BOOST_AUTO_TEST_SUITE(bookkeeping)
    auto epoch = an::epoch_t{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() };
    BOOST_AUTO_TEST_CASE(bookkeeping_01) {
        an::Bookkeeper bk(1520812800,100.0, epoch);
        //using roundTo = an::TickTable::roundTo;
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        bk.trade(an::BUY, 100, 5.25, epoch.steadyClockStartTime);
        BOOST_CHECK(bk.stats().trades == 1);
        BOOST_CHECK(bk.stats().shares_traded == 100);
        BOOST_CHECK(bk.stats().volume == 100*5.25);
        BOOST_CHECK(bk.stats().trades == 1);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);
        BOOST_CHECK(bk.stats().daily_high == 5.25);
        BOOST_CHECK(bk.stats().daily_low == 5.25);
        BOOST_CHECK(bk.stats().open_price == 5.25);
        BOOST_CHECK(bk.stats().close_price == 0.0);
        BOOST_CHECK(bk.stats().avg_share_price == 0.0);
        BOOST_CHECK(bk.stats().last_trade_price == 5.25);
        BOOST_CHECK(bk.stats().last_trade_time == epoch.steadyClockStartTime);

        bk.cancel();
        bk.amend();
        bk.reject();
        BOOST_CHECK(bk.stats().cancels == 1);
        BOOST_CHECK(bk.stats().amends == 1);
        BOOST_CHECK(bk.stats().rejects == 1);

        an::since_t t =  std::chrono::steady_clock::now();
        bk.trade(an::SELL, 10, 5.15, t);
        BOOST_CHECK(bk.stats().trades == 2);
        BOOST_CHECK(bk.stats().shares_traded == 100+10);
        BOOST_CHECK(bk.stats().volume == 100*5.25+10*5.15);
        BOOST_CHECK(bk.stats().daily_high == 5.25);
        BOOST_CHECK(bk.stats().daily_low == 5.15);
        BOOST_CHECK(bk.stats().open_price == 5.25);
        BOOST_CHECK(bk.stats().last_trade_price == 5.15);
        BOOST_CHECK(bk.stats().last_trade_time == t);

        bk.cancel();
        bk.amend(); bk.amend();
        bk.reject(); bk.reject(); bk.reject();
        BOOST_CHECK(bk.stats().cancels == 2);
        BOOST_CHECK(bk.stats().amends == 3);
        BOOST_CHECK(bk.stats().rejects == 4);

        t =  std::chrono::steady_clock::now();
        bk.trade(an::BUY, 5, 5.35, t);
        BOOST_CHECK(bk.stats().trades == 3);
        BOOST_CHECK(bk.stats().shares_traded == 100+10+5);
        BOOST_CHECK(bk.stats().volume == 5.25*100+5.15*10+5.35*5);
        BOOST_CHECK(bk.stats().daily_high == 5.35);
        BOOST_CHECK(bk.stats().daily_low == 5.15);
        BOOST_CHECK(bk.stats().open_price == 5.25);
        BOOST_CHECK(bk.stats().last_trade_price == 5.35);
        BOOST_CHECK(bk.stats().last_trade_time == t);

        bk.close();
        BOOST_CHECK(bk.stats().close_price == 5.35);
        BOOST_CHECK(bk.stats().avg_share_price ==(5.25*100+5.15*10+5.35*5)/(100+10+5));
    }
    BOOST_AUTO_TEST_CASE(side_01) {
        an::Bookkeeper bk(1520812800, 100.0, epoch);
        an::Side buySide(bk, an::BUY, epoch);
        an::SideRecord* pr1 = nullptr;

        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        an::SideRecord sr1 {
            .id = 1, .seq=1, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::BUY, .price=100.0, .shares=10, .visible=true, .on_book=false };
        buySide.add(sr1);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    10);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10);
        BOOST_CHECK(buySide.top().id      ==    1);

        an::SideRecord sr2 {
            .id = 2, .seq=2, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::BUY, .price=101.0, .shares=5, .visible=true, .on_book=false };
        buySide.add(sr2);
        BOOST_CHECK(bk.stats().buy.trades ==    2);
        BOOST_CHECK(bk.stats().buy.shares ==    15);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10+101.0*5);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5);
        BOOST_CHECK(buySide.top().id      ==    2);


        an::SideRecord sr3 { 
            .id = 3, .seq=3, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::BUY, .price=101.0, .shares=5, .visible=true, .on_book=false };
        buySide.add(sr3);
        BOOST_CHECK(bk.stats().buy.trades ==    3);
        BOOST_CHECK(bk.stats().buy.shares ==    20);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10+101.0*5*2);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5*2);
        BOOST_CHECK(buySide.top().id      ==    2);

        an::SideRecord sr4 { // Same price earlier time
            .id = 4, .seq=4, .time = epoch.steadyClockStartTime, .order_type=an::LIMIT,
            .direction=an::BUY, .price=101.0, .shares=5, .visible=true, .on_book=false };
        buySide.add(sr4);
        BOOST_CHECK(bk.stats().buy.trades ==    4);
        BOOST_CHECK(bk.stats().buy.shares ==    25);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10+101.0*5*3);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5*3);
        BOOST_CHECK(buySide.top().id      ==    4);

        pr1 = buySide.findRecord(4);
        buySide.remove(*pr1);
        BOOST_CHECK(bk.stats().buy.trades ==    3);
        BOOST_CHECK(bk.stats().buy.shares ==    20);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10+101.0*5*2);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5*3);
        BOOST_CHECK(buySide.top().id      ==    2);

        pr1 = buySide.findRecord(1);
        buySide.remove(*pr1);
        BOOST_CHECK(bk.stats().buy.trades ==    2);
        BOOST_CHECK(bk.stats().buy.shares ==    10);
        BOOST_CHECK(bk.stats().buy.value  ==    101.0*5*2);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5*3);
        BOOST_CHECK(buySide.top().id      ==    2);
    }
    BOOST_AUTO_TEST_CASE(side_02) {
        an::Bookkeeper bk(1520812800, 100.0, epoch);
        an::Side sellSide(bk, an::SELL, epoch);
        an::SideRecord* pr1 = nullptr;

        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        an::SideRecord sr1 {
            .id = 1, .seq=1, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::SELL, .price=100.0, .shares=10, .visible=true, .on_book=false };
        sellSide.add(sr1);
        BOOST_CHECK(bk.stats().sell.trades ==    1);
        BOOST_CHECK(bk.stats().sell.shares ==    10);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10);
        BOOST_CHECK(sellSide.top().id      ==    1);

        an::SideRecord sr2 {
            .id = 2, .seq=2, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::SELL, .price=99.0, .shares=5, .visible=true, .on_book=false };
        sellSide.add(sr2);
        BOOST_CHECK(bk.stats().sell.trades ==    2);
        BOOST_CHECK(bk.stats().sell.shares ==    15);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10+99.0*5);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10+99.0*5);
        BOOST_CHECK(sellSide.top().id      ==    2);


        an::SideRecord sr3 { 
            .id = 3, .seq=3, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::SELL, .price=99.0, .shares=5, .visible=true, .on_book=false };
        sellSide.add(sr3);
        //std::cout << sellSide.to_string(false) << std::endl;
        BOOST_CHECK(bk.stats().sell.trades ==    3);
        BOOST_CHECK(bk.stats().sell.shares ==    20);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10+99.0*5*2);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10+99.0*5*2);
        BOOST_CHECK(sellSide.top().id      ==    2);

        an::SideRecord sr4 { // Same price earlier time
            .id = 4, .seq=4, .time = epoch.steadyClockStartTime, .order_type=an::LIMIT,
            .direction=an::SELL, .price=99.0, .shares=5, .visible=true, .on_book=false };
        sellSide.add(sr4);
        BOOST_CHECK(bk.stats().sell.trades ==    4);
        BOOST_CHECK(bk.stats().sell.shares ==    25);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10+99.0*5*3);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10+99.0*5*3);
        BOOST_CHECK(sellSide.top().id      ==    4);

        pr1 = sellSide.findRecord(4);
        sellSide.remove(*pr1);
        BOOST_CHECK(bk.stats().sell.trades ==    3);
        BOOST_CHECK(bk.stats().sell.shares ==    20);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10+99.0*5*2);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10+99.0*5*3);
        BOOST_CHECK(sellSide.top().id      ==    2);

        pr1 = sellSide.findRecord(1);
        sellSide.remove(*pr1);
        BOOST_CHECK(bk.stats().sell.trades ==    2);
        BOOST_CHECK(bk.stats().sell.shares ==    10);
        BOOST_CHECK(bk.stats().sell.value  ==    99.0*5*2);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10+99.0*5*3);
        BOOST_CHECK(sellSide.top().id      ==    2);
    }
    BOOST_AUTO_TEST_CASE(buy_side_amend_01) {
        an::Bookkeeper bk(1520812800, 100.0, epoch);
        an::Side buySide(bk, an::BUY, epoch);
        an::shares_t shr = 0;
        an::SideRecord* pr1 = nullptr;

        an::SideRecord sr1 {
            .id = 1, .seq=1, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::BUY, .price=100.0, .shares=10, .visible=true, .on_book=false };
        buySide.add(sr1);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    10);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10);
        BOOST_CHECK(buySide.top().id      ==    1);

        pr1 = buySide.findRecord(1);
        shr = pr1->shares; pr1->shares = 20;
        buySide.amendShares(*pr1, shr);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    20);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*20);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*20);
        BOOST_CHECK(buySide.top().id      ==    1);
        
        pr1 = buySide.findRecord(1);
        shr = pr1->shares; pr1->shares = 5;
        buySide.amendShares(*pr1, shr);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    5);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*5);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*5);
        BOOST_CHECK(buySide.top().id      ==    1);
    }
    BOOST_AUTO_TEST_CASE(buy_side_amend_02) { // Amend price
        an::Bookkeeper bk(1520812800, 100.0, epoch);
        an::Side buySide(bk, an::BUY, epoch);
        an::shares_t shr = 0;
        an::SideRecord* ptr1 = nullptr; 

        an::SideRecord sr1 {
            .id = 1, .seq=1, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::BUY, .price=100.0, .shares=10, .visible=true, .on_book=false };
        buySide.add(sr1);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    10);
        BOOST_CHECK(bk.stats().buy.value  ==    100.0*10);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10);
        BOOST_CHECK(buySide.top().id      ==    1);

        ptr1 = buySide.findRecord(1);
        buySide.remove(*ptr1);
        sr1.price = 101.0; sr1.shares = 15;
        buySide.add(sr1);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    15);
        BOOST_CHECK(bk.stats().buy.value  ==    101.0*15);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*15);
        BOOST_CHECK(buySide.top().id      ==    1);
        
        ptr1 = buySide.findRecord(1);
        shr = ptr1->shares; ptr1->shares = 5;
        buySide.amendShares(*ptr1, shr);
        BOOST_CHECK(bk.stats().buy.trades ==    1);
        BOOST_CHECK(bk.stats().buy.shares ==    5);
        BOOST_CHECK(bk.stats().buy.value  ==    101.0*5);
        BOOST_CHECK(bk.stats().buy.volume ==    100.0*10+101.0*5);
        BOOST_CHECK(buySide.top().id      ==    1);
    }
    BOOST_AUTO_TEST_CASE(sell_side_amend_02) {
        an::Bookkeeper bk(1520812800, 100.0, epoch);
        an::Side sellSide(bk, an::SELL, epoch);
        an::shares_t shr = 0;
        an::SideRecord* pr1 = nullptr;

        an::SideRecord sr1 {
            .id = 1, .seq=1, .time = std::chrono::steady_clock::now(), .order_type=an::LIMIT,
            .direction=an::SELL, .price=100.0, .shares=10, .visible=true, .on_book=false };
        sellSide.add(sr1);
        BOOST_CHECK(bk.stats().sell.trades ==    1);
        BOOST_CHECK(bk.stats().sell.shares ==    10);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*10);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*10);
        BOOST_CHECK(sellSide.top().id      ==    1);

        pr1 = sellSide.findRecord(1);
        shr = pr1->shares; pr1->shares = 20;
        sellSide.amendShares(*pr1, shr);
        BOOST_CHECK(bk.stats().sell.trades ==    1);
        BOOST_CHECK(bk.stats().sell.shares ==    20);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*20);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*20);
        BOOST_CHECK(sellSide.top().id      ==    1);
        
        pr1 = sellSide.findRecord(1);
        shr = pr1->shares; pr1->shares = 5;
        sellSide.amendShares(*pr1, shr);
        BOOST_CHECK(bk.stats().sell.trades ==    1);
        BOOST_CHECK(bk.stats().sell.shares ==    5);
        BOOST_CHECK(bk.stats().sell.value  ==    100.0*5);
        BOOST_CHECK(bk.stats().sell.volume ==    100.0*5);
        BOOST_CHECK(sellSide.top().id      ==    1);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(book)
    auto epoch = an::epoch_t{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() };

    BOOST_AUTO_TEST_CASE(book_limit_cancel_amend_01) {
        an::sequence_t seq = 0;
        an::order_id_t id = 0;
        an::SideRecord rec;
        an::TickTable tt;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);
        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        BOOST_CHECK( book.matchSymbol("APPL"));
        BOOST_CHECK(!book.matchSymbol("IBM"));

        // Open book
        book.open();

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::SELL,10,172.00);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().sell.trades  == 1);
        BOOST_CHECK(bk.stats().rejects      == 0);

        auto lim01b = std::make_unique<an::LimitOrder >(  2,"Client2", an::ME,"APPL",an::BUY,10,170.00);
        lim01b->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        BOOST_CHECK(bk.stats().buy.trades   == 0);
        book.executeOrder(rec, std::move(lim01b));
        BOOST_CHECK(bk.stats().sell.trades  == 1);
        BOOST_CHECK(bk.stats().buy.trades   == 1); // All good, buy executed
        BOOST_CHECK(bk.stats().rejects      == 0);

        auto can01a = std::make_unique<an::CancelOrder>(  1,"Client1", an::ME,"APPL");
        id = can01a->orderId();
        book.cancelActiveOrder(id, std::move(can01a));
        BOOST_CHECK(bk.stats().sell.trades  == 0);
        BOOST_CHECK(bk.stats().cancels      == 1);

        BOOST_CHECK(bk.stats().buy.shares   == 10);
        auto amd01a = std::make_unique<an::AmendOrder>(  2,"Client2", an::ME,"APPL", an::shares_t(20));
        id = amd01a->orderId();
        book.amendActiveOrder(id, std::move(amd01a));
        BOOST_CHECK(bk.stats().buy.trades   == 1);
        BOOST_CHECK(bk.stats().buy.shares   == 20);
        BOOST_CHECK(bk.stats().buy.value    == 170.0*20);
        BOOST_CHECK(bk.stats().cancels      == 1);
        BOOST_CHECK(bk.stats().amends       == 1);
        BOOST_CHECK(bk.stats().rejects      == 0);

        auto amd02a = std::make_unique<an::AmendOrder>(  2,"Client2", an::ME,"APPL", 175.0);
        id = amd02a->orderId();
        book.amendActiveOrder(id, std::move(amd02a));
        BOOST_CHECK(bk.stats().buy.trades   == 1);
        BOOST_CHECK(bk.stats().buy.shares   == 20);
        BOOST_CHECK(bk.stats().buy.value    == 175.0*20);
        BOOST_CHECK(bk.stats().cancels      == 1);
        BOOST_CHECK(bk.stats().amends       == 2);
        BOOST_CHECK(bk.stats().rejects      == 0);

        auto amd03a = std::make_unique<an::AmendOrder>(  2,"Client2", an::ME,"APPL", 165.0);
        id = amd03a->orderId();
        book.amendActiveOrder(id, std::move(amd03a));
        BOOST_CHECK(bk.stats().buy.trades   == 1);
        BOOST_CHECK(bk.stats().buy.shares   == 20);
        BOOST_CHECK(bk.stats().buy.value    == 165.0*20);
        BOOST_CHECK(bk.stats().cancels      == 1);
        BOOST_CHECK(bk.stats().amends       == 3);
        BOOST_CHECK(bk.stats().rejects      == 0);

        book.close();
    }
    BOOST_AUTO_TEST_CASE(book_limit_market_01) {
        an::sequence_t seq = 0;
        an::TickTable tt;
        an::SideRecord rec;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);

        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);
 
        // Open book
        book.open();

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::SELL,10,172.00);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().sell.trades  == 1);
        BOOST_CHECK(bk.stats().cancels      == 0);
        BOOST_CHECK(bk.stats().rejects      == 0);

        //Market Order
        auto mkt01a = std::make_unique<an::MarketOrder >(  2,"Client2", an::ME,"APPL",an::SELL, 10);
        mkt01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(mkt01a));
        BOOST_CHECK(bk.stats().sell.trades  == 1);
        BOOST_CHECK(bk.stats().cancels      == 1); // Reject, no BID
        BOOST_CHECK(bk.stats().rejects      == 0);


        auto mkt01b = std::make_unique<an::MarketOrder >(  3,"Client2", an::ME,"APPL",an::BUY, 10);
        mkt01b->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(mkt01b));
        BOOST_CHECK(bk.stats().shares_traded== 20); 
        BOOST_CHECK(bk.stats().volume       == 172.0*10*2); 
        BOOST_CHECK(bk.stats().trades       == 2); 
        BOOST_CHECK(bk.stats().cancels      == 1); 
        BOOST_CHECK(bk.stats().rejects      == 0);
        BOOST_CHECK(bk.stats().sell.trades  == 0); // Filled
        BOOST_CHECK(bk.stats().buy.trades   == 0); // Filled

        book.close();
    }
    BOOST_AUTO_TEST_CASE(book_limit_market_02) {
        an::sequence_t seq = 0;
        an::TickTable tt;
        an::SideRecord rec;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);

        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);
 
        // Open book, orders should be ok now
        book.open();

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::BUY,10,172.00);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().buy.trades   == 1);
        BOOST_CHECK(bk.stats().cancels      == 0);
        BOOST_CHECK(bk.stats().rejects      == 0);

        //Market Order
        auto mkt01a = std::make_unique<an::MarketOrder >(  2,"Client2", an::ME,"APPL",an::BUY, 10);
        mkt01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(mkt01a));
        BOOST_CHECK(bk.stats().buy.trades   == 1);
        BOOST_CHECK(bk.stats().cancels      == 1); // Cancelled, no BID
        BOOST_CHECK(bk.stats().rejects      == 0);

        std::cout << book.to_string(true) << std::endl;
        auto mkt01b = std::make_unique<an::MarketOrder >(  3,"Client2", an::ME,"APPL",an::SELL, 5);
        mkt01b->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(mkt01b));
        BOOST_CHECK(bk.stats().shares_traded== 10); 
        BOOST_CHECK(bk.stats().volume       == 172.0*5*2); 
        BOOST_CHECK(bk.stats().trades       == 2); 
        BOOST_CHECK(bk.stats().cancels      == 1); 
        BOOST_CHECK(bk.stats().rejects      == 0);
        BOOST_CHECK(bk.stats().sell.trades  == 0); // Trade didn't make it to book (marketable) 
        BOOST_CHECK(bk.stats().sell.volume  == 0); // Ditto
        BOOST_CHECK(bk.stats().buy.trades   == 1); // Part filled
        BOOST_CHECK(bk.stats().buy.shares   == 5); // Part filled
        BOOST_CHECK(bk.stats().buy.volume   == 172.0 * 5); // Filled
        BOOST_CHECK(bk.stats().buy.value    == 172.0*5); 

        BOOST_CHECK(bk.stats().cancels      == 1);
        book.close();
        BOOST_CHECK(bk.stats().cancels      == 2); // Active order cancelled
    }
    BOOST_AUTO_TEST_CASE(book_limit_limit_01) {
        an::sequence_t seq = 0;
        an::TickTable tt;
        an::SideRecord rec;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);

        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        // Open book, orders should be ok now
        book.open();

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::BUY,10,172.00);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().buy.trades    == 1);
        BOOST_CHECK(bk.stats().sell.trades   == 0);

        auto lim02a = std::make_unique<an::LimitOrder >(  2,"Client1", an::ME,"APPL",an::SELL,10,172.00);
        lim02a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim02a));
        BOOST_CHECK(bk.stats().volume == 172.0 * 10 * 2);
        BOOST_CHECK(bk.stats().trades == 2);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().buy.trades    == 0);
        BOOST_CHECK(bk.stats().buy.volume    == 172.0 * 10);
        BOOST_CHECK(bk.stats().sell.trades   == 0);
        BOOST_CHECK(bk.stats().sell.volume   == 0.0);  // Sell not added to book, as it is markerable.

        book.close();
    }
    BOOST_AUTO_TEST_CASE(book_limit_limit_amend_01) {
        an::sequence_t seq = 0;
        an::order_id_t id = 0;
        an::TickTable tt;
        an::SideRecord rec;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);

        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().shares_traded == 0);
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().rejects == 0);

        // Open book, orders should be ok now
        book.open();

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::BUY,10,172.0);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().buy.trades    == 1);
        BOOST_CHECK(bk.stats().sell.trades   == 0);

        auto lim02a = std::make_unique<an::LimitOrder >(  2,"Client2", an::ME,"APPL",an::SELL,10,174.0);
        lim02a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim02a));
        BOOST_CHECK(bk.stats().volume == 0);
        BOOST_CHECK(bk.stats().trades == 0);
        BOOST_CHECK(bk.stats().cancels == 0);
        BOOST_CHECK(bk.stats().amends == 0);
        BOOST_CHECK(bk.stats().buy.trades    == 1);
        BOOST_CHECK(bk.stats().buy.volume    == 172.0 * 10);
        BOOST_CHECK(bk.stats().sell.trades   == 1);
        BOOST_CHECK(bk.stats().sell.volume   == 174.0 * 10);

        auto amd01a = std::make_unique<an::AmendOrder>(  1,"Client1", an::ME,"APPL", 175.0);
        id = amd01a->orderId();
        book.amendActiveOrder(id, std::move(amd01a));
        BOOST_CHECK(bk.stats().cancels       == 0);
        BOOST_CHECK(bk.stats().amends        == 1);
        BOOST_CHECK(bk.stats().rejects       == 0);
        BOOST_CHECK(bk.stats().buy.trades    == 0);
        BOOST_CHECK(bk.stats().buy.shares    == 0);
        BOOST_CHECK(bk.stats().buy.value     == 0);
        BOOST_CHECK(bk.stats().sell.trades   == 0);
        BOOST_CHECK(bk.stats().sell.shares   == 0);
        BOOST_CHECK(bk.stats().sell.value    == 0);
        BOOST_CHECK(bk.stats().buy.volume    == 174.0 * 10);
        BOOST_CHECK(bk.stats().sell.volume   == 174.0 * 10);

        book.close();
    }
 
    BOOST_AUTO_TEST_CASE(book_limit_reject_01) {
        an::sequence_t seq = 0;
        an::TickTable tt;
        an::SideRecord rec;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));
        const an::price_t prev_close = 171.07;

        an::Book book("APPL", epoch, tt, true, prev_close);

        const an::Bookkeeper& bk = book.bookkeeper();
        BOOST_CHECK(bk.stats().rejects == 0);
        BOOST_CHECK(bk.stats().volume  == 0);

        auto lim01a = std::make_unique<an::LimitOrder >(  1,"Client1", an::ME,"APPL",an::SELL,10,172.00);
        lim01a->pack(rec);
        rec.time = std::chrono::steady_clock::now();
        rec.seq = ++seq;
        rec.visible = true;
        book.executeOrder(rec, std::move(lim01a));
        BOOST_CHECK(bk.stats().sell.trades  == 0);
        BOOST_CHECK(bk.stats().rejects      == 1); // Book not open
        BOOST_CHECK(bk.stats().volume       == 0);
 
        // Open book, orders should be ok now
        book.open();

        book.close();
    }
BOOST_AUTO_TEST_SUITE_END()

struct CheckMessage {
    explicit CheckMessage(std::string const& str) : str_(str) { }
    bool operator()(const std::exception& ex) const {
        bool ok = ex.what() == str_; 
        if (!ok) {
            BOOST_TEST_MESSAGE("Got [" << ex.what() << "] != Expected {" << str_ <<"]");
        }
        return ok;
    }
    std::string str_;
};

BOOST_AUTO_TEST_SUITE(matching_engine_01)
    BOOST_AUTO_TEST_CASE(secdb_01) {
    }
    BOOST_AUTO_TEST_CASE(tick_01) {
    }
BOOST_AUTO_TEST_SUITE_END()

