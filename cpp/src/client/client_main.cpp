#include "libcarb/carrierbridge.h"
#include <iostream>
#include <string>

int main() {
    libcarb::CClient client(0, "127.0.0.1", 55000);
    client.init();

    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    client.register_user(username);

    client.set_message_callback([&](const std::string& from, const std::string& msg){
        std::cout << "<" << from << "> " << msg << std::endl;
    });

    std::cout << "Commands:\n";
    std::cout << "  msg <recipient> <text>\n";
    std::cout << "  call <from> <to>\n";

    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line.rfind("msg ", 0) == 0) {
            auto rest = line.substr(4);
            auto sp = rest.find(' ');
            if (sp == std::string::npos) continue;
            auto to = rest.substr(0, sp);
            auto text = rest.substr(sp + 1);
            client.send_message(to, text);
        }
        else if (line.rfind("call ", 0) == 0) {
            auto rest = line.substr(5); // expects "<from> <to>"
            client.call_user(rest);
        }
    }

    client.shutdown();
    return 0;
}
