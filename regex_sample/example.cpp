#include <boost/regex.hpp>
#include <iostream>
#include <string>
#include <queue>
#include <vector>



template<class T, class C = std::vector<T>, class P = std::less<typename C::value_type> >
struct PriorityQueue : std::priority_queue<T,C,P> {
    //using std::priority_queue<T,C,P>::priority_queue;
    typename C::iterator begin() { return std::priority_queue<T, C, P>::c.begin(); }
    typename C::iterator end() { return std::priority_queue<T, C, P>::c.end(); }
    typename C::const_iterator begin() const { return std::priority_queue<T, C, P>::c.cbegin(); }
    typename C::const_iterator end() const { return std::priority_queue<T, C, P>::c.cend(); }
};

typedef PriorityQueue<int, std::deque<int>, std::less<int> > MQ;


int main()
{
    std::string line;
    boost::regex pat( "^Subject: (Re: |Aw: )*(.*)" );

    while (std::cin) {
        std::getline(std::cin, line);
        boost::smatch matches;
        if (boost::regex_match(line, matches, pat)) {
            std::cout << matches[2] << std::endl;
        }
    }

    MQ pq;
    pq.push(0);
    pq.push(1);
    pq.push(8);
    pq.push(4);
    pq.push(1);
    std::cout << "ITEMS" << std::endl;
    for (const auto& v : pq) {
        std::cout << v << std::endl;
    }

    std::cout << "SORT ORDER" << std::endl;
    MQ::container_type data;
    for (const auto &v : pq) {
        data.push_back(v);
    }
    std::sort( data.begin(), data.end(), std::not2(MQ::value_compare()) );
    for (auto &v : data) {
        std::cout << v << std::endl;
    }

    std::cout << "QUEUE ORDER" << std::endl;
    while (!pq.empty()) {
        std::cout << pq.top() << std::endl;
        pq.pop();
    }
}
