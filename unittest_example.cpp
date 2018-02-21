// #define BOOST_TEST_DYN_LINK
// #define BOOST_TEST_MODULE Test Order

#include <boost/test/included/unit_test.hpp>
#include <cmath>

// Uncommenting below cause a seg fault
//int subtract(int i, int j) {
//   return i - j;
//}

//BOOST_AUTO_TEST_SUITE(Maths)
//BOOST_AUTO_TEST_CASE(subtractTest) {
//    BOOST_CHECK(subtract(3, 3) == 0);
//}
//BOOST_AUTO_TEST_SUITE_END()
//
//BOOST_AUTO_TEST_SUITE(Physics)
//BOOST_AUTO_TEST_CASE(forceTest) {
//   int F = 250;
//   int m = 25;
//   int a = 10;
//   BOOST_CHECK(F == m*a);
//}
//BOOST_AUTO_TEST_CASE(forceTest1) {
//   int F = 250;
//   int m = 25;
//   int a = 10;
//   BOOST_CHECK(F == m*a);
//}
//BOOST_AUTO_TEST_SUITE_END()
//
//
struct MyClass {
   MyClass() { i = new int; *i =0; }
   ~MyClass() {delete i; }
   int* i;
};

BOOST_AUTO_TEST_SUITE(AClass)
BOOST_FIXTURE_TEST_CASE(test_case1, MyClass) {
    MyClass c;
    BOOST_CHECK(*c.i == 60);
    BOOST_CHECK(*i == 0);
}
BOOST_AUTO_TEST_CASE(test_case2) {
    MyClass c;
    BOOST_CHECK(*c.i == 0);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(BClass, MyClass)
BOOST_FIXTURE_TEST_CASE(test_case3, MyClass) {
    BOOST_CHECK(*i == 0);
}
BOOST_AUTO_TEST_CASE(test_case4) {
    BOOST_CHECK(*i == 0);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_CASE(Test1) {
    // 1
    BOOST_REQUIRE_GE ( std::pow(2,4), 16);
    // 2
    double cos0 = std::cos(0.0);
    BOOST_WARN_MESSAGE( (cos0 < 1.0 || cos0 > 1.0), "cos0 = " << cos0 << " incorrect value" );
    // 3
    int i = 2;
    int j = 1;
    BOOST_REQUIRE_EQUAL(i, j); // fail
}
using namespace boost::unit_test;

void test_case1() { /* ! */ } 
void test_case2() { /* ! */ } 
void test_case3() { /* ! */ } 
void test_case4() { /* ! */ } 

test_suite* init_unit_test_suite(int argc, char* argv[]) {
    test_suite* ts1 = BOOST_TEST_SUITE("test_suite1");
    ts1->add( BOOST_TEST_CASE(&test_case1) );
    ts1->add( BOOST_TEST_CASE(&test_case2) );

    test_suite* ts2 = BOOST_TEST_SUITE("test_suite2");
    ts2->add( BOOST_TEST_CASE(&test_case3) );
    ts2->add( BOOST_TEST_CASE(&test_case4) );

   framework::master_test_suite().add(ts1);
   framework::master_test_suite().add(ts2);

   return 0;
}
