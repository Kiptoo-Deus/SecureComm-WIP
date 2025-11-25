#include "carrierbridge.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "../carrierbridge/core.cpp"

int main() {
    cb_init();

    cb_register("Joel");
    cb_register("Kiptoo");

    // callback to print messages
    CBServer server;
    server.set_message_callback([](const std::string& to, const std::string& msg) {
        std::cout << "[Message Received] " << to << ": " << msg << "\n";
        });

    // simulate messages
    cb_send_message("Kiptoo", "Hello Kiptoo!");
    cb_send_message("Joel", "Hello Joel!");

    std::cout << "Press Enter to exit...\n";
    std::cin.get();

    cb_shutdown();
    return 0;
}
