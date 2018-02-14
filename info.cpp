
#include <vector>
#include <boost/shared_ptr.hpp>

// *** SERVER ***
template <typename ConnectionHandler>
class asio_generic_server {
  using shared_handler_t = std::shared_ptr<ConnectionHandler>;
  public:
    asio_generic_server(int thread_count=1) : 
          thread_count_(thread_count), acceptor_(io_service_) {
    }
    
    void start_server(uint16_t port);
  private:
    void handle_new_connection( 
      shared_handler_t handler, 
      system::error_code const & error ) {
    }
    
    int thread_count_;
    std::vector<std::thread> thread_pool_;
    boost:asio::io_service io_service_;
    boost::asio::ip::tcp::acceptor acceptor_; # Listening
};


void asio_generic_server::start_server(uint16_t port) {
  auto handler = std::make_shared<ConnectionHandler>(io_service_);

  // set up the acceptor to listen on the tcp port
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true)); //???
  acceptor_.bind(endpoint);
  acceptor_.listen();
  
  acceptor_.async_accept(handler->socket(), [=](auto ec) 
    {
      handle_new_connection(handler,ec);
    }
  );


  // start pool of threads to process the asio events
  for(int i=0; i < thread_count_; ++i) {
    thread_pool_.emplace_back( [=]{io_service_.run();} );
  }
}


void asio_generic_server::handle_new_connection( 
    shared_handler_t handler, system::error_code const & error ) {
  if(error){ return; }
  
  handler->start();

  auto new_handler = std::make_shared<ConnectionHandler>(io_service_);
  
  acceptor_.async_accept(
      new_handler->socket(), [=](auto ec) {
                                handle_new_connection(new_handler,ec);
                              }
  );
}

// CRTP allows us to inject behaviour to get shared pointer to self at any time
// this allow us to control our own lifetime.
class chat_handler : public std::enable_shared_from_this<chat_handler> {
  public:
    chat_handler(asio::io_service& service) : 
           service_(service), socket_(service), write_strand_(service) {
    }
    
    boost::asio::ip::tcp::socket& socket() {
      return socket_; // Utilised elsewhere
    }

    void start() {
      read_packet(); // Start our operations
    }
    
    void send(std::string msg){
      service_.post( write_strand_.wrap( [me=shared_from_this()]() {
        me->queue_message(msg);
      } ));
    }
    
  private:
    
    void read_packet();
    void read_packet_done( system::error_code const& error, std::size_t bytes_transferred ) ;
    
    void start_packet_send();
    void packet_send_done(system::error_code const& error);
    
    void queue_message(std::string message) {
      bool write_in_progress = !send_packet_queue.empty();
      send_packet_queue.push_back(std::move(message));
      
      if(!write_in_progress) {
        start_packet_send();
      }
    }
    
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_; // Prevents multiple writes to port
    boost::asio::streambuf in_packet_;
    std::deque<std::string> send_packet_queue;
};


void chat_handler::read_packet() {
  asio::async_read_until( 
      socket_,    // From
      in_packet_, // Destination
      ’\0’,       // Terminator
      // Completion handler, me is a shared pointer
      [me=shared_from_this()]( system::error_code const& ec, std::size_t bytes_xfer)
      {
        me->read_packet_done(ec, bytes_xfer);
      } 
  );
}


void chat_handler::read_packet_done( system::error_code const& error, std::size_t bytes_transferred ) {
  if(error){ return; }

  std::istream stream(&in_packet_);
  std::string packet_string;
  stream >> packet_string;
  
  // do something with it
  
  read_packet();  
}


void chat_handler::start_packet_send() {
  send_packet_queue.front() += "\0";
  async_write( socket_, 
      asio::buffer(send_packet_queue.front()), 
           write_strand_.wrap( 
             [me=shared_from_this()]( system::error_code const& ec, std::size_t) {
                me->packet_send_done(ec);
              }
           )
         );
}


void chat_handler::packet_send_done(system::error_code const& error) {
  if(!error) {
    send_packet_queue.pop_front();
    if(!send_packet_queue.empty()) { 
      start_packet_send();
    }
  }
}


int main() {
  // Instinate new chat_handler each time client connects
  asio_generic_server<chat_handler> server;
  server.start_server(8888);
}

