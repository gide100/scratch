#include <atomic>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class Shutdown {
public:
    Shutdown()
        : is_signal_received_ (false), wait_time_(10),
        signalService_(),
        signals_(signalService_, SIGINT, SIGTERM, SIGQUIT) {
        std::cout<<"constructor"<<std::endl;
    }
    ~Shutdown() {
        signals_.cancel();
        signalService_.stop();
        signalThread_.join();
    }

    void init() {
        std::cout<<"Init "<<std::endl;
        signals_.async_wait(boost::bind(&Shutdown::handleStop, this, _1, _2));
        signalThread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &signalService_));
        std::cout<<"Init Completed"<<std::endl;
    }

    bool isSignalReceived() const {
        return is_signal_received_;
    }
    int waitTime() {
	return wait_time_;	
    }

private:
    std::atomic<bool> is_signal_received_;
    std::atomic<int> wait_time_;
    boost::asio::io_service signalService_;
    boost::thread signalThread_;
    boost::asio::signal_set signals_;

    void handleStop(const boost::system::error_code& error, int signal_number) {
        is_signal_received_ = true;
        wait_time_ = 0;
        myHandleStop(error, signal_number);
    }

    virtual void myHandleStop(const boost::system::error_code& error, int signal_number) = 0;
};

class MyShutdown: public Shutdown {
private:
    void myHandleStop(const boost::system::error_code& error, int signal_number) {
        std::cout << "Executing Safe Shutdown signal=" << signal_number << std::endl;
        exit(error ? 1 : 0);
    }
};

int main() {
    MyShutdown safeShutdown;
    safeShutdown.init();
    while (!safeShutdown.isSignalReceived()) {
        std::cout<<"Waiting ctl-c"<<std::endl;
        sleep(safeShutdown.waitTime());
    }
}
