#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
#include <iostream>

int main()
{
    using namespace std;
    using namespace boost;

    typedef boost::chrono::time_point<boost::chrono::steady_clock, boost::chrono::duration<double, boost::ratio<3600> > > T;
    T tp = boost::chrono::steady_clock::now();
    std::cout << tp << '\n';

    cout << time_fmt(chrono::timezone::local, "%A %B %e, %Y %r");
    cout << chrono::system_clock::now() << '\n';  // Sunday April 24, 2011 02:36:59 PM
    return 0;
}
