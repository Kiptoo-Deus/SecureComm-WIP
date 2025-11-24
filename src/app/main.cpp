#include "carrierbridge.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "../carrierbridge/core.cpp"

int main() {
    cb_init();

    cb_register("Joel");
    cb_register("Kiptoo");

    cb_send_message("Kiptoo", "Hello Bob!");
    cb_send_message("Joel", "Hello Alice!");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    cb_shutdown();
    return 0;
}
