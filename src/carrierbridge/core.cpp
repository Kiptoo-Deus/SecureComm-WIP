#include "carrierbridge.h"
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <unordered_map>

struct CBServer::Impl {
    asio::io_context io;
    std::thread io_thread;
    std::unordered_map<std::string, std::string> registry;
    CBServer::MessageCallback msg_cb;
};

CBServer::CBServer() : pImpl(new Impl{}) {}
CBServer::~CBServer() { delete pImpl; }

void CBServer::init() {
    std::cout << "[CarrierBridge] Async server init\n";
    pImpl->io_thread = std::thread([this]() { pImpl->io.run(); });
}

void CBServer::shutdown() {
    pImpl->io.stop();
    if (pImpl->io_thread.joinable())
        pImpl->io_thread.join();
    std::cout << "[CarrierBridge] Async server shutdown\n";
}

void CBServer::register_user(const std::string& username) {
    pImpl->registry[username] = "registered";
    std::cout << "[CarrierBridge] Registered user: " << username << "\n";
}

void CBServer::send_message(const std::string& to, const std::string& message) {
    if (pImpl->msg_cb)
        pImpl->msg_cb(to, message);
}

void CBServer::set_message_callback(MessageCallback cb) {
    pImpl->msg_cb = cb;
}

// C API wrappers
void cb_init() { static CBServer srv; srv.init(); }
void cb_shutdown() { static CBServer srv; srv.shutdown(); }
void cb_register(const char* username) { static CBServer srv; srv.register_user(username); }
void cb_send_message(const char* to, const char* message) { static CBServer srv; srv.send_message(to, message); }
