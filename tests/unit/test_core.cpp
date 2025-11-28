#include "libcarb/carrierbridge.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    libcarb::CBServer srv(55000);
    srv.init();

    bool got = false;
    srv.set_message_callback([&](const std::string& to, const std::string& msg){
        std::cout << "[TEST] callback: " << to << " -> " << msg << "\n";
        got = true;
    });
    srv.register_user("testuser");
    srv.send_message("testuser", "hello test");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    srv.shutdown();
    std::cout << "[TEST] done\n";
    return 0;
}
