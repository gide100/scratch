#ifndef AN_SHUTDOWN_HPP
#define AN_SHUTDOWN_HPP

#include <atomic>
#include <iostream>
#include <boost/asio.hpp>


// Every second (wait_time) check if signal has been received
class DoWait {
    public:
        DoWait(boost::asio::steady_timer& timer, std::atomic<int>& count, std::chrono::seconds wait_time) 
            : timer_(timer), count_(count), wait_time_(wait_time) {}
        void operator()(const boost::system::error_code& ec) {
            std::cerr << "[DoWait] iteration = " << count_ << std::endl;
            if (!ec && count_ > 0) {
                ++count_;
                timer_.expires_at(timer_.expires_at() + wait_time_); // Reset in wait_time seconds
                timer_.async_wait( DoWait(timer_, count_, wait_time_) ); // Resubmit to queue
            } else {
                // DO NOTHING, don't resubmit to queue.
            }
        }
    private:
        boost::asio::steady_timer& timer_;
        std::atomic<int>& count_;
        const std::chrono::seconds wait_time_;
};


// Register signals, reset counter when signal is received
class Shutdown {
    public:
        Shutdown()
            : signal_not_received_(1),
              signalContext_(),
              signals_(signalContext_) {
            std::cout<<"constructor"<<std::endl;

            // Setup signals
            signals_.add(SIGINT);
            signals_.add(SIGTERM);
            signals_.add(SIGQUIT);
            signals_.add(SIGILL);
            signals_.add(SIGHUP);
            signals_.async_wait( [&] (const boost::system::error_code& ec, int s) { handleStop(ec, s); } );
            signalThread_ = std::thread( [&]() { signalContext_.run(); } );
        }

        virtual ~Shutdown() {
            std::cout<<"~dtor"<<std::endl;
            signals_.cancel();
            signalContext_.stop();
            //signalThread_.join();
        }
        Shutdown(const Shutdown& shut) = delete;
        Shutdown& operator=(const Shutdown& shut) = delete;

        bool isSignalReceived() const {
            return signal_not_received_!=0;
        }

        void join() {
            std::cout << "Join (wait)" << std::endl;
            signalThread_.join();
        }
    protected:
        // TODO: Override this to call shutdown code
        virtual void myHandleStop(const boost::system::error_code& error, int signal_number) = 0;

        void handleStop(const boost::system::error_code& error, int signal_number) {
            signal_not_received_ = 0;
            myHandleStop(error, signal_number);
        }

        std::atomic<int> signal_not_received_; // Zero indicates signal received
        boost::asio::io_context signalContext_;
        std::thread signalThread_;
        boost::asio::signal_set signals_;
};

#endif

