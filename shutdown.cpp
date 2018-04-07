#include "shutdown.hpp"

class MyShutdown: public Shutdown {
    public:
        const std::chrono::seconds WAIT_TIME = std::chrono::seconds(1);
        MyShutdown() : Shutdown(), doWaitTimer_(signalContext_, WAIT_TIME)  {
            doWaitTimer_.async_wait( DoWait(doWaitTimer_, signal_not_received_, WAIT_TIME) );
        }
        virtual ~MyShutdown() { }
    protected:
        virtual void myHandleStop(const boost::system::error_code& error, int signal_number) {
            std::cout << "Executing Safe Shutdown signal=" << signal_number << std::endl;
        }
    private:
        boost::asio::steady_timer doWaitTimer_;
};

int main() {
    MyShutdown safeShutdown;
    safeShutdown.join();
    return 0;
}
