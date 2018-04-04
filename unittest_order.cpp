#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Orders

#include <boost/test/unit_test.hpp>
#include "order.hpp"

BOOST_AUTO_TEST_SUITE(all_orders)
    BOOST_AUTO_TEST_CASE(limit_order01) {
        an::LimitOrder lo1(11,"Client2",an::ME,"IBM",an::SELL,10,5.12);
        BOOST_CHECK_EQUAL(lo1.to_string(),"type=LIMIT:id=11:origin=Client2:destination=ME:symbol=IBM:direction=SELL:shares=10:price=5.12");
        an::LimitOrder lo2(1234,"ClientA",an::ME,"APPL",an::BUY,99,15.12);
        BOOST_CHECK_EQUAL(lo2.to_string(),"type=LIMIT:id=1234:origin=ClientA:destination=ME:symbol=APPL:direction=BUY:shares=99:price=15.12");
    }
    BOOST_AUTO_TEST_CASE(market_order01) {
        an::MarketOrder lo1(11,"Client2",an::ME,"IBM",an::SELL,10);
        BOOST_CHECK_EQUAL(lo1.to_string(),"type=MARKET:id=11:origin=Client2:destination=ME:symbol=IBM:direction=SELL:shares=10");
        an::MarketOrder lo2(1234,"ClientA",an::ME,"APPL",an::BUY,99);
        BOOST_CHECK_EQUAL(lo2.to_string(),"type=MARKET:id=1234:origin=ClientA:destination=ME:symbol=APPL:direction=BUY:shares=99");
    }
    BOOST_AUTO_TEST_CASE(cancel_order01) {
        an::CancelOrder lo1(11,"Client2",an::ME,"APPL");
        BOOST_CHECK_EQUAL(lo1.to_string(),"type=CANCEL:id=11:origin=Client2:destination=ME:symbol=APPL");
        an::CancelOrder lo2(1234,"ClientA","MatchingEngine2","APPL");
        BOOST_CHECK_EQUAL(lo2.to_string(),"type=CANCEL:id=1234:origin=ClientA:destination=MatchingEngine2:symbol=APPL");
    }
    BOOST_AUTO_TEST_CASE(amend_order01) {
        an::AmendOrder lo1(11,"Client2",an::ME,"APPL",99.99);
        BOOST_CHECK_EQUAL(lo1.to_string(),"type=AMEND:id=11:origin=Client2:destination=ME:symbol=APPL:price=99.99");
        an::AmendOrder lo2(1234,"ClientA",an::ME,"APPL",an::shares_t(999));
        BOOST_CHECK_EQUAL(lo2.to_string(),"type=AMEND:id=1234:origin=ClientA:destination=ME:symbol=APPL:shares=999");
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(make_orders)
    BOOST_AUTO_TEST_CASE(limit_order01) {
        an::Message* o1 = an::Order::makeOrder("origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50:type=LIMIT:id=123");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92.0");
    }
    BOOST_AUTO_TEST_CASE(market_order01) {
        an::Message* o1 = an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
    }
    BOOST_AUTO_TEST_CASE(cancel_order01) {
        an::Message* o1 = an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:symbol=APPL");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=CANCEL:id=123:origin=Client1:destination=ME:symbol=APPL");
    }
    BOOST_AUTO_TEST_CASE(amend_order01) {
        an::Message* o1 = an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:price=99.99");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:price=99.99");
        an::Message* o2 = an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:shares=99");
        BOOST_CHECK_EQUAL(o2->to_string(),"type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:shares=99");
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(replies)
    BOOST_AUTO_TEST_CASE(response_01) {
        an::Message* o1 = an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
        an::Response res1(o1,an::ACK,"OK");
        BOOST_CHECK_EQUAL(res1.to_string(),"type=MARKET:id=123:origin=ME:destination=Client1:symbol=MSFT:direction=BUY:shares=50:response=ACK:text=OK");
    }
    BOOST_AUTO_TEST_CASE(response_02) {
        an::Response res1(an::ME,"Client1",an::ACK,"OK");
        BOOST_CHECK_EQUAL(res1.to_string(),"type=REPLY:origin=ME:destination=Client1:response=ACK:text=OK");
        an::Response res2("FTSE","Client2",an::ERROR,"Bad stuff");
        BOOST_CHECK_EQUAL(res2.to_string(),"type=REPLY:origin=FTSE:destination=Client2:response=ERROR:text=Bad stuff");
    }
    BOOST_AUTO_TEST_CASE(trade_report_01) {
        auto mkt01 = static_cast<an::MarketOrder*>(an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50"));
        BOOST_CHECK_EQUAL(mkt01->to_string(),"type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
        an::TradeReport trr01(mkt01,an::BUY, 50, 100.0);
        BOOST_CHECK_EQUAL(trr01.to_string(),"type=TRADE:origin=ME:destination=Client1:orig_order_id=123:symbol=MSFT:direction=BUY:shares=50:price=100.0");
        an::TradeReport trr02(mkt01,an::SELL, 10, 101.0);
        BOOST_CHECK_EQUAL(trr02.to_string(),"type=TRADE:origin=ME:destination=Client1:orig_order_id=123:symbol=MSFT:direction=SELL:shares=10:price=101.0");
    }
    BOOST_AUTO_TEST_CASE(market_data_01) {
        an::market_data_t quote{ 
            .origin=an::ME, .symbol="MSFT", 
            .have_bid=true, .bid=100.0, .bid_size=100, 
            .have_ask=true, .ask=101.0, .ask_size=10, 
            .have_last_trade=true, .last_trade_price=100.10, .last_trade_shares=50,
            .trade_time="2018-01-01 12:00:00.00000", .quote_time="2018-01-01 12:01:00.00000",
            .volume=100.10*50 };
        an::MarketData md1(an::ME, quote);
        BOOST_CHECK_EQUAL(md1.to_string(), "type=MARKETDATA:origin=ME:destination=:symbol=MSFT:bid=100.0:bid_size=100:ask=101.0:ask_size=10:last_trade_price=100.1:last_trade_shares=50:trade_time=2018-01-01 12:00:00.00000:quote_time=2018-01-01 12:01:00.00000:volume=5005.0");
        quote.have_bid = false; quote.have_ask = false;
        an::MarketData md2(an::ME, quote);
        BOOST_CHECK_EQUAL(md2.to_string(), "type=MARKETDATA:origin=ME:destination=:symbol=MSFT:bid=N/A:bid_size=N/A:ask=N/A:ask_size=N/A:last_trade_price=100.1:last_trade_shares=50:trade_time=2018-01-01 12:00:00.00000:quote_time=2018-01-01 12:01:00.00000:volume=5005.0");
        quote.have_last_trade = false; quote.volume = 0.0; 
        an::MarketData md3(an::ME, quote);
        BOOST_CHECK_EQUAL(md3.to_string(), "type=MARKETDATA:origin=ME:destination=:symbol=MSFT:bid=N/A:bid_size=N/A:ask=N/A:ask_size=N/A:last_trade_price=N/A:last_trade_shares=N/A:trade_time=N/A:quote_time=2018-01-01 12:01:00.00000:volume=0.0");
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(make_login)
    BOOST_AUTO_TEST_CASE(login_01) {
        an::Message* o1 = an::Message::makeOrder("type=LOGIN:origin=Client1:destination=ME");
        BOOST_CHECK_EQUAL(o1->to_string(),"type=LOGIN:origin=Client1:destination=ME");
        o1->reverse_direction();
        BOOST_CHECK_EQUAL(o1->to_string(),"type=LOGIN:origin=ME:destination=Client1");
    }
BOOST_AUTO_TEST_SUITE_END()

struct CheckMessage {
    explicit CheckMessage(std::string const& str) : str_(str) { }
    bool operator()(an::OrderError const& ex) const {
        bool ok = ex.what() == str_; 
        if (!ok) {
            BOOST_TEST_MESSAGE("Got [" << ex.what() << "] != Expected [" << str_ <<"]");
        }
        return ok;
    }
    std::string str_;
};

bool test(std::exception const& ex) {
    return true;
}

BOOST_AUTO_TEST_SUITE(make_orders_bad)
    //BOOST_CHECK_EXCEPTION( { an::Order 0 = an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50") }, an::OrderError, CheckMessage("Invalid order type []") );
    int x = 0;
    BOOST_AUTO_TEST_CASE(missing_type_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50"),  // type=LIMIT missing
        an::OrderError, CheckMessage("Invalid order type []") );
    }
    BOOST_AUTO_TEST_CASE(missing_type_02) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=XXX:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50"),  // type=XXX unknown
        an::OrderError, CheckMessage("Invalid order type [XXX]") );
    }
    BOOST_AUTO_TEST_CASE(missing_type_03) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50"),  // type=XXX unknown
        an::OrderError, CheckMessage("Invalid order type []") );
    }
    BOOST_AUTO_TEST_CASE(market_invalid_type_04) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type"),  // Nothing after equals
        an::OrderError, CheckMessage("Bad token missing seperator (=) [type]") );
    }
    BOOST_AUTO_TEST_CASE(market_invalid_token_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("xxx=LIMIT"),  // Unknown token
        an::OrderError, CheckMessage("Unused token [xxx,LIMIT]") );
    }
    BOOST_AUTO_TEST_CASE(missing_id_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 111111010 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_origin_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 111110110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_destination_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:symbol=MSFT:direction=BUY:price=92.0:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 111101110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_symbol_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:direction=BUY:price=92.0:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 111011110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_direction_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:price=92.0:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 110111110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_shares_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0"),
        an::OrderError, CheckMessage("Invalid flags got 101111110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(missing_price_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 011111110 expected 111111110") );
    }
    BOOST_AUTO_TEST_CASE(market_extra_price_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92.0"),  // price not needed
        an::OrderError, CheckMessage("Invalid flags got 111111110 expected 011111110") );
    }
    BOOST_AUTO_TEST_CASE(amend_missing_id_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:origin=Client1:destination=ME:symbol=APPL:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 000111010 expected 000111110") );
    }
    BOOST_AUTO_TEST_CASE(amend_extra_direction_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:direction=BUY"), // Can't amend direction
        an::OrderError, CheckMessage("Invalid flags got 001111110 expected 000111110") );
    }
    BOOST_AUTO_TEST_CASE(amend_option_not_given_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL"), 
        an::OrderError, CheckMessage("Invalid amend (none given)") );
    }
    BOOST_AUTO_TEST_CASE(amend_shares_invalid_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:shares=0"), 
        an::OrderError, CheckMessage("Invalid amend number of shares too small") );
    }
    BOOST_AUTO_TEST_CASE(amend_shares_invalid_02) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:symbol=APPL:shares=-10"), 
        an::OrderError, CheckMessage("Invalid shares - out of range") );
    }
    BOOST_AUTO_TEST_CASE(cancel_missing_id_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:origin=Client1:destination=ME:symbol=APPL"), // missing id
        an::OrderError, CheckMessage("Invalid flags got 000111010 expected 000111110") );
    }
    BOOST_AUTO_TEST_CASE(cancel_missing_symbol_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME"), // missing symbol
        an::OrderError, CheckMessage("Invalid flags got 000011110 expected 000111110") );
    }
    BOOST_AUTO_TEST_CASE(cancel_id_zero_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:id=0:origin=Client1:destination=ME:symbol=APPL"), // id is zero
        an::OrderError, CheckMessage("Invalid id set to 0") );
    }
    BOOST_AUTO_TEST_CASE(cancel_extra_price_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:symbol=APPL:price=99.0"),
        an::OrderError, CheckMessage("Invalid flags got 100111110 expected 000111110") );
    }
    BOOST_AUTO_TEST_CASE(market_invalid_direction_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=XXX:shares=50:price=92.0"),  // Invalid direction
        an::OrderError, CheckMessage("Invalid direction [XXX]") );
    }
    BOOST_AUTO_TEST_CASE(response_bad_01) {
       an::Message* o1 = an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
       BOOST_CHECK_EQUAL(o1->to_string(),"type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
       BOOST_CHECK_EXCEPTION(an::Response res1(o1,an::ACK,"Error:message"), an::OrderError, CheckMessage("Cannot have [:|=|\\n] in Response text") );
       BOOST_CHECK_EXCEPTION(an::Response res2(o1,an::ACK,"Error=message"), an::OrderError, CheckMessage("Cannot have [:|=|\\n] in Response text") );
       BOOST_CHECK_EXCEPTION(an::Response res3(o1,an::ACK,"Error\nmessage"), an::OrderError, CheckMessage("Cannot have [:|=|\\n] in Response text") );
       BOOST_CHECK_EXCEPTION(an::Response res4(nullptr,an::ACK,"OK"), an::OrderError, CheckMessage("nullptr in Response Message") );
    }  

BOOST_AUTO_TEST_SUITE_END()
