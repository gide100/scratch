#include <vector>
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

// *** SERVER ***
template <typename ConnectionHandler>
class asio_generic_server {
    // Shared pointer to the type of the connection handler
    using shared_handler_t = std::shared_ptr<ConnectionHandler>; 
    public:
        asio_generic_server(int thread_count=1)
            : thread_count_(thread_count), acceptor_(io_context_) {
        }

        void start_server(uint16_t port);
        void wait_for(); // Join
    private:
        void handle_new_connection(shared_handler_t handler, boost::system::error_code const& error);

        int thread_count_;
        std::vector<std::thread> thread_pool_;
        boost::asio::io_context io_context_; // Server
        boost::asio::ip::tcp::acceptor acceptor_; // Listening
};


// CRTP allows us to inject behaviour to get shared pointer to self at any time
// this allow it to control its own lifetime.
// Communicates with the client
class chat_handler : public std::enable_shared_from_this<chat_handler> {
    public:
        chat_handler(boost::asio::io_context& context)
            : context_(context), socket_(context_), write_strand_(context_) {
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

        boost::asio::io_context& context_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::io_context::strand write_strand_; // Prevents multiple writes to port
        boost::asio::streambuf in_packet_;
        std::deque<std::string> send_packet_queue_;
};



template<typename ConnectionHandler>
void asio_generic_server<ConnectionHandler>::start_server(uint16_t port) {
    // Shared pointer to the type handling the connection
    auto handler = std::make_shared<ConnectionHandler>(io_context_);

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
    if(error){ return; } // Bail

    handler->start(); // Start the handler

    // Create new handler for next connnection that will come in.
    auto new_handler = std::make_shared<ConnectionHandler>(io_context_);

    // Start accept with myself
    acceptor_.async_accept(
        new_handler->socket(), [=](auto ec) {
            handle_new_connection(new_handler,ec);
        }
    );
}


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
    if (error) { return; }

    std::istream stream(&in_packet_);
    std::string packet_string;
    std::getline(stream >> std::ws, packet_string); // Read whole line including spaces
    //stream >> packet_string;

    std::cout << bytes_transferred << " GOT: "<< packet_string << std::endl; //TODO
    // do something with it
    if (packet_string == "quit") {
        std::cout << "QUIT!"<< std::endl; //TODO
    } else {
        send(packet_string); send("\n"); //TODO - remove echo
        read_packet();  
    }
}


void chat_handler::start_packet_send() {
    send_packet_queue_.front() += "\0";
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
