#ifndef AN_SHUTDOWN_HPP
#define AN_SHUTDOWN_HPP

#include <atomic>
#include <iostream>
#include <boost/asio.hpp>


class DoWait {
    public:
        DoWait(boost::asio::deadline_timer* timer, std::atomic<int>* count) : timer_(timer), count_(count), wait_time_(1) {}
        void operator()(const boost::system::error_code& ec) {
            std::cerr << "[DoWait] iteration = " << *count_ << std::endl;
            if (!ec && *count_ > 0) {
                ++(*count_);
                timer_->expires_at(timer_->expires_at() + wait_time_);
                timer_->async_wait( DoWait(timer_, count_) );
            } else {
                // DO NOTHING, don't resubmit to queue.
            }
        }
    private:
        boost::asio::deadline_timer* timer_;
        std::atomic<int>* count_;
        boost::posix_time::seconds wait_time_;
};


class Shutdown {
    public:
        Shutdown()
              : signal_not_received_(1),
              signalContext_(),
              signals_(signalContext_, SIGINT, SIGTERM, SIGQUIT),
              doWaitTimer_(signalContext_, boost::posix_time::seconds(1)) {
            std::cout<<"constructor"<<std::endl;
            signals_.add(SIGILL);
            signals_.add(SIGHUP);
            
            // Setup timer and signals
            signals_.async_wait( [&] (const boost::system::error_code& ec, int s) { handleStop(ec, s); } );
            doWaitTimer_.async_wait( DoWait(&doWaitTimer_, &signal_not_received_) );
            signalThread_ = std::thread( [&]() { signalContext_.run(); } );
              
        }
        virtual ~Shutdown() {
            std::cout<<"~dtor"<<std::endl;
            signals_.cancel();
            signalContext_.stop();
            //signalThread_.join();
        }

        bool isSignalReceived() const {
            return signal_not_received_!=0;
        }

        void join() {
            std::cout << "Join (wait)" << std::endl;
            signalThread_.join();
        }
    private:
        std::atomic<int> signal_not_received_; // Zero idicates signal received
        std::atomic<int> wait_time_;
        boost::asio::io_context signalContext_;
        std::thread signalThread_;
        boost::asio::signal_set signals_;
        
        boost::asio::deadline_timer doWaitTimer_;

        void handleStop(const boost::system::error_code& error, int signal_number) {
            signal_not_received_ = 0;
            myHandleStop(error, signal_number);
        }

        virtual void myHandleStop(const boost::system::error_code& error, int signal_number) = 0;
};

class MyShutdown: public Shutdown {
public:
    MyShutdown() : Shutdown() {}
    virtual ~MyShutdown() { }
    MyShutdown(const MyShutdown& shut) = delete;
    MyShutdown& operator=(const MyShutdown& shut) = delete;
private:
    void myHandleStop(const boost::system::error_code& error, int signal_number) {
        std::cout << "Executing Safe Shutdown signal=" << signal_number << std::endl;
    }
};

#endif
