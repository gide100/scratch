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
    an::CancelOrder lo1(11,"Client2",an::ME);
    BOOST_CHECK_EQUAL(lo1.to_string(),"type=CANCEL:id=11:origin=Client2:destination=ME");
    an::CancelOrder lo2(1234,"ClientA","MatchingEngine2");
    BOOST_CHECK_EQUAL(lo2.to_string(),"type=CANCEL:id=1234:origin=ClientA:destination=MatchingEngine2");
}
BOOST_AUTO_TEST_CASE(amend_order01) {
    an::AmendOrder lo1(11,"Client2",an::ME,99.99);
    BOOST_CHECK_EQUAL(lo1.to_string(),"type=AMEND:id=11:origin=Client2:destination=ME:price=99.99");
    an::AmendOrder lo2(1234,"ClientA",an::ME,an::number_shares_t(999));
    BOOST_CHECK_EQUAL(lo2.to_string(),"type=AMEND:id=1234:origin=ClientA:destination=ME:shares=999");
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(make_orders)
BOOST_AUTO_TEST_CASE(limit_order01) {
    an::Order* o1 = an::Order::makeOrder("origin=Client1:destination=ME:symbol=MSFT:direction=BUY:price=92.0:shares=50:type=LIMIT:id=123");
    BOOST_CHECK_EQUAL(o1->to_string(),"type=LIMIT:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50:price=92");
}
BOOST_AUTO_TEST_CASE(market_order01) {
    an::Order* o1 = an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
    BOOST_CHECK_EQUAL(o1->to_string(),"type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=BUY:shares=50");
}
BOOST_AUTO_TEST_CASE(cancel_order01) {
    an::Order* o1 = an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME");
    BOOST_CHECK_EQUAL(o1->to_string(),"type=CANCEL:id=123:origin=Client1:destination=ME");
}
BOOST_AUTO_TEST_CASE(amend_order01) {
    an::Order* o1 = an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:price=99.99");
    BOOST_CHECK_EQUAL(o1->to_string(),"type=AMEND:id=123:origin=Client1:destination=ME:price=99.99");
    an::Order* o2 = an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:shares=99");
    BOOST_CHECK_EQUAL(o2->to_string(),"type=AMEND:id=123:origin=Client1:destination=ME:shares=99");
}
BOOST_AUTO_TEST_SUITE_END()

struct CheckMessage {
    explicit CheckMessage(std::string const& str) : str_(str) { }
    bool operator()(an::OrderError const& ex) const {
        bool ok = ex.what() == str_; 
        if (!ok) {
            BOOST_TEST_MESSAGE("Got [" << ex.what() << "] != Expected {" << str_ <<"]");
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
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:origin=Client1:destination=ME:shares=50"),
        an::OrderError, CheckMessage("Invalid flags got 000011010 expected 000011110") );
    }
    BOOST_AUTO_TEST_CASE(amend_extra_direction_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME:direction=BUY"), // Can't amend direction
        an::OrderError, CheckMessage("Invalid flags got 001011110 expected 000011110") );
    }
    BOOST_AUTO_TEST_CASE(amend_option_not_given_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=AMEND:id=123:origin=Client1:destination=ME"), // Can't amend direction
        an::OrderError, CheckMessage("Invalid amend (none given)") );
    }
    BOOST_AUTO_TEST_CASE(cancel_missing_id_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:origin=Client1:destination=ME"),
        an::OrderError, CheckMessage("Invalid flags got 000011010 expected 000011110") );
    }
    BOOST_AUTO_TEST_CASE(cancel_extra_price_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=CANCEL:id=123:origin=Client1:destination=ME:price=99.0"),
        an::OrderError, CheckMessage("Invalid flags got 100011110 expected 000011110") );
    }
    BOOST_AUTO_TEST_CASE(market_invalid_direction_01) {
        BOOST_CHECK_EXCEPTION( (void)an::Order::makeOrder("type=MARKET:id=123:origin=Client1:destination=ME:symbol=MSFT:direction=XXX:shares=50:price=92.0"),  // Invalid direction
        an::OrderError, CheckMessage("Invalid direction [XXX]") );
    }

BOOST_AUTO_TEST_SUITE_END()
