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

class LIBCARB_API CBServer {
public:
    using MessageCallback = std::function<void(const std::string& from, const std::string& message)>;

    explicit CBServer(unsigned short port = 55000);
    ~CBServer();
    void init();
    void shutdown();
    void register_user(const std::string& username);
    void send_message(const std::string& to, const std::string& message);

    // register callback to be called when a message is routed locally
    void set_message_callback(MessageCallback cb);

private:
    struct Impl;
    Impl* pImpl;
};

}

// ----------------------
// C-compatible minimal wrappers (for future cgo/Go use)
// ----------------------
extern "C" {
    typedef void (*libcarb_message_cb_t)(const char* from, const char* message);

    // opaque pointer to server object
    typedef void* libcarb_server_t;

    // create/destroy
    LIBCARB_API libcarb_server_t libcarb_server_create(unsigned short port);
    LIBCARB_API void libcarb_server_destroy(libcarb_server_t s);

    // lifecycle
    LIBCARB_API int libcarb_server_init(libcarb_server_t s);
    LIBCARB_API void libcarb_server_shutdown(libcarb_server_t s);

    // operations
    LIBCARB_API void libcarb_server_register(libcarb_server_t s, const char* username);
    LIBCARB_API void libcarb_server_send(libcarb_server_t s, const char* to, const char* message);

    // set C callback
    LIBCARB_API void libcarb_server_set_callback(libcarb_server_t s, libcarb_message_cb_t cb);
}
