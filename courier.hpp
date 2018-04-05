#ifndef AN_COURIER_HPP
#define AN_COURIER_HPP

#include "types.hpp"
#include "order.hpp"
namespace an {


class MatchingEngine;

struct courier_stats_t {
    courier_stats_t() 
        : response_msgs(0), trade_report_msgs(0), market_data_msgs(0), 
          receive_msgs(0), dropped_msgs(0), inscribed_destinations(0) {}
    counter_t       response_msgs;
    counter_t       trade_report_msgs;
    counter_t       market_data_msgs;
    counter_t       receive_msgs;
    counter_t       dropped_msgs; // No destination or no matching engine
    counter_t       inscribed_destinations;
};

class Courier {
    public:
        Courier() : destination_(""), me_(nullptr), stats_() {}
        ~Courier();

        std::string to_sting() const;

        void send(Response& r);
        void send(TradeReport& tr);
        void send(MarketData& md);

        void receive(std::unique_ptr<Order> o);

        void inscribe(an::location_t destination, MatchingEngine* me);

        const courier_stats_t& stats() const {
            return stats_;
        }
    protected:
        location_t      destination_;
        MatchingEngine* me_;
        courier_stats_t stats_;
};

} // an - namespace

#endif

