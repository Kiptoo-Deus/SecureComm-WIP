#pragma once

#include "transport.hpp"
#include "envelope.hpp"
#include "ratchet.hpp"
#include "mls_manager.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

namespace securecomm {

class Dispatcher {
public:
    using OnInboundMessage = std::function<void(const Envelope& env)>;

    Dispatcher(TransportPtr transport);
    ~Dispatcher();

    void start();
    void stop();

    void register_device(const std::string& device_id);
    void create_session_with(const std::string& remote_device_id, const std::vector<uint8_t>& root_key);
    void send_message_to_device(const std::string& remote_device_id, const std::vector<uint8_t>& plaintext);
    void send_group_message(const std::vector<uint8_t>& group_id, const std::string& sender_id, const std::vector<uint8_t>& plaintext);

    void set_on_inbound(OnInboundMessage cb);

private:
    void on_raw_message(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> serialize_envelope(const Envelope& env);
    std::optional<Envelope> deserialize_envelope(const std::vector<uint8_t>& bytes);

    TransportPtr transport_;
    std::string device_id_;
    std::mutex mutex_;

    struct SessionState {
        Ratchet ratchet;
        bool initialized = false;
    };

    std::unordered_map<std::string, SessionState> sessions_;
    MLSManager mls_;
    OnInboundMessage on_inbound_;
};

using DispatcherPtr = std::shared_ptr<Dispatcher>;

} // namespace securecomm
