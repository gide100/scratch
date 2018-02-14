#include <iostream>
#include <boost/asio.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

int main() {
    // Slushie shack
    boost::asio::io_service service;

    boost::asio::deadline_timer timer(service, boost::posix_time::seconds(5));

    // Non blocking call, Jonny
    timer.async_wait([](auto ... vn) {
                           std::cout << boost::chrono::system_clock::now()
                                     << " : timer expired.\n";
                                     }
                    );

    //std::cout << boost::chrono::time_fmt(boost::chrono::timezone::local) ;
    std::cout << boost::chrono::system_clock::now() << " : calling run\n";

    service.run(); // Bulter to slushy shack, blocks wait for all services to complete

    std::cout << boost::chrono::system_clock::now() << " : done\n";
}
