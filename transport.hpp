#ifndef AN_TRANSPORT_HPP
#define AN_TRANSPORT_HPP

#include "types.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

// See for changes to interface in 1.66
// http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/net_ts.html
//
//
//

namespace an {

// *** SERVER ***
template <typename ConnectionHandler>
class asio_generic_server {
    // Shared pointer to the type of the connection handler, this is the connectionbetween the client 
    // and the server. Who owns it? When it has nothing to send and no ability to read, or no internal
    // work. It stays alive.
    using shared_handler_t = std::shared_ptr<ConnectionHandler>;

    public:
        // *** BROADCAST to all Clients ***
        typedef std::string msg_t;
        typedef std::deque<msg_t> chat_message_queue;
        class ClientBroadcast {
            public:
                ClientBroadcast(std::size_t max_recent_msgs = 100) : max_recent_msgs_(max_recent_msgs) {
                }
                ~ClientBroadcast() { }

                void join(shared_handler_t participant, msg_t client_name = "");

                void leave(shared_handler_t participant);

                void deliver(const msg_t& msg, const msg_t& client_name = "");
            private:
                std::set<shared_handler_t> participants_;
                std::unordered_map<msg_t, shared_handler_t> conn_name_;
                std::size_t max_recent_msgs_;
                chat_message_queue recent_msgs_;
        };

        asio_generic_server(int thread_count=1, long max_msgs=100) // TODO
            :  thread_count_(thread_count), acceptor_(io_context_), broadcast_(max_msgs) {
        }

        void start_server(std::uint16_t port);
        void wait_for(); // Join
        void send(const msg_t& msg, const msg_t& to = "") {
            broadcast_.deliver(msg, to);
        }
    private:
        // New connection comes in this is called.
        void handle_new_connection(shared_handler_t handler, boost::system::error_code const& error);

        int thread_count_;
        std::vector<std::thread> thread_pool_;
        boost::asio::io_context io_context_; // Server
        boost::asio::ip::tcp::acceptor acceptor_; // Listening
        ClientBroadcast broadcast_;
};

template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::start_server(std::uint16_t port) {
    // Shared pointer to the type handling the connection
    auto handler = std::make_shared<ConnectionHandler>(io_context_, broadcast_); // IO service by reference

    // set up the acceptor to listen on the tcp port
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    acceptor_.async_accept(
        handler->socket(), [=](auto ec) {
            // Pass handler and error code
            handle_new_connection(handler,ec); // Completion handler
        }
    );

    // start pool of threads to process the asio events
    for(int i=0; i < thread_count_; ++i) {
        thread_pool_.emplace_back( [=]{io_context_.run();} );
    }
}

template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::wait_for() {
    // Join the threads
    for(int i=0; i < thread_count_; ++i) {
        thread_pool_[i].join();
    }
}

template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::handle_new_connection(
       shared_handler_t handler, const boost::system::error_code& error ) {
    if(error){
        //std::cout << "ERROR asio_generic_server<ConnectionHandler>::handle_new_connection" << std::endl;
        return; // Bail
    }

    handler->start(); // Start the handler
    broadcast_.join(handler);
    broadcast_.deliver("Welcome Client\n");

    // Create new handler for next connnection that will come in.
    auto new_handler = std::make_shared<ConnectionHandler>(io_context_, broadcast_);

    // Start accept with myself
    acceptor_.async_accept(
        new_handler->socket(), [=](auto ec) {
            handle_new_connection(new_handler,ec);
        }
    );
}



// CRTP allows us to inject behaviour to get shared pointer to itself at any time
// this allows it to control its own lifetime.
// Communicates with the client
// Instantiate a new one of these each time a client connects.
class chat_handler : public std::enable_shared_from_this<chat_handler> {
    public:
        chat_handler(boost::asio::io_context& context,
                     asio_generic_server<chat_handler>::ClientBroadcast& broadcast)
            : context_(context), socket_(context_), write_strand_(context_), broadcast_(broadcast) {
        }

        boost::asio::ip::tcp::socket& socket() {
            return socket_; // Utilised elsewhere
        }

        void start() {
            read_packet(); // Start our operations
        }

        // Can't write to multiple times to the send.
        // Post to queue the work to get done.
        void send(std::string msg) {
            //std::cout << "chat_handler::send " << msg << std::endl;
            context_.post(
                write_strand_.wrap(
                    [me=shared_from_this(),msg]() {
                         me->queue_message(msg);
                    }
                )
            );
         }
    private:
        void read_packet();
        void read_packet_done( boost::system::error_code const& error, std::size_t bytes_transferred ) ;

        void start_packet_send();
        void packet_send_done(boost::system::error_code const& error);

        void queue_message(std::string message) {
            bool write_in_progress = !send_packet_queue_.empty(); // Write in progress, wrapped in strand
            send_packet_queue_.push_back(std::move(message));

            if(!write_in_progress) {
                // Start sending
                start_packet_send();
            }
        }

        boost::asio::io_context&        context_;
        boost::asio::ip::tcp::socket    socket_; // Socket the client communicates on
        boost::asio::io_context::strand write_strand_; // Prevents multiple writes to port
        boost::asio::streambuf          in_packet_; // Data coming in
        std::deque<std::string>         send_packet_queue_; // Data going out
        asio_generic_server<chat_handler>::ClientBroadcast&   broadcast_;
};

} // an - namespace
#endif

