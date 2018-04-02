#include "transport.hpp"
#include <iostream>

template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::ClientBroadcast::join(shared_handler_t participant, msg_t client_name) {                             
    participants_.insert(participant);                                                        
    if (client_name != "") {                                                                  
        conn_name_.emplace(client_name, participant);                                         
    }                                                                                         
    for (const msg_t& msg: recent_msgs_) {                                                    
        std::cout << "ClientBroadcast::Join " << msg << " " << participants_.size() << std::endl;
        participant->send(msg);                                                               
    }                                                                                         
}     

template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::ClientBroadcast::leave(shared_handler_t participant) {                                                    
    participants_.erase(participant);                                                         
    for (auto it = conn_name_.begin; it != conn_name_.end(); ) {                              
        if (it->second == participant) {                                                      
            it = conn_name_.erase(it);                                                        
        } else {                                                                              
            ++it;                                                                             
        }                                                                                     
    }                                                                                         
}


template<typename ConnectionHandler>
void an::asio_generic_server<ConnectionHandler>::ClientBroadcast::deliver(const msg_t& msg, const msg_t& client_name) {                               
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
            std::cout << "ClientBroadcast::deliver to=" << it->first << " " << msg 
                      << participants_.size() <<  std::endl;
            it->second->send(msg);                                                            
        }                                                                                     
    }                                                                                         
}



void an::chat_handler::read_packet() {
    boost::asio::async_read_until( 
        socket_,    // From
        in_packet_, // Destination (stream buffer)
        '\n',       // Terminator
        // Completion handler, me is a shared pointer
        [me=shared_from_this()]( boost::system::error_code const& ec, std::size_t bytes_xfer) {
            me->read_packet_done(ec, bytes_xfer);
        } 
    );
}


void an::chat_handler::read_packet_done( boost::system::error_code const& error, std::size_t bytes_transferred ) {
    if (error) { 
        // On error fall out (don't re-queue read).
        std::cout << "ERROR chat_handler::read_packet_done" << std::endl;
        return; // bail 
    }

    std::istream stream(&in_packet_); // stream to string
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
    read_packet(); // Queue another read 
}


void an::chat_handler::start_packet_send() {
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
}


void an::chat_handler::packet_send_done(boost::system::error_code const& error) {
    if(!error) {
        send_packet_queue_.pop_front(); // Remove from deque
        if(!send_packet_queue_.empty()) {  // More work? do it
            start_packet_send();
        }
    } else {
        std::cout << "ERROR chat_handler::packet_send_done " 
                  << boost::system::system_error(error).what() << std::endl;
    }
}

