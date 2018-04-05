#include "courier.hpp"
#include "matching_engine.hpp"

an::Courier::~Courier() {
    me_ = nullptr;
    stats_.inscribed_destinations = 0;
}

std::string an::Courier::to_sting() const {
   std::ostringstream os;
   return os.str();
}


void an::Courier::send(Response& r) {
    ++stats_.response_msgs;
    std::cout << "Courier::send Response:" << r.to_string() << std::endl;
}

void an::Courier::send(TradeReport& tr) {
    ++stats_.trade_report_msgs;
    std::cout << "Courier::send TradeReport:" << tr.to_string() << std::endl;
}

void an::Courier::send(MarketData& md) {
    ++stats_.market_data_msgs;
    std::cout << "Courier::send MarketData:" << md.to_string() << std::endl;
}

void an::Courier::receive(std::unique_ptr<Order> o) {
    ++stats_.receive_msgs;
    if (me_ != nullptr) {
        std::cout << "Courier::receive Order engine:" << o->to_string() << std::endl;
        Order* order = o.get();
        o.release(); // MatchingEngine now owns order
        order->applyOrder(*me_);
    } else {
        ++stats_.dropped_msgs;
        std::cout << "Courier::receive Order dropped:" << o->to_string() << std::endl;
    }
}


void an::Courier::inscribe(an::location_t destination, MatchingEngine* me) {
    assert(!destination.empty() && "Courier::inscribe destination empty");
    assert(me != nullptr && "Courier::inscribe matching engine null");

    destination_ = destination;
    me_ = me;
    stats_.inscribed_destinations = 1;
}

