#include "shutdown.hpp"

int main() {
    MyShutdown safeShutdown;
    safeShutdown.join();
    return 0;
}
