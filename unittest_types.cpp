#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test Types

#include <boost/test/unit_test.hpp>
#include "types.hpp"
#include <iostream>

BOOST_AUTO_TEST_SUITE(round_ok)
    BOOST_AUTO_TEST_CASE(floatDecimalPlaces_01) {
        BOOST_CHECK(an::floatDecimalPlaces( 0.0,1)   == "0.0");
        BOOST_CHECK(an::floatDecimalPlaces( 0.0,2)   == "0.0");
        BOOST_CHECK(an::floatDecimalPlaces( 0.0,0)   == "0");

        BOOST_CHECK(an::floatDecimalPlaces( 1.0,1)   == "1.0");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0,2)   == "1.0");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0,0)   == "1");

        BOOST_CHECK(an::floatDecimalPlaces(-1.0,1)   == "-1.0");
        BOOST_CHECK(an::floatDecimalPlaces(-1.0,2)   == "-1.0");
        BOOST_CHECK(an::floatDecimalPlaces(-1.0,0)   == "-1");


        BOOST_CHECK(an::floatDecimalPlaces( 1.12,1)   == "1.1");
        BOOST_CHECK(an::floatDecimalPlaces( 1.12,2)   == "1.12");
        BOOST_CHECK(an::floatDecimalPlaces( 1.12,0)   == "1");

        BOOST_CHECK(an::floatDecimalPlaces( 100000.0,1)   == "100000.0");
        BOOST_CHECK(an::floatDecimalPlaces( 100000.0,2)   == "100000.0");
        BOOST_CHECK(an::floatDecimalPlaces( 100000.0,0)   == "100000");

        BOOST_CHECK(an::floatDecimalPlaces( 100000.12,1)   == "100000.1");
        BOOST_CHECK(an::floatDecimalPlaces( 100000.12,2)   == "100000.12");
        BOOST_CHECK(an::floatDecimalPlaces( 100000.12,0)   == "100000");

        BOOST_CHECK(an::floatDecimalPlaces( 1.12,3)   == "1.12");
        BOOST_CHECK(an::floatDecimalPlaces( 1.12,6)   == "1.12");

        BOOST_CHECK(an::floatDecimalPlaces( 1.1205,3)   == "1.121");
        BOOST_CHECK(an::floatDecimalPlaces( 1.1205,2)   == "1.12");
        BOOST_CHECK(an::floatDecimalPlaces( 1.1205,4)   == "1.1205");
        BOOST_CHECK(an::floatDecimalPlaces( 1.1205,5)   == "1.1205");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0/(1000*1000*1000),10)   == "0.000000001");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0/(1000*1000*1000),9)    == "0.000000001");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0/(1000*1000*1000),8)    == "0.0");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0/(1000*1000*1000),1)    == "0.0");
        BOOST_CHECK(an::floatDecimalPlaces( 1.0/(1000*1000*1000),0)    == "0");
    }

    BOOST_AUTO_TEST_CASE(roundTo_01) {
        BOOST_CHECK(an::roundTo(1.0,0)   == 1.0);
        BOOST_CHECK(an::roundTo(1.1,0)   == 1.0);
        BOOST_CHECK(an::roundTo(1.4,0)   == 1.0);
        BOOST_CHECK(an::roundTo(1.5,0)   == 2.0);
        BOOST_CHECK(an::roundTo(1.9,0)   == 2.0);

        BOOST_CHECK(an::roundTo(1.0,1)   == 1.0);
        BOOST_CHECK(an::roundTo(1.1,1)   == 1.1);
        BOOST_CHECK(an::roundTo(1.11,1)  == 1.1);
        BOOST_CHECK(an::roundTo(1.15,1)  == 1.2);
        BOOST_CHECK(an::roundTo(1.19,1)  == 1.2);

        BOOST_CHECK(an::roundTo(1.11,2)  == 1.11);
        BOOST_CHECK(an::roundTo(1.15,2)  == 1.15);
        BOOST_CHECK(an::roundTo(1.55,2)  == 1.55);
        BOOST_CHECK(an::roundTo(1.550123,2)  == 1.55);
        BOOST_CHECK(an::roundTo(1.551123,2)  == 1.55);
        BOOST_CHECK(an::roundTo(1.554999,2)  == 1.55);
        BOOST_CHECK(an::roundTo(1.555001,2)  == 1.56);

        BOOST_CHECK(an::roundTo(-1.0,0)  == -1.0);
        BOOST_CHECK(an::roundTo(-1.1,0)  == -1.0);
        BOOST_CHECK(an::roundTo(-1.4,0)  == -1.0);
        BOOST_CHECK(an::roundTo(-1.5,0)  == -2.0);

        BOOST_CHECK(an::roundTo(1.5,-1)  ==  0.0);
        BOOST_CHECK(an::roundTo(5.0,-1)  == 10.0);

        BOOST_CHECK(an::roundTo(-1.5,-1)  ==  -0.0);
        BOOST_CHECK(an::roundTo(-5.0,-1)  == -10.0);

        BOOST_CHECK(an::roundTo(8.999,1)    ==  9.0);
        BOOST_CHECK(an::roundTo(8.999,2)    ==  9.0);
        BOOST_CHECK(an::roundTo(8.999,3)    !=  9.0);
        BOOST_CHECK(an::roundTo(8.999,3)    ==  8.999);
        BOOST_CHECK(an::roundTo(8.999,3)    ==  8.999);
        BOOST_CHECK(an::roundTo(8.9999,3)   ==  9.0);
        BOOST_CHECK(an::roundTo(8.9990,3)   ==  8.999);

        BOOST_CHECK(an::roundTo(8.999999999999990,1)   ==  9.0);
        BOOST_CHECK(an::roundTo(8.999999999999990,13)  ==  9.0);
        //BOOST_CHECK(an::roundTo(8.999'999'999'999'990,12)  ==  9.0);
        BOOST_CHECK(an::roundTo(8.999999999999990,14)  !=  9.0);
    }
    BOOST_AUTO_TEST_CASE(roundToAny) {
        BOOST_CHECK(an::roundToAny(1.0,1.0)     == 1.0);
        BOOST_CHECK(an::roundToAny(1.1,1.0)     == 1.0);
        BOOST_CHECK(an::roundToAny(1.3,1.0)     == 1.0);
        BOOST_CHECK(an::roundToAny(1.499,1.0)   == 1.0);
        BOOST_CHECK(an::roundToAny(1.5,1.0)     == 2.0);

        BOOST_CHECK(an::roundToAny(1.2,0.5)     == 1.0);
        BOOST_CHECK(an::roundToAny(1.25,0.5)    == 1.5);
        BOOST_CHECK(an::roundToAny(1.3,0.5)     == 1.5);
        BOOST_CHECK(an::roundToAny(1.5,0.5)     == 1.5);
        BOOST_CHECK(an::roundToAny(1.7,0.5)     == 1.5);
        BOOST_CHECK(an::roundToAny(1.75,0.5)    == 2.0);

        BOOST_CHECK(an::roundToAny(1.2,5.0)     == 0.0);
        BOOST_CHECK(an::roundToAny(2.5,5.0)     == 5.0);
        BOOST_CHECK(an::roundToAny(5.5,5.0)     == 5.0);
        BOOST_CHECK(an::roundToAny(7.5,5.0)     == 10.0);

        BOOST_CHECK(an::roundToAny(1.2,1.25)     == 1.25);
        BOOST_CHECK(an::roundToAny(0.0,1.25)     == 0.0 );
        BOOST_CHECK(an::roundToAny(1.8,1.25)     == 1.25 );
        BOOST_CHECK(an::roundToAny(1.9,1.25)     == 2.50 );


        BOOST_CHECK(an::roundToAny(-1.0, 1.0)    == -1.0);
        BOOST_CHECK(an::roundToAny(-1.1, 1.0)    == -1.0);
        BOOST_CHECK(an::roundToAny(-1.499,1.0)   == -1.0);
        BOOST_CHECK(an::roundToAny(-1.5, 1.0)    == -2.0);
        BOOST_CHECK(an::roundToAny(-1.9, 1.0)    == -2.0);

        BOOST_CHECK(an::roundToAny(-1.2,5.0)     == -0.0);
        BOOST_CHECK(an::roundToAny(-2.5,5.0)     == -5.0);
        BOOST_CHECK(an::roundToAny(-5.5,5.0)     == -5.0);
        BOOST_CHECK(an::roundToAny(-7.5,5.0)     == -10.0);
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(pstring)
    BOOST_AUTO_TEST_CASE(pstring_01) {
        BOOST_CHECK_NO_THROW(an::PString p0);
        BOOST_CHECK_NO_THROW(an::PString p1("TEST",4));
        BOOST_CHECK_NO_THROW(an::PString p2("TEST"));
        BOOST_CHECK_NO_THROW(an::PString p3(std::string("TEST")));
    }
    BOOST_AUTO_TEST_CASE(pstring_02) {
        BOOST_CHECK(an::PString("TEST",4)       == an::PString("TEST"));
        BOOST_CHECK(an::PString("TEST\0MORE")   == an::PString("TEST"));
        BOOST_CHECK(an::PString("TEST\0MORE")   == an::PString("TEST\0MORE"));
        BOOST_CHECK(an::PString("TEST\0MORE").to_string().size() == 4);
        BOOST_CHECK(an::PString("TEST\0MORE",9) != an::PString("TEST"));
        BOOST_CHECK(an::PString("TEST\0MORE",9).to_string().size() == 9);
    }
    BOOST_AUTO_TEST_CASE(pstring_03) {
        BOOST_CHECK( (an::PString("TEST") == an::PString("TEST")) );
        BOOST_CHECK(!(an::PString("TEST") != an::PString("TEST")) );
        BOOST_CHECK( (an::PString("TEST") != an::PString("ABCD")) );
        BOOST_CHECK(!(an::PString("TEST") == an::PString("ABCD")) );

        BOOST_CHECK( (an::PString("b")    >  an::PString("a")) );
        BOOST_CHECK( (an::PString("B")    >  an::PString("A")) );
        BOOST_CHECK( (an::PString("z")    >  an::PString("y")) );
        BOOST_CHECK( (an::PString("b")    >  an::PString("a")) );
        BOOST_CHECK(!(an::PString("a")    >  an::PString("a")) );
        BOOST_CHECK(!(an::PString("a")    >  an::PString("z")) );

        BOOST_CHECK( (an::PString("TEST") >= an::PString("TEST")) );
        BOOST_CHECK( (an::PString("z")    >= an::PString("z")) );
        BOOST_CHECK( (an::PString("a")    >= an::PString("a")) );
        BOOST_CHECK( (an::PString("z")    >= an::PString("a")) );
        BOOST_CHECK(!(an::PString("a")    >= an::PString("z")) );

        BOOST_CHECK(!(an::PString("b")    <  an::PString("a")) );
        BOOST_CHECK(!(an::PString("B")    <  an::PString("A")) );
        BOOST_CHECK(!(an::PString("z")    <  an::PString("y")) );
        BOOST_CHECK(!(an::PString("b")    <  an::PString("a")) );
        BOOST_CHECK(!(an::PString("a")    <  an::PString("a")) );
        BOOST_CHECK( (an::PString("a")    <  an::PString("z")) );

        BOOST_CHECK( (an::PString("TEST") <= an::PString("TEST")) );
        BOOST_CHECK( (an::PString("z")    <= an::PString("z")) );
        BOOST_CHECK( (an::PString("a")    <= an::PString("a")) );
        BOOST_CHECK(!(an::PString("z")    <= an::PString("a")) );
        BOOST_CHECK( (an::PString("a")    <= an::PString("z")) );
    }
    BOOST_AUTO_TEST_CASE(pstring_hash_01) {
        BOOST_CHECK( (std::hash<an::PString>()(an::PString("TEST")) == std::hash<an::PString>()(an::PString("TEST"))) );
        BOOST_CHECK( (std::hash<an::PString>()(an::PString("ABCD")) != std::hash<an::PString>()(an::PString("TEST"))) );
    }
BOOST_AUTO_TEST_SUITE_END()

struct CheckMessage {
    explicit CheckMessage(std::string const& str) : str_(str) { }
    bool operator()(const std::exception& ex) const {
        bool ok = ex.what() == str_;
        if (!ok) {
            BOOST_TEST_MESSAGE("Got [" << ex.what() << "] != Expected [" << str_ <<"]");
        }
        return ok;
    }
    std::string str_;
};

BOOST_AUTO_TEST_SUITE(pstring_bad_01)
    BOOST_AUTO_TEST_CASE(long_string_01) {
        BOOST_CHECK_NO_THROW (an::PString p("",1));
        BOOST_CHECK_NO_THROW (an::PString p("",(1 << 15)-1));
        BOOST_CHECK_EXCEPTION(an::PString p("",1 << 15),std::runtime_error,CheckMessage("PString too long"));
        BOOST_CHECK_EXCEPTION(an::PString p(std::string(1 << 16,'a')),std::runtime_error,CheckMessage("PString too long"));
    }
    BOOST_AUTO_TEST_CASE(long_string_02) {
        std::string s(1 << 16, 'a');
        an::PString p;
        BOOST_CHECK_EXCEPTION(p.assign(s.c_str(), s.size()),
                              std::runtime_error, CheckMessage("PString too long") );
    }
    BOOST_AUTO_TEST_CASE(long_string_03) {
        std::string s(1 << 16, 'a');
        BOOST_CHECK_EXCEPTION(an::PString p(s.c_str(), s.size()),
                              std::runtime_error, CheckMessage("PString too long") );
    }
    BOOST_AUTO_TEST_CASE(long_string_04) {
        const std::string myS(1 << 16, 'a');
        BOOST_CHECK_EXCEPTION(an::PString p(myS),
                              std::runtime_error, CheckMessage("PString too long") );
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(conv)
    BOOST_AUTO_TEST_CASE(int_conv_01) {
        std::uint64_t myULong = 0;
        BOOST_CHECK(an::pstring2int<uint64_t>(&myULong,an::PString("10")));
        BOOST_CHECK(myULong == 10);
        BOOST_CHECK(an::pstring2int<uint64_t>(&myULong,an::PString("0")));
        BOOST_CHECK(myULong == 0);
        BOOST_CHECK(an::pstring2int<uint64_t>(&myULong,an::PString("1234567890")));
        BOOST_CHECK(myULong == 1234567890);
        BOOST_CHECK(an::pstring2int<uint64_t>(&myULong,an::PString("+1")));
        BOOST_CHECK(myULong == 1);
        BOOST_CHECK(an::pstring2int<uint64_t>(&myULong,an::PString("-1")));
        BOOST_CHECK(myULong == -1UL);
        BOOST_CHECK(myULong == 18'446'744'073'709'551'615UL);
        BOOST_CHECK(myULong == 0xFFFFFFFFFFFFFFFFUL);

        std::int64_t myLong = 0;
        BOOST_CHECK(an::pstring2int<int64_t>(&myLong,an::PString("10")));
        BOOST_CHECK(myLong == 10);
        BOOST_CHECK(an::pstring2int<int64_t>(&myLong,an::PString("0")));
        BOOST_CHECK(myLong == 0);
        BOOST_CHECK(an::pstring2int<int64_t>(&myLong,an::PString("1234567890")));
        BOOST_CHECK(myLong == 1234567890);
        BOOST_CHECK(an::pstring2int<int64_t>(&myLong,an::PString("+1")));
        BOOST_CHECK(myLong == 1);
        BOOST_CHECK(an::pstring2int<int64_t>(&myLong,an::PString("-1")));
        BOOST_CHECK(myLong == -1);
    }
    BOOST_AUTO_TEST_CASE(int_conv_error_01) {
        std::uint64_t myULong = 0;
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("x10")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString(" 10")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("10x")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("++1")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("--1")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("+-1")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("-+1")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("0x")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("0x1")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("F")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString(".")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("1.")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("1e")));
        BOOST_CHECK(myULong == 0);
    }
    BOOST_AUTO_TEST_CASE(int_conv_overflow_01) {
        std::int64_t myLong = 0;
        BOOST_CHECK(!an::pstring2int<int64_t>(&myLong,an::PString("18446744073709551615")));
        BOOST_CHECK(!an::pstring2int<int64_t>(&myLong,an::PString("-18446744073709551615")));
        BOOST_CHECK(myLong == 0);
    }
    BOOST_AUTO_TEST_CASE(int_conv_overflow_02) {
        std::uint64_t myULong = 0;
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("184467440737095516150")));
        BOOST_CHECK(!an::pstring2int<uint64_t>(&myULong,an::PString("-184467440737095516150")));
        BOOST_CHECK(myULong == 0);
    }
BOOST_AUTO_TEST_SUITE_END()

