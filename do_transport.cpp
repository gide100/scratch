#include "transport.hpp" 
#include <iostream>

int main() {                                                                                                  
    try {                                                                                                       
        const std::uint16_t myPort = 5050;                                                                           
        // Instinate new chat_handler each time client connects                                                 
        std::cout << "Starting server on port=" << myPort << std::endl;                                         
        an::asio_generic_server<an::chat_handler> server;                                                               
        server.start_server(myPort);                                                                            
        server.wait_for(); //Join                                                                               
        std::cout << "Exiting server on port=" << myPort << std::endl;                                          
    } catch(std::exception& e) {                                                                                
        std::cout << e.what() << std::endl ;                                                                    
    }                                                                                                           
}
