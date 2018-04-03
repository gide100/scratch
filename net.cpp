#include <experimental/net>
#include <string>
#include <iostream>
namespace net = std::experimental::net;
int main() {
    net::ip::tcp::iostream s("www.boost.org", "http");
    s << "GET / HTTP/1.0\r\n";
    s << "Host: www.boost.org\r\n";
    s << "Accept: */*\r\n";
    s << "Connection: close\r\n\r\n";
    std::string header;
    while(std::getline(s, header) && header != "\r") {
        std::cout << header << "\n";
    }
    std::cout << s.rdbuf();
}
