#pragma once
#include <string>
#include <functional>

class CBServer {
public:
    using MessageCallback = std::function<void(const std::string& from, const std::string& message)>;

    explicit CBServer(unsigned short port = 55000);
    ~CBServer();

    void init();
    void shutdown();

    void register_user(const std::string& username);
    void send_message(const std::string& to, const std::string& message);

    void set_message_callback(MessageCallback cb);

private:
    struct Impl;
    Impl* pImpl;
};
