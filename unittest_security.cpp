#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Securities

#include <boost/test/unit_test.hpp>
#include "security_master.hpp"


BOOST_AUTO_TEST_SUITE(tick_table_round_ok)
    BOOST_AUTO_TEST_CASE(tick_table_roundTo_01) {
        //using roundTo = an::TickTable::roundTo;
        BOOST_CHECK(an::TickTable::roundTo(1.0,0)   == 1.0);
        BOOST_CHECK(an::TickTable::roundTo(1.1,0)   == 1.0);
        BOOST_CHECK(an::TickTable::roundTo(1.4,0)   == 1.0);
        BOOST_CHECK(an::TickTable::roundTo(1.5,0)   == 2.0);
        BOOST_CHECK(an::TickTable::roundTo(1.9,0)   == 2.0);

        BOOST_CHECK(an::TickTable::roundTo(1.0,1)   == 1.0);
        BOOST_CHECK(an::TickTable::roundTo(1.1,1)   == 1.1);
        BOOST_CHECK(an::TickTable::roundTo(1.11,1)  == 1.1);
        BOOST_CHECK(an::TickTable::roundTo(1.15,1)  == 1.2);
        BOOST_CHECK(an::TickTable::roundTo(1.19,1)  == 1.2);

        BOOST_CHECK(an::TickTable::roundTo(1.11,2)  == 1.11);
        BOOST_CHECK(an::TickTable::roundTo(1.15,2)  == 1.15);
        BOOST_CHECK(an::TickTable::roundTo(1.55,2)  == 1.55);
        BOOST_CHECK(an::TickTable::roundTo(1.550123,2)  == 1.55);
        BOOST_CHECK(an::TickTable::roundTo(1.551123,2)  == 1.55);
        BOOST_CHECK(an::TickTable::roundTo(1.554999,2)  == 1.55);
        BOOST_CHECK(an::TickTable::roundTo(1.555001,2)  == 1.56);

        BOOST_CHECK(an::TickTable::roundTo(-1.0,0)  == -1.0);
        BOOST_CHECK(an::TickTable::roundTo(-1.1,0)  == -1.0);
        BOOST_CHECK(an::TickTable::roundTo(-1.4,0)  == -1.0);
        BOOST_CHECK(an::TickTable::roundTo(-1.5,0)  == -2.0);

        BOOST_CHECK(an::TickTable::roundTo(1.5,-1)  ==  0.0);
        BOOST_CHECK(an::TickTable::roundTo(5.0,-1)  == 10.0);

        BOOST_CHECK(an::TickTable::roundTo(-1.5,-1)  ==  -0.0);
        BOOST_CHECK(an::TickTable::roundTo(-5.0,-1)  == -10.0);

        BOOST_CHECK(an::TickTable::roundTo(8.999,1)    ==  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.999,2)    ==  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.999,3)    !=  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.999,3)    ==  8.999);
        BOOST_CHECK(an::TickTable::roundTo(8.999,3)    ==  8.999);
        BOOST_CHECK(an::TickTable::roundTo(8.9999,3)   ==  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.9990,3)   ==  8.999);

        BOOST_CHECK(an::TickTable::roundTo(8.999999999999990,1)   ==  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.999999999999990,13)  ==  9.0);
        //BOOST_CHECK(an::TickTable::roundTo(8.999'999'999'999'990,12)  ==  9.0);
        BOOST_CHECK(an::TickTable::roundTo(8.999999999999990,14)  !=  9.0);
    }
    BOOST_AUTO_TEST_CASE(tick_table_roundToAny) {
        BOOST_CHECK(an::TickTable::roundToAny(1.0,1.0)     == 1.0);
        BOOST_CHECK(an::TickTable::roundToAny(1.1,1.0)     == 1.0);
        BOOST_CHECK(an::TickTable::roundToAny(1.3,1.0)     == 1.0);
        BOOST_CHECK(an::TickTable::roundToAny(1.499,1.0)   == 1.0);
        BOOST_CHECK(an::TickTable::roundToAny(1.5,1.0)     == 2.0);

        BOOST_CHECK(an::TickTable::roundToAny(1.2,0.5)     == 1.0);
        BOOST_CHECK(an::TickTable::roundToAny(1.25,0.5)    == 1.5);
        BOOST_CHECK(an::TickTable::roundToAny(1.3,0.5)     == 1.5);
        BOOST_CHECK(an::TickTable::roundToAny(1.5,0.5)     == 1.5);
        BOOST_CHECK(an::TickTable::roundToAny(1.7,0.5)     == 1.5);
        BOOST_CHECK(an::TickTable::roundToAny(1.75,0.5)    == 2.0);

        BOOST_CHECK(an::TickTable::roundToAny(1.2,5.0)     == 0.0);
        BOOST_CHECK(an::TickTable::roundToAny(2.5,5.0)     == 5.0);
        BOOST_CHECK(an::TickTable::roundToAny(5.5,5.0)     == 5.0);
        BOOST_CHECK(an::TickTable::roundToAny(7.5,5.0)     == 10.0);
        
        BOOST_CHECK(an::TickTable::roundToAny(1.2,1.25)     == 1.25);
        BOOST_CHECK(an::TickTable::roundToAny(0.0,1.25)     == 0.0 );
        BOOST_CHECK(an::TickTable::roundToAny(1.8,1.25)     == 1.25 );
        BOOST_CHECK(an::TickTable::roundToAny(1.9,1.25)     == 2.50 );

        
        BOOST_CHECK(an::TickTable::roundToAny(-1.0, 1.0)    == -1.0);
        BOOST_CHECK(an::TickTable::roundToAny(-1.1, 1.0)    == -1.0);
        BOOST_CHECK(an::TickTable::roundToAny(-1.499,1.0)   == -1.0);
        BOOST_CHECK(an::TickTable::roundToAny(-1.5, 1.0)    == -2.0);
        BOOST_CHECK(an::TickTable::roundToAny(-1.9, 1.0)    == -2.0);

        BOOST_CHECK(an::TickTable::roundToAny(-1.2,5.0)     == -0.0);
        BOOST_CHECK(an::TickTable::roundToAny(-2.5,5.0)     == -5.0);
        BOOST_CHECK(an::TickTable::roundToAny(-5.5,5.0)     == -5.0);
        BOOST_CHECK(an::TickTable::roundToAny(-7.5,5.0)     == -10.0);
    }
BOOST_AUTO_TEST_SUITE_END()

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
        BOOST_CHECK( tt.validatePrice(9.0)); 
        BOOST_CHECK( tt.validatePrice(1.00000000000001)); // OK treated as 1.0
        BOOST_CHECK(!tt.validatePrice(1.0000000000001));  // Fails (as expected) not 1.0
        BOOST_CHECK(!tt.validatePrice(1.000000001)); 
        BOOST_CHECK(!tt.validatePrice(9.999999)); 
        BOOST_CHECK(!tt.validatePrice(9.9999999999999999999)); // Treated as 10.0 
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

        p =              1.0000000000001;
        BOOST_CHECK(p == 1.0000000000001); // Not 1.0
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK(!tt.validatePriceAndRound(p));
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK(p == 1.0000000000001);

        p =              1.00000000000001;
        BOOST_CHECK(p == 1.00000000000001);
        BOOST_CHECK(p != 1.0);
        BOOST_CHECK( tt.validatePriceAndRound(p)); // Treated as 1.0 rounded
        BOOST_CHECK(p == 1.0);
        BOOST_CHECK(p != 1.00000000000001);

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
BOOST_AUTO_TEST_SUITE_END()
