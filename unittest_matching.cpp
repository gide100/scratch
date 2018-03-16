#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Matching Engine

#include <boost/test/unit_test.hpp>
#include "types.hpp"
#include "order.hpp"
#include "security_master.hpp"
#include "matching_engine.hpp"


BOOST_AUTO_TEST_SUITE(bookkeeping)
    BOOST_AUTO_TEST_CASE(bookkeeping_01) {
        auto epoch = an::epoch_t{ std::chrono::steady_clock::now(), std::chrono::system_clock::now() };

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
        BOOST_CHECK(bk.stats().volume == 100*5.25+10*5.15+5*5.35);
        BOOST_CHECK(bk.stats().daily_high == 5.35);
        BOOST_CHECK(bk.stats().daily_low == 5.15);
        BOOST_CHECK(bk.stats().open_price == 5.25);
        BOOST_CHECK(bk.stats().last_trade_price == 5.35);
        BOOST_CHECK(bk.stats().last_trade_time == t);

        bk.close();
        BOOST_CHECK(bk.stats().close_price == 5.35);
        BOOST_CHECK(bk.stats().avg_share_price ==(100*5.25+10*5.15+5*5.35)/(100+10+5));
    }
    BOOST_AUTO_TEST_CASE(trades_01) {
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(book)
    BOOST_AUTO_TEST_CASE(side_buy_01) {
    }
    BOOST_AUTO_TEST_CASE(side_sell_01) {
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

