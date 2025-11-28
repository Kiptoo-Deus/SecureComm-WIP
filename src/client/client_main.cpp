#include "libcarb/carrierbridge.h"
#include <iostream>
#include <string>

int main() {
    try {
        libcarb::CBServer client(55000);
        client.init();

        std::string username;
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);

        client.register_user(username);

        client.set_message_callback([&](const std::string& from, const std::string& msg){
            // print any routed message
            std::cout << "[" << from << "] " << msg << "\n";
        });

        std::cout << "Type messages as: <recipient> <message>\n";

        while (true) {
            std::string line;
            std::getline(std::cin, line);
            if (line.empty()) continue;
            auto space = line.find(' ');
            if (space == std::string::npos) {
                std::cout << "Invalid format\n";
                continue;
            }
            std::string recipient = line.substr(0, space);
            std::string message = line.substr(space + 1);
            client.send_message(recipient, message);
        }

        client.shutdown();
    } catch (std::exception& e) {
        std::cerr << "client error: " << e.what() << "\n";
    }
    return 0;
}
