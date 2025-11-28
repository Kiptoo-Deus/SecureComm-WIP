#define LIBCARB_EXPORTS
#include "libcarb/carrierbridge.h"
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <array>

using namespace libcarb;

struct CClient::Impl {
    asio::io_context io;
    asio::ip::udp::socket socket;
    std::thread io_thread;
    CClient::MessageCallback msg_cb;
    std::mutex mtx;
    unsigned short listen_port;
    std::string broker_host;
    unsigned short broker_port;

    Impl(unsigned short requested_port, std::string brokerHost, unsigned short brokerPort)
        : socket(io), listen_port(requested_port), broker_host(std::move(brokerHost)), broker_port(brokerPort)
    {
        asio::ip::udp::endpoint ep(asio::ip::udp::v4(), requested_port);
        socket.open(asio::ip::udp::v4());
        socket.bind(ep);
        listen_port = socket.local_endpoint().port();
        std::cout << "[Client] bound to port " << listen_port << "\n";
    }
};

CClient::CClient(unsigned short listen_port, const std::string& broker_host, unsigned short broker_port)
    : pImpl(new Impl(listen_port, broker_host, broker_port)) {}
CClient::~CClient() { delete pImpl; }

void CClient::init() {
    auto buffer = std::make_shared<std::array<char, 2048>>();
    auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
    auto do_receive = std::make_shared<std::function<void()>>();

    *do_receive = [this, buffer, endpoint, do_receive]() {
        pImpl->socket.async_receive_from(
            asio::buffer(*buffer), *endpoint,
            [this, buffer, endpoint, do_receive](std::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    std::string msg(buffer->data(), bytes_recvd);

                    // MESSAGE relay
                    if (msg.rfind("MESSAGE ", 0) == 0) {
                        auto sp = msg.find(' ', 8);
                        if (sp != std::string::npos) {
                            std::string to = msg.substr(8, sp - 8);
                            std::string text = msg.substr(sp + 1);
                            if (pImpl->msg_cb) pImpl->msg_cb(to, text);
                        }
                    }
                    // CALL incoming
                    else if (msg.rfind("INCOMING_CALL ", 0) == 0) {
                        std::string from = msg.substr(14);
                        if (pImpl->msg_cb) pImpl->msg_cb(from, "INCOMING_CALL");
                    }
                    else if (msg.rfind("CALL_ACCEPTED ", 0) == 0) {
                        std::string peer = msg.substr(14);
                        if (pImpl->msg_cb) pImpl->msg_cb(peer, "CALL_ACCEPTED");
                    }
                }

                (*do_receive)();
            }
        );
    };

    (*do_receive)();
    pImpl->io_thread = std::thread([this]() { pImpl->io.run(); });
}

void CClient::shutdown() {
    pImpl->io.stop();
    if (pImpl->io_thread.joinable()) pImpl->io_thread.join();
}

void CClient::register_user(const std::string& username) {
    asio::ip::udp::endpoint ep(asio::ip::make_address(pImpl->broker_host), pImpl->broker_port);
    std::string msg = "REGISTER " + username;
    pImpl->socket.send_to(asio::buffer(msg), ep);
}

void CClient::send_message(const std::string& to, const std::string& message) {
    asio::ip::udp::endpoint ep(asio::ip::make_address(pImpl->broker_host), pImpl->broker_port);
    std::string msg = "MESSAGE " + to + " " + message;
    pImpl->socket.send_to(asio::buffer(msg), ep);
}

void CClient::call_user(const std::string& to) {
    asio::ip::udp::endpoint ep(asio::ip::make_address(pImpl->broker_host), pImpl->broker_port);
    std::string msg = "CALL " + to;
    pImpl->socket.send_to(asio::buffer(msg), ep);
}

void CClient::set_message_callback(MessageCallback cb) {
    pImpl->msg_cb = cb;
}
