#include <vector>
#include <set>
#include <unordered_map>
#include <deque>
#include <string>
#include <exception>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

// See for changes to interface in 1.66
// http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/net_ts.html
//
//
//



// *** SERVER ***
template <typename ConnectionHandler>
class asio_generic_server {
    // Shared pointer to the type of the connection handler
    using shared_handler_t = std::shared_ptr<ConnectionHandler>; 

    public:
        // *** BROADCAST to all CLients ***
        typedef std::string msg_t;
        typedef std::deque<msg_t> chat_message_queue;
        class ClientBroadcast {
            public:
                ClientBroadcast(std::size_t max_recent_msgs = 100) : max_recent_msgs_(max_recent_msgs) {
                }
                ~ClientBroadcast() { }

                void join(shared_handler_t participant, msg_t client_name = "") {
                    participants_.insert(participant);
                    if (client_name != "") {
                        conn_name_.emplace(client_name, participant);
                    }
                    for (const msg_t& msg: recent_msgs_) {
                        std::cout << "ClientBroadcast::Join " << msg << " " << participants_.size() << std::endl;
                        participant->send(msg);
                    }
                }

                void leave(shared_handler_t participant) {
                    participants_.erase(participant);
                    for (auto it = conn_name_.begin; it != conn_name_.end(); ) {
                        if (it->second == participant) {
                            it = conn_name_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }

                void deliver(const msg_t& msg, const msg_t& client_name = "") {
                    if (client_name == "") {
                        recent_msgs_.push_back(msg);
                        while (recent_msgs_.size() > max_recent_msgs_) {
                            recent_msgs_.pop_front();
                        }
                        for (auto participant: participants_) {
                            participant->send(msg);
                            std::cout << "ClientBroadcast::deliver " << msg << participants_.size() <<  std::endl;
                        }
                    } else {
                        auto it = conn_name_.find(client_name);
                        if (it != conn_name_.end()) {
                            std::cout << "ClientBroadcast::deliver to=" << it->first << " " << msg << participants_.size() <<  std::endl;
                            it->second->send(msg);
                        }
                    }
                }
            private:
                std::set<shared_handler_t> participants_;
                std::unordered_map<msg_t, shared_handler_t> conn_name_;
                std::size_t max_recent_msgs_;
                chat_message_queue recent_msgs_;
        };

        asio_generic_server(int thread_count=1, long max_msgs=100) // TODO
            :  thread_count_(thread_count), acceptor_(io_context_), broadcast_(max_msgs) {
        }

        void start_server(uint16_t port);
        void wait_for(); // Join
        void send(const msg_t& msg, const msg_t& to = "") {
            broadcast_.deliver(msg, to);
        }
    private:
        void handle_new_connection(shared_handler_t handler, boost::system::error_code const& error);

        int thread_count_;
        std::vector<std::thread> thread_pool_;
        boost::asio::io_context io_context_; // Server
        boost::asio::ip::tcp::acceptor acceptor_; // Listening
        ClientBroadcast broadcast_;
};



template<typename ConnectionHandler>
void asio_generic_server<ConnectionHandler>::start_server(uint16_t port) {
    // Shared pointer to the type handling the connection
    auto handler = std::make_shared<ConnectionHandler>(io_context_, broadcast_);

    // set up the acceptor to listen on the tcp port
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
  
    acceptor_.async_accept(
        handler->socket(), [=](auto ec) {
            // Pass handler and error code
            handle_new_connection(handler,ec);
        }
    );

    // start pool of threads to process the asio events
    for(int i=0; i < thread_count_; ++i) {
        thread_pool_.emplace_back( [=]{io_context_.run();} );
    }
}

template<typename ConnectionHandler>
void asio_generic_server<ConnectionHandler>::wait_for() {
    // Join the threads
    for(int i=0; i < thread_count_; ++i) {
        thread_pool_[i].join();
    }
}

template<typename ConnectionHandler>
void asio_generic_server<ConnectionHandler>::handle_new_connection( 
       shared_handler_t handler, boost::system::error_code const & error ) {
    if(error){ 
        std::cout << "ERROR asio_generic_server<ConnectionHandler>::handle_new_connection" << std::endl;
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




// CRTP allows us to inject behaviour to get shared pointer to self at any time
// this allow it to control its own lifetime.
// Communicates with the client
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
            std::cout << "chat_handler::send " << msg << std::endl;
            context_.post( 
                write_strand_.wrap(
                    [me=shared_from_this(),msg]() {
                         me->queue_message(msg);
                    } 
                )
            );
// https://github.com/boostorg/asio/issues/79

//       boost::asio::post(context_, 
//           boost::asio::bind_executor(
//              write_strand_, 
//              [me=shared_from_this(),msg]() {
//                  me->queue_message(msg);
//              } 
//           )
//       );
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
                start_packet_send();
            }
        }

        boost::asio::io_context&        context_;
        boost::asio::ip::tcp::socket    socket_;
        boost::asio::io_context::strand write_strand_; // Prevents multiple writes to port
        boost::asio::streambuf          in_packet_;
        std::deque<std::string>         send_packet_queue_;
        asio_generic_server<chat_handler>::ClientBroadcast&   broadcast_;
};


void chat_handler::read_packet() {
    boost::asio::async_read_until( 
        socket_,    // From
        in_packet_, // Destination
        '\n',       // Terminator
        // Completion handler, me is a shared pointer
        [me=shared_from_this()]( boost::system::error_code const& ec, std::size_t bytes_xfer) {
            me->read_packet_done(ec, bytes_xfer);
        } 
    );
}


void chat_handler::read_packet_done( boost::system::error_code const& error, std::size_t bytes_transferred ) {
    if (error) { 
        std::cout << "ERROR chat_handler::read_packet_done" << std::endl;
        return; 
    }

    std::istream stream(&in_packet_);
    std::string packet_string;
    std::getline(stream >> std::ws, packet_string); // Read whole line including spaces
    //stream >> packet_string;

    std::cout << "port=" << socket_.remote_endpoint() << " bytes=" << bytes_transferred << " GOT: "<< packet_string << std::endl; //TODO
    // do something with it
    if (packet_string == "quit") {
        send("QUIT"); send("\n"); //TODO - remove echo
        std::cout << "QUIT!" << std::endl; //TODO
    } else if (packet_string.find("client") == 0) { // client.Client1
        packet_string.erase(0,6); // .Client1
        if (!packet_string.empty() && packet_string[0] == '.') { // .Client1
            packet_string.erase(0,1); // Client1
            broadcast_.join(shared_from_this(),packet_string);
        }
    } else if (packet_string.find("send") == 0) { // send.Client1
        packet_string.erase(0,4); // .Client1
        if (!packet_string.empty() && packet_string[0] == '.') { // .Client1
            packet_string.erase(0,1); // Client1
            broadcast_.deliver("testing\n",packet_string);
        }
    } else {
        send(packet_string); send("\n"); //TODO - remove echo
    }
    read_packet();  
}


void chat_handler::start_packet_send() {
    send_packet_queue_.front() += "\0";
    std::cout << "chat_handler::start_packet_send " << send_packet_queue_.front() << std::endl;
    boost::asio::async_write( socket_, 
        boost::asio::buffer(send_packet_queue_.front()), // Pass location in deque
            write_strand_.wrap( 
                [me=shared_from_this()]( boost::system::error_code const& ec, std::size_t) {
                    me->packet_send_done(ec);
                }
            )
    );
  
//   boost::asio::async_write( socket_, 
//       boost::asio::buffer(send_packet_queue_.front()), 
//         boost::asio::bind_executor(write_strand_,  // aka wrap
//               [me=shared_from_this()]( boost::system::error_code const& ec, std::size_t) {
//                 me->packet_send_done(ec);
//               }
//         )
//     ) ;
}


void chat_handler::packet_send_done(boost::system::error_code const& error) {
    if(!error) {
        send_packet_queue_.pop_front(); // Remove from deque
        if(!send_packet_queue_.empty()) {  // More work? do it
            start_packet_send();
        }
    } else {
        std::cout << "ERROR chat_handler::packet_send_done " << boost::system::system_error(error).what() << std::endl;
    }
}


int main() {
  try {
      const uint16_t myPort = 5050;
      // Instinate new chat_handler each time client connects
      std::cout << "Starting server on port=" << myPort << std::endl;
      asio_generic_server<chat_handler> server;
      server.start_server(myPort);
      server.wait_for(); //Join
      std::cout << "Exiting server on port=" << myPort << std::endl;
  } catch(std::exception& e) {
      std::cout << e.what() << std::endl ;
  }
}
