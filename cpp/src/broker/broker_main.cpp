#include "libcarb/carrierbridge.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};
void handle_sigint(int) { g_running = false; }

int main() {
    std::signal(SIGINT, handle_sigint);

    libcarb::CBroker broker(55000);
    broker.init();

    std::cout << "[Broker] Running on port 55000. Ctrl+C to stop.\n";

    while (g_running)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    broker.shutdown();
    return 0;
}
