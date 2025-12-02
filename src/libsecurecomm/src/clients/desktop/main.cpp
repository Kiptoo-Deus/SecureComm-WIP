#include "securecomm/dispatcher.hpp"
#include <memory>
#include <thread>
#include <iostream>
#include <chrono>

extern "C" securecomm::Transport* create_inmemory_transport();

int main() {
    auto transport = std::shared_ptr<securecomm::Transport>(create_inmemory_transport());

    auto dispatcherA = std::make_shared<securecomm::Dispatcher>(transport);
    auto dispatcherB = std::make_shared<securecomm::Dispatcher>(transport);

    dispatcherA->register_device("alice");
    dispatcherB->register_device("bob");

    dispatcherA->start();
    dispatcherB->start();

    std::vector<uint8_t> root(32, 5);
    dispatcherA->create_session_with("bob", root);
    dispatcherB->create_session_with("alice", root);

    dispatcherA->set_on_inbound([](const securecomm::Envelope& env){
        auto pt = env.ciphertext; // in demo, plaintext already decrypted in dispatcher callback chain
        std::string s(pt.begin(), pt.end());
        std::cout << "Alice inbound: " << s << "\n";
    });

    dispatcherB->set_on_inbound([](const securecomm::Envelope& env){
        auto pt = env.ciphertext;
        std::string s(pt.begin(), pt.end());
        std::cout << "Bob inbound: " << s << "\n";
    });

    dispatcherA->send_message_to_device("bob", std::vector<uint8_t>({'H','i',' ','B','o','b'}));
    dispatcherB->send_message_to_device("alice", std::vector<uint8_t>({'H','i',' ','A','l','i','c','e'}));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    dispatcherA->stop();
    dispatcherB->stop();
    return 0;
}
