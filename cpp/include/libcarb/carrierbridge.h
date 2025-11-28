#pragma once
#include <string>
#include <functional>


#ifdef _WIN32
#ifdef LIBCARB_EXPORTS
#define LIBCARB_API __declspec(dllexport)
#else
#define LIBCARB_API __declspec(dllimport)
#endif
#else
#define LIBCARB_API
#endif


namespace libcarb {


class LIBCARB_API CBroker {
public:
using MessageCallback = std::function<void(const std::string& to, const std::string& message)>;
explicit CBroker(unsigned short port = 55000);
~CBroker();
void init();
void shutdown();
void set_message_callback(MessageCallback cb);
private:
struct Impl;
Impl* pImpl;
};


class LIBCARB_API CClient {
public:
using MessageCallback = std::function<void(const std::string& from, const std::string& message)>;
explicit CClient(unsigned short listen_port = 0, const std::string& broker_host = "127.0.0.1", unsigned short broker_port = 55000);
~CClient();
void init();
void shutdown();


// user-level
void register_user(const std::string& username);
void send_message(const std::string& to, const std::string& message);
void call_user(const std::string& to);


void set_message_callback(MessageCallback cb);
private:
struct Impl;
Impl* pImpl;
};


} // namespace libcarb