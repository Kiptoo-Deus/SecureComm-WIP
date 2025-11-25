#include <iostream>
#include <thread>
#include <chrono>
#include <carrierbridge/include/carrierbridge.h>


int main() {
    try {
        CBServer server(55000);  // fixed port broker
        server.init();

        std::string my_name;
        std::cout << "Enter your username: ";
        std::getline(std::cin, my_name);

        server.register_user(my_name);

        server.set_message_callback([&](const std::string& to, const std::string& msg) {
            std::cout << "[Message Received] " << to << ": " << msg << "\n";
            });

        std::cout << "Type messages as: <recipient> <message>\n";

        while (true) {
            std::string line;
            std::getline(std::cin, line);
            if (line.empty()) continue;

            auto space = line.find(' ');
            if (space == std::string::npos) {
                std::cout << "Invalid format. Use: <recipient> <message>\n";
                continue;
            }

            std::string recipient = line.substr(0, space);
            std::string message = line.substr(space + 1);
            server.send_message(recipient, message);
        }

        server.shutdown();
    }
    catch (std::exception& e) {
        std::cerr << "[Error] " << e.what() << "\n";
    }
}
