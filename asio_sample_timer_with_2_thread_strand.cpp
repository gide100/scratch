#include <iostream>
#include <string>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

void print(const boost::system::error_code& /*e*/,
    boost::asio::deadline_timer* t, int* count) {
    std::cerr << "[Timer Callback Func] Iteration = " << *count << std::endl; 
    if (*count > 0) {
        //std::cout << *count << std::endl;
        ++(*count);

        t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
        t->async_wait(boost::bind(print,boost::asio::placeholders::error, t, count)); 
    }
}

class DoWait {
    public:
        DoWait(boost::asio::deadline_timer* timer, std::atomic<int>* count) : timer_(timer), count_(count) {}
        void operator()(const boost::system::error_code& ec) {
            std::cerr << "[DoWait] iteration = " << *count_ << std::endl; 
            if (!ec && *count_ > 0) {
                ++(*count_);
                timer_->expires_at(timer_->expires_at() + boost::posix_time::seconds(1));
                timer_->async_wait( DoWait(timer_, count_) );
            } else {
                // DO NOTHING, don't resubmit to queue.
            }
        }
    private:
        boost::asio::deadline_timer* timer_;
        std::atomic<int>* count_;
};

void timer_expired(std::string id) {
    std::cout << boost::chrono::system_clock::now() << " " << id << " enter.\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << boost::chrono::system_clock::now() << " " << id << " leave.\n";
}

void callback(const boost::system::error_code& e, boost::asio::deadline_timer* pt, int* count) {
    std::cerr << "[Timer Callback Func] Iteration = " << *count << std::endl; 
    if (*count > 0) {
        ++(*count);
        //** Updates the absolute timer target time (when the callback function will be called again)
        pt->expires_at(pt->expires_at() + boost::posix_time::seconds(1)); 
        //pt->expires_from_now(boost::posix_time::milliseconds(1123)); 
        //** Reschedule a new job for the timer 
        //pt.async_wait(boost::bind(callback_func, boost::asio::placeholders::error, pt, count));
        pt->async_wait( [&](const boost::system::error_code& ec) { callback(ec, pt, count); } );
        //pt->async_wait(boost::bind(callback, boost::asio::placeholders::error, pt, count));
    }
}

void callback2(boost::asio::deadline_timer* pt, int* pcont) {
    std::cout << "[Timer Callback Func] Iteration = " << *pcont << std::endl; 
    if (*pcont) {
        //** Updates the absolute timer target time (when the callback function will be called again)
        pt->expires_from_now(boost::posix_time::seconds(1)); 
        //** Reschedule a new job for the timer 
        //pt.async_wait(boost::bind(callback_func, boost::asio::placeholders::error, pt, pcont));
        pt->async_wait( [&](auto ... vn) { callback2(pt, pcont); } );
    }
}

int main() {
    // Slushie shack
    boost::asio::io_context context;
    boost::asio::io_context::strand strand(context);

    boost::asio::deadline_timer timer1(context, boost::posix_time::seconds(5));
    boost::asio::deadline_timer timer2(context, boost::posix_time::seconds(5));
    boost::asio::deadline_timer timer3(context, boost::posix_time::seconds(1));
    boost::asio::deadline_timer timer4(context, boost::posix_time::seconds(20));

    // Non blocking call, Jonny. When timeout finishes calls completion handler to print ' ...timer expired'
    // Timer begins, waits 5 second, places handler on completion queue.
    //auto bb = boost::asio::bind_executor(strand, [](auto ... vn) { timer_expired("timer1"); } );
    //timer1.async_wait( bb );
    std::cout << boost::chrono::system_clock::now() << " : start\n";
    timer1.async_wait( strand.wrap( [](auto ... vn) { timer_expired("timer1"); } ) );
    timer2.async_wait( strand.wrap( [](auto ... vn) { timer_expired("timer2"); } ) );
    std::atomic<int> myCount(1);
    // 1) Functor
    timer3.async_wait( DoWait(&timer3, &myCount) );
    // 2) Lambda
//    timer3.async_wait(  [&](const boost::system::error_code& ec) {  
//                        callback(ec, &timer3,  &myCount);
//               n                         } );
    // 3) Boost bind
    //timer3.async_wait(boost::bind(print,
    //    boost::asio::placeholders::error, &timer3, &myCount));

//    context.post( [&] { callback2(&timer3, &myInt); } );
    timer4.async_wait( [&myCount](auto ... vn) { myCount = 0; } );
   
    //timer1.async_wait( boost::asio::bind_executor(strand, [](auto ... vn) { timer_expired("timer1"); } ) );
    //timer2.async_wait( boost::asio::bind_executor(strand, [](auto ... vn) { timer_expired("timer2"); } ) );

    // Thread that services the completion handler. Butler to slushy shack, blocks wait for all services to complete
    std::thread butler1( [&]() { context.run(); } );
    std::thread butler2( [&]() { context.run(); } );

    butler1.join();
    butler2.join();

    std::cout << "Final count is " << myCount << std::endl;
    std::cout << boost::chrono::system_clock::now() << " : done\n";
}
