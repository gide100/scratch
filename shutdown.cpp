#include "shutdown.hpp"

int main() {
    MyShutdown safeShutdown;
    safeShutdown.init();
    safeShutdown.join();
    return 0;
}
