#include "securecomm/transport.hpp"
#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <string>

namespace securecomm {

// Minimal WebSocket transport stub. To use for production, implement with a WebSocket
// client library (Boost.Beast, websocketpp, libcurl + websockets, or platform SDKs).
class WebSocketTransport : public Transport {
public:
    WebSocketTransport(const std::string& uri) : uri_(uri), running_(false) {}
    ~WebSocketTransport() override { stop(); }

    void start() override {
        running_ = true;
        // TODO: implement websocket connect loop using chosen websocket library
    }

    void stop() override {
        running_ = false;
        // TODO: close socket cleanly
    }

    void send(const std::vector<uint8_t>& bytes) override {
        // TODO: send bytes over websocket
    }

    void set_on_message(OnMessageCb cb) override { on_message_ = cb; }

private:
    std::string uri_;
    std::atomic<bool> running_;
    OnMessageCb on_message_;
};

} // namespace securecomm

extern "C" securecomm::Transport* create_websocket_transport(const char* uri) {
    return new securecomm::WebSocketTransport(std::string(uri));
}
