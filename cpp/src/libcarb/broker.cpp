#define LIBCARB_EXPORTS
#include "libcarb/carrierbridge.h"
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <array>

using namespace libcarb;

struct CBroker::Impl {
    asio::io_context io;
    asio::ip::udp::socket socket;
    std::thread io_thread;
    std::unordered_map<std::string, asio::ip::udp::endpoint> registry;
    CBroker::MessageCallback msg_cb;
    std::mutex mtx;
    unsigned short port;

    Impl(unsigned short requested_port)
        : socket(io), port(requested_port)
    {
        socket.open(asio::ip::udp::v4());
        socket.set_option(asio::socket_base::reuse_address(true));
        socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), requested_port));
        port = socket.local_endpoint().port();
        std::cout << "[Broker] UDP server running on port " << port << "\n";
    }
};

CBroker::CBroker(unsigned short port) : pImpl(new Impl(port)) {}
CBroker::~CBroker() { delete pImpl; }

void CBroker::init() {
    std::cout << "[Broker] async loop starting\n";
    auto buffer = std::make_shared<std::array<char, 2048>>();
    auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
    auto do_receive = std::make_shared<std::function<void()>>();

    *do_receive = [this, buffer, endpoint, do_receive]() {
        pImpl->socket.async_receive_from(
            asio::buffer(*buffer), *endpoint,
            [this, buffer, endpoint, do_receive](std::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    std::string msg(buffer->data(), bytes_recvd);

                    // REGISTER
                    if (msg.rfind("REGISTER ", 0) == 0) {
                        std::string user = msg.substr(9);
                        {
                            std::lock_guard<std::mutex> lock(pImpl->mtx);
                            pImpl->registry[user] = *endpoint;
                        }
                        std::cout << "[Broker] Registered user: " << user << "\n";
                    }

                    // MESSAGE relay
                    else if (msg.rfind("MESSAGE ", 0) == 0) {
                        auto sp = msg.find(' ', 8);
                        if (sp != std::string::npos) {
                            std::string to = msg.substr(8, sp - 8);
                            std::string text = msg.substr(sp + 1);

                            asio::ip::udp::endpoint ep;
                            {
                                std::lock_guard<std::mutex> lock(pImpl->mtx);
                                auto it = pImpl->registry.find(to);
                                if (it != pImpl->registry.end()) ep = it->second;
                            }

                            if (ep.port() != 0) {
                                std::string out = "MESSAGE " + to + " " + text;
                                pImpl->socket.send_to(asio::buffer(out), ep);
                            }
                        }
                    }

                    // CALL signaling
                    else if (msg.rfind("CALL ", 0) == 0) {
                        auto sp = msg.find(' ', 5);
                        if (sp != std::string::npos) {
                            std::string from = msg.substr(5, sp - 5);
                            std::string to = msg.substr(sp + 1);

                            asio::ip::udp::endpoint ep;
                            {
                                std::lock_guard<std::mutex> lock(pImpl->mtx);
                                auto it = pImpl->registry.find(to);
                                if (it != pImpl->registry.end()) ep = it->second;
                            }

                            if (ep.port() != 0) {
                                std::string out = "INCOMING_CALL " + from;
                                pImpl->socket.send_to(asio::buffer(out), ep);
                                std::cout << "[Broker] Forwarded CALL " << from << " -> " << to << "\n";
                            } else {
                                std::cout << "[Broker] CALL target not found: " << to << "\n";
                            }
                        }
                    }
                }

                (*do_receive)();
            }
        );
    };

    (*do_receive)();
    pImpl->io_thread = std::thread([this]() { pImpl->io.run(); });
}

void CBroker::shutdown() {
    pImpl->io.stop();
    if (pImpl->io_thread.joinable()) pImpl->io_thread.join();
    std::cout << "[Broker] UDP server shutdown\n";
}

void CBroker::set_message_callback(MessageCallback cb) {
    pImpl->msg_cb = cb;
}
