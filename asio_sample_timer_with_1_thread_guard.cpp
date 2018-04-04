#include "date.h"
#include <iostream>
#include <chrono>
#include <boost/asio.hpp>
//#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
//#include <boost/system/error_code.hpp>
#include <system_error>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <thread> 

int main() {
    using namespace date;


    // Slushie shack
    boost::asio::io_context context;

    // If the next line is commented out the program exits before the timer completes
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(context);
 
    std::cout << std::chrono::system_clock::now() << " UTC\n";

    // Thread that services the completion handler. Butler to slushy shack, blocks wait for all services to complete
    std::thread butler( [&]() { context.run(); } );

    std::this_thread::sleep_for(std::chrono::seconds(2));

    boost::asio::steady_timer timer(context, std::chrono::seconds(3));
    // Non blocking call, Jonny. When timeout finishes calls completion handler to print ' ...timer expired'
    // Timer begins, waits 5 second, places handler on completion queue.
    timer.async_wait(
        [&](auto ... vn) {
            std::cout << std::chrono::system_clock::now() << " : timer expired.\n";
            work.reset(); // Make context.run() complete
        }
    );

    butler.join();

    std::cout << std::chrono::system_clock::now() << " : done\n";
}
