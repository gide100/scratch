#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Securities

#include <boost/test/unit_test.hpp>
#include "security_master.hpp"

BOOST_AUTO_TEST_SUITE(tick_table_row_ok)
    BOOST_AUTO_TEST_CASE(tick_table_row_ctor_01) {
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(0,1,0.1));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(0,10,1));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(1,2,0.1));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(10,11,0.1));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(0,10000,0.1));
    }
    BOOST_AUTO_TEST_CASE(tick_table_row_ctor_02) {
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(0,0.1));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(0,1.0));
        BOOST_CHECK_NO_THROW((void)an::tick_table_row_t(10000,100.0));
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

BOOST_AUTO_TEST_SUITE(tick_table_row_bad)
    BOOST_AUTO_TEST_CASE(lower_gte_upper_01) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(1,0,1),
            an::SecurityError, CheckMessage("Tick lower >= upper") );
    }
    BOOST_AUTO_TEST_CASE(lower_gte_upper_02) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(1,0.1,1),
            an::SecurityError, CheckMessage("Tick lower >= upper") );
    }
    BOOST_AUTO_TEST_CASE(lower_gte_upper_03) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,0,1),
            an::SecurityError, CheckMessage("Tick lower >= upper") );
    }
    BOOST_AUTO_TEST_CASE(lower_gte_upper_04) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,0.0+an::PRICE_EPSILON/1.01,an::PRICE_EPSILON),
            an::SecurityError, CheckMessage("Invalid tick size") );
    }
    BOOST_AUTO_TEST_CASE(negative_01) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(-1.0,1.0,1),
            an::SecurityError, CheckMessage("lower < 0.0") );
    }
    BOOST_AUTO_TEST_CASE(negative_02) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(-1.0,1),
            an::SecurityError, CheckMessage("lower < 0.0") );
    }
    BOOST_AUTO_TEST_CASE(invalid_increment_01) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,1,0),
            an::SecurityError, CheckMessage("Invalid tick size") );
    }
    BOOST_AUTO_TEST_CASE(invalid_increment_02) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,0),
            an::SecurityError, CheckMessage("Invalid tick size") );
    }
    BOOST_AUTO_TEST_CASE(invalid_increment_03) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,1,2),
            an::SecurityError, CheckMessage("Increment greater than lower-upper") );
    }
    BOOST_AUTO_TEST_CASE(invalid_increment_04) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,10,20),
            an::SecurityError, CheckMessage("Increment greater than lower-upper") );
    }
    BOOST_AUTO_TEST_CASE(invalid_increment_05) {
        BOOST_CHECK_EXCEPTION( (void)an::tick_table_row_t(0,1,1.0/(1000.0*1000.0*1000.0)),
            an::SecurityError, CheckMessage("Invalid tick size") );
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(tick_table)
    BOOST_AUTO_TEST_CASE(tick_table_ok_01) {
        an::TickTable tt;
        tt.add(an::tick_table_row_t( 0,  1, 0.1));
        tt.add(an::tick_table_row_t( 1, 10, 1));

        BOOST_CHECK_EQUAL(tt.to_string(),
            "0        >= < 1        0.1\n"
            "1        >= < 10       1\n" );
        BOOST_CHECK( tt.validatePrice(0.0));
        BOOST_CHECK( tt.validatePrice(0.1));
        BOOST_CHECK( tt.validatePrice(0.9));
        BOOST_CHECK( tt.validatePrice(1.0));
        BOOST_CHECK( tt.validatePrice(2.0));
        BOOST_CHECK( tt.validatePrice(8.9999999));  // Ok treated as 9.0
        BOOST_CHECK( tt.validatePrice(9.0));
        BOOST_CHECK( tt.validatePrice(1.00000001)); // OK treated as 1.0
        BOOST_CHECK(!tt.validatePrice(1.0000001));  // Fails (as expected) not 1.0
        BOOST_CHECK(!tt.validatePrice(1.000001)); // Fails (as expected) not 1.0
        BOOST_CHECK(!tt.validatePrice(8.999999));  // Fails, not 9.0
        BOOST_CHECK(!tt.validatePrice(9.9999999999999999)); // Failes, treated as 10.0 by compiler
        BOOST_CHECK(!tt.validatePrice(1.1)); // Fails
        BOOST_CHECK(!tt.validatePrice(10)); // Fails
        BOOST_CHECK(!tt.validatePrice(20)); // Fails
        BOOST_CHECK(!tt.validatePrice(100)); // Fails
        BOOST_CHECK(!tt.validatePrice(100000)); // Fails

        tt.add(an::tick_table_row_t( 10, 20, 2));
        BOOST_CHECK_EQUAL(tt.to_string(),
            "0        >= < 1        0.1\n"
            "1        >= < 10       1\n"
            "10       >= < 20       2\n" );
        BOOST_CHECK(tt.validatePrice(10)); // Now ok
        BOOST_CHECK(tt.validatePrice(9.9999999999999999999)); // Treated as 10.0
        BOOST_CHECK(!tt.validatePrice(20)); // Still fails
    }
    BOOST_AUTO_TEST_CASE(tick_table_ok_02) {
        an::TickTable tt;
        tt.add(an::tick_table_row_t(  0,  0.001));
        tt.add(an::tick_table_row_t( 10,  0.005));
        tt.add(an::tick_table_row_t( 50,  0.01));
        tt.add(an::tick_table_row_t(100,  0.05));

        BOOST_CHECK_EQUAL(tt.to_string(),
            "0        >= < 10       0.001\n"
            "10       >= < 50       0.005\n"
            "50       >= < 100      0.01\n"
            "100      >=            0.05\n" );
        BOOST_CHECK( tt.validatePrice(0.0));
        BOOST_CHECK( tt.validatePrice(0.1));
        BOOST_CHECK( tt.validatePrice(0.9));
        BOOST_CHECK( tt.validatePrice(1.0));
        BOOST_CHECK( tt.validatePrice(2.0));
        BOOST_CHECK( tt.validatePrice(10));
        BOOST_CHECK( tt.validatePrice(20));
        BOOST_CHECK( tt.validatePrice(100));
        BOOST_CHECK( tt.validatePrice(100000));
        BOOST_CHECK( tt.validatePrice(1000000));
        BOOST_CHECK( tt.validatePrice(10000000));
        BOOST_CHECK( tt.validatePrice(1.001));
        BOOST_CHECK(!tt.validatePrice(10.001)); // Fails (as expected), tick 0.005
        BOOST_CHECK( tt.validatePrice(50.0));  // Ok
        BOOST_CHECK(!tt.validatePrice(50.005));  // Fails, not tick 0.01
        BOOST_CHECK( tt.validatePrice(50.01));  // Ok
        BOOST_CHECK(!tt.validatePrice(100000.01)); //Fails,tick 0.05
        BOOST_CHECK( tt.validatePrice(100000.05)); //Ok
    }
    BOOST_AUTO_TEST_CASE(tick_table_rounding_ok_01) {
        an::price_t p = 0.0;
        an::TickTable tt;
        tt.add(an::tick_table_row_t( 0,  1, 0.1));
        tt.add(an::tick_table_row_t( 1, 10, 1));

        BOOST_CHECK_EQUAL(tt.to_string(),
            "0        >= < 1        0.1\n"
            "1        >= < 10       1\n" );

        p =              1.0;
        BOOST_CHECK(p == 1.0);
        BOOST_CHECK( tt.validatePriceAndRound(p));
        BOOST_CHECK(p == 1.0);

        p =              1.1;
        BOOST_CHECK(p == 1.1);
        BOOST_CHECK(!tt.validatePriceAndRound(p));
        BOOST_CHECK(p == 1.1);

        p =              1.13;
        BOOST_CHECK(p == 1.13);
        BOOST_CHECK(!tt.validatePriceAndRound(p));
        BOOST_CHECK(p == 1.13);

        p =              1.0000001;
        BOOST_CHECK(p == 1.0000001); // Not 1.0
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK(!tt.validatePriceAndRound(p));
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK(p == 1.0000001);

        p =              1.00000001;
        BOOST_CHECK(p == 1.00000001);
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK( tt.validatePriceAndRound(p)); // Treated as 1.0 rounded
        BOOST_CHECK(p == 1.0);
        BOOST_CHECK(p != 1.00000001);

        p =              7.999999999999999;
        BOOST_CHECK(p == 7.999999999999999);
        BOOST_CHECK(p != 8.0);
        BOOST_CHECK( tt.validatePriceAndRound(p)); // Treated as 8.0
        BOOST_CHECK(p == 8.0);
        BOOST_CHECK(p != 7.999999999999999);

        p =              7.9999999999999999;
        BOOST_CHECK(p == 7.9999999999999999);
        BOOST_CHECK(p == 8.0);  // Rounded by compiler
        BOOST_CHECK( tt.validatePriceAndRound(p)); // Treated as 8.0
        BOOST_CHECK(p == 8.0);
        BOOST_CHECK(p == 7.9999999999999999); // Is 8.0

    }
    BOOST_AUTO_TEST_CASE(tick_table_bad_01) {
        an::TickTable tt;
        tt.add(an::tick_table_row_t( 0,  1, 0.1));
        BOOST_CHECK_EXCEPTION( tt.add(an::tick_table_row_t( 2, 10, 1)),
            an::SecurityError, CheckMessage("Gap in tick table") );
    }
    BOOST_AUTO_TEST_CASE(tick_table_bad_02) {
        an::TickTable tt;
        tt.add(an::tick_table_row_t( 0,  2, 0.1));
        BOOST_CHECK_EXCEPTION( tt.add(an::tick_table_row_t( 1, 10, 1)),
            an::SecurityError, CheckMessage("Overlapping ranges in tick table") );
    }
    BOOST_AUTO_TEST_CASE(tick_ladder_load_01) {
        an::TickTable* tt = nullptr;
        an::TickLadder tickdb;
        tickdb.loadData("NXT_ticksize.txt");

        tt = tickdb.find(1);
        BOOST_CHECK(tt != nullptr);
        BOOST_CHECK_EQUAL(tt->to_string(),
            "0        >=            0.01\n" );

        tt = tickdb.find(10);
        BOOST_CHECK(tt != nullptr);
        BOOST_CHECK_EQUAL(tt->to_string(),
            "0        >= < 10       0.001\n"
            "10       >= < 50       0.005\n"
            "50       >= < 100      0.01\n"
            "100      >=            0.05\n" );

        tt = tickdb.find(96);
        BOOST_CHECK(tt != nullptr);
        BOOST_CHECK_EQUAL(tt->to_string(),
            "0        >=            1e-06\n" );

        BOOST_CHECK(tickdb.find(73) != nullptr);
        BOOST_CHECK(tickdb.find(89) != nullptr);
        BOOST_CHECK(tickdb.find(96) != nullptr);

        BOOST_CHECK(tickdb.find(97) == nullptr);
        BOOST_CHECK(tickdb.find(20) == nullptr);
    }

    BOOST_AUTO_TEST_CASE(secdb_load_01) {
        an::TickLadder tickdb;
        tickdb.loadData("NXT_ticksize.txt");
        an::SecurityDatabase secdb(an::ME, tickdb);
        secdb.loadData("security_database.csv");
        BOOST_CHECK(secdb.securities().size()            == 6);

        BOOST_CHECK(secdb.securities().front().id        == 1000);
        BOOST_CHECK(secdb.securities().front().symbol    == "APPL");
        BOOST_CHECK(!secdb.securities().front().has_died);
        BOOST_CHECK(secdb.securities().front().tradeable);
        BOOST_CHECK(secdb.find("APPL")                   == 0);
        BOOST_CHECK(secdb.tickTable(secdb.find("APPL"))  != nullptr); // Has TickTable

        BOOST_CHECK(secdb.find("IBM")                    == 2);
        BOOST_CHECK(secdb.securities()[2].id             == 99);
        BOOST_CHECK(secdb.securities()[2].symbol         == "IBM");
        BOOST_CHECK(secdb.tickTable(2)                   != nullptr); // Has TickTable

        BOOST_CHECK(secdb.find("GE")                     == 5);
        BOOST_CHECK(secdb.securities().back().id         == 66);
        BOOST_CHECK(secdb.securities().back().symbol     == "GE");

        BOOST_CHECK(secdb.securities()[1].id             == 666);
        BOOST_CHECK(secdb.securities()[1].symbol         == "ENE");
        BOOST_CHECK(secdb.securities()[1].has_died);
        BOOST_CHECK(!secdb.securities()[1].tradeable);

        BOOST_CHECK(secdb.securities()[4].id             == 100);
        BOOST_CHECK(secdb.securities()[4].symbol         == "VOD.L");
        BOOST_CHECK(!secdb.securities()[4].has_died);
        BOOST_CHECK(secdb.securities()[4].tradeable);

        BOOST_CHECK(secdb.find("MSFT")                   != an::SecurityDatabase::npos);

        // Securities that have died or have a different exchange cannot be looked up
        BOOST_CHECK(secdb.find("ENE")                    == an::SecurityDatabase::npos);
        BOOST_CHECK(secdb.find("VOD.L")                  == an::SecurityDatabase::npos);
        BOOST_CHECK(secdb.find("UNKNOWN")                == an::SecurityDatabase::npos);
    }
    BOOST_AUTO_TEST_CASE(secdb_load_bad_01) {
        an::TickLadder tickdb;
        an::SecurityDatabase secdb(an::ME, tickdb);
        secdb.loadData("security_database.csv");
        BOOST_CHECK(secdb.securities().size()            == 6);
        BOOST_CHECK(secdb.find("APPL")                   == 0);
        BOOST_CHECK(secdb.tickTable(secdb.find("APPL"))  == nullptr); // Didn't load tickdb
        BOOST_CHECK(secdb.tickTable(1)                   == nullptr); // Didn't load tickdb
        BOOST_CHECK(secdb.tickTable(5)                   == nullptr); // Didn't load tickdb
    }
BOOST_AUTO_TEST_SUITE_END()
