#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>

int main() {
    // Slushie shack
    boost::asio::io_context context;

    context.post( []{ std::cout << "eat\n"; } );
    context.post( []{ std::cout << "drink\n"; } );
    context.post( []{ std::cout << "and be merry!\n"; } );

    // Thread that services the completion handler. Butler to slushy shack, blocks wait for all services to complete
    std::thread butler1( [&]() { context.run(); } );

    butler1.join();

    std::cout << boost::chrono::system_clock::now() << " : done\n";
}
