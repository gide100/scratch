#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


void timer_expired(std::string id) {
    std::cout << boost::chrono::system_clock::now() << " " << id << " enter.\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << boost::chrono::system_clock::now() << " " << id << " leave.\n";
}

int main() {
    // Slushie shack
    boost::asio::io_context context;

    boost::asio::deadline_timer timer1(context, boost::posix_time::seconds(5));
    boost::asio::deadline_timer timer2(context, boost::posix_time::seconds(5));

    // Non blocking call, Jonny. When timeout finishes calls completion handler to print ' ...timer expired'
    // Timer begins, waits 5 second, places handler on completion queue.
    timer1.async_wait( [](auto ... vn) { timer_expired("timer1"); } );
    timer2.async_wait( [](auto ... vn) { timer_expired("timer2"); } );

    // Thread that services the completion handler. Butler to slushy shack, blocks wait for all services to complete
    std::thread butler1( [&]() { context.run(); } );
    std::thread butler2( [&]() { context.run(); } );


    butler1.join();
    butler2.join();

    std::cout << boost::chrono::system_clock::now() << " : done\n";
}
