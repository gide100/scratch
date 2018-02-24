#include "date.h"
#include "types.hpp"
#include <ctime>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/operators.hpp>

template <typename T, typename Meaning>
struct Explicit : boost::totally_ordered< Explicit<T,Meaning> >
{
  //! Default constructor does not initialize the value.
  Explicit() { }

  //! Construction from a fundamental value.
  explicit Explicit(T value) : value(value) { }

  //! Implicit conversion back to the fundamental data type.
  explicit inline operator T () const { return value; }

  bool operator==(Explicit<T,Meaning> const& other) const{return other.value == value;}
  bool operator<(Explicit<T,Meaning> const& other) const{return other.value < value;}

  //! The actual fundamental value.
  T value;
};

typedef Explicit<int, struct Age_in_years> MyAge;
typedef Explicit<int, struct Height_in_cm> MyHeight;


// See
// https://stackoverflow.com/questions/48827289/c-convert-a-string-timestamp-to-stdchronosystem-clocktime-point
std::chrono::system_clock::time_point parse_my_timestamp(std::string const& timestamp) {
    auto error = [&timestamp]() {
                    throw std::invalid_argument("Invalid timestamp: "  + timestamp);
                  };

    std::tm tm;
    auto fraction = ::strptime(timestamp.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    if(fraction==nullptr)
        error();
    std::chrono::nanoseconds ns(0);
    if('.' == *fraction) {
        ++fraction;
        char* fraction_end = 0;
        std::chrono::nanoseconds fraction_value(std::strtoul(fraction, &fraction_end, 10));
        if(fraction_end != timestamp.data() + timestamp.size())
            error();
        auto fraction_len = fraction_end - fraction;
        if(fraction_len > 9)
            error();
        ns = fraction_value * static_cast<std::int32_t>(std::pow(10, 9 - fraction_len));
    }
    else if(fraction != timestamp.data() + timestamp.size())
        error();
    auto seconds_since_epoch = std::mktime(&tm); // Assumes timestamp is in localtime. For UTC use timegm.
    auto timepoint_ns = std::chrono::system_clock::from_time_t(seconds_since_epoch) + ns;
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(timepoint_ns);
}


int main()
{
    //std::chrono::time_point<std::chrono::steady_clock> tp = std::chrono::steady_clock::now();
   // std::cout << tp << '\n';

    //std::cout << time_fmt(std::chrono::timezone::local, "%A %B %e, %Y %r");
    //std::cout << std::chrono::system_clock::now() << '\n';  // Sunday April 24, 2011 02:36:59 PM

    std::tm t = {};
    std::istringstream ss("2011-Februar-18 23:12:34");
    ss.imbue(std::locale("de_DE.utf-8"));
    ss >> std::get_time(&t, "%Y-%b-%d %H:%M:%S");
    if (ss.fail()) {
        std::cout << "Parse failed\n";
    } else {
        std::cout << std::put_time(&t, "%c") << '\n';
    }

    std::chrono::system_clock::time_point myTime = parse_my_timestamp("2017-08-28 03:59:55.0007");
    std::cout << myTime.time_since_epoch().count() << std::endl;


    MyAge age(10);
    MyHeight height(10);
    MyHeight height1(10);
    // ERROR not allowed
    // age = height;

    //(void) (age == height) ;

    if (age.value == height.value) {
         std::cout << "All good" << std::endl;
    }
    if (height == height1) {
         std::cout << "All good" << std::endl;
    }

    std::stringstream str( "2017-08-28 03:59:55.0007" );
    str.imbue( std::locale("en_GB.UTF-8") );
    std::chrono::time_point< std::chrono::system_clock, std::chrono::microseconds > result;
    date::from_stream( str, "%Y-%m-%d %H:%M:%S", result );
    std::cout << result.time_since_epoch().count() << std::endl;

    std::cout << "stringToTimeT " << an::stringToTimeT("2017-08-28 03:59:55.0007", "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "Max time [2038-01-19 03:14:07]" << an::timeTtoString(an::MY_MAX_DATE, "%Y-%b-%d %H:%M:%S") << std::endl;
    std::cout << "Max time [1970-01-01]" << an::dateToString(0) << std::endl;
    std::cout << "Max time [2018-02-23 22:53:09]" << an::dateToString(1519426389) << std::endl;

    return 0;
}
