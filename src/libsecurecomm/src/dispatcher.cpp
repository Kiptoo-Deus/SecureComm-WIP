#include "securecomm/dispatcher.hpp"
#include <sodium.h>
#include <stdexcept>
#include <cstring>
#include <chrono>

namespace securecomm {

Dispatcher::Dispatcher(TransportPtr transport)
    : transport_(transport) {
    if (sodium_init() < 0) throw std::runtime_error("sodium_init failed");
    transport_->set_on_message([this](const std::vector<uint8_t>& b){ on_raw_message(b); });
}

Dispatcher::~Dispatcher() {
    stop();
}

void Dispatcher::start() {
    transport_->start();
}

void Dispatcher::stop() {
    transport_->stop();
}

void Dispatcher::register_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    device_id_ = device_id;
}

void Dispatcher::create_session_with(const std::string& remote_device_id, const std::vector<uint8_t>& root_key) {
    std::lock_guard<std::mutex> lk(mutex_);
    SessionState& s = sessions_[remote_device_id];
    s.ratchet.initialize(root_key);
    // generate initial DH step to sync
    s.ratchet.ratchet_step(s.ratchet.dh_public_key()); // local-only initial
    s.initialized = true;
}

void Dispatcher::send_message_to_device(const std::string& remote_device_id, const std::vector<uint8_t>& plaintext) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(remote_device_id);
    if (it == sessions_.end() || !it->second.initialized) throw std::runtime_error("session not initialized");
    Envelope env = it->second.ratchet.encrypt_envelope(plaintext);
    env.sender_device_id = device_id_;
    auto bytes = serialize_envelope(env);
    transport_->send(bytes);
}

void Dispatcher::send_group_message(const std::vector<uint8_t>& group_id, const std::string& sender_id, const std::vector<uint8_t>& plaintext) {
    Envelope env = mls_.encrypt_group_message(group_id, sender_id, plaintext);
    env.sender_device_id = device_id_;
    auto bytes = serialize_envelope(env);
    transport_->send(bytes);
}

void Dispatcher::set_on_inbound(OnInboundMessage cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    on_inbound_ = cb;
}

void Dispatcher::on_raw_message(const std::vector<uint8_t>& bytes) {
    auto env_opt = deserialize_envelope(bytes);
    if (!env_opt.has_value()) return;
    Envelope env = env_opt.value();

    // Determine if group or direct
    if (!env.session_id.empty() && mls_.get_group_epoch(env.session_id) != 0) {
        // group
        auto pt = mls_.decrypt_group_message(env.session_id, device_id_, env);
        if (pt.has_value()) {
            if (on_inbound_) on_inbound_(env);
        }
        return;
    }

    // direct: find session by sender device id
    std::string sender = env.sender_device_id;
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(sender);
    if (it == sessions_.end()) {
        // create ephemeral session with fixed root (for demo use root from env.ciphertext if present)
        // For now, attempt to decrypt using current session if exists
        return;
    }
    auto pt = it->second.ratchet.decrypt_envelope(env);
    if (pt.has_value()) {
        if (on_inbound_) on_inbound_(env);
    }
}

std::vector<uint8_t> Dispatcher::serialize_envelope(const Envelope& env) {
    std::vector<uint8_t> out;
    // session id length + session id
    uint32_t sid_len = static_cast<uint32_t>(env.session_id.size());
    out.push_back((sid_len >> 24) & 0xFF);
    out.push_back((sid_len >> 16) & 0xFF);
    out.push_back((sid_len >> 8) & 0xFF);
    out.push_back((sid_len) & 0xFF);
    out.insert(out.end(), env.session_id.begin(), env.session_id.end());

    // message_index, previous_counter, timestamp
    uint32_t mi = env.message_index;
    out.push_back((mi >> 24) & 0xFF);
    out.push_back((mi >> 16) & 0xFF);
    out.push_back((mi >> 8) & 0xFF);
    out.push_back((mi) & 0xFF);

    uint32_t pc = env.previous_counter;
    out.push_back((pc >> 24) & 0xFF);
    out.push_back((pc >> 16) & 0xFF);
    out.push_back((pc >> 8) & 0xFF);
    out.push_back((pc) & 0xFF);

    uint64_t ts = env.timestamp;
    for (int i = 7; i >= 0; --i) out.push_back((ts >> (8*i)) & 0xFF);

    // sender_device_id length + bytes
    uint32_t idlen = static_cast<uint32_t>(env.sender_device_id.size());
    out.push_back((idlen >> 24) & 0xFF);
    out.push_back((idlen >> 16) & 0xFF);
    out.push_back((idlen >> 8) & 0xFF);
    out.push_back((idlen) & 0xFF);
    out.insert(out.end(), env.sender_device_id.begin(), env.sender_device_id.end());

    // aad len + aad, ct len + ct
    uint32_t aadlen = static_cast<uint32_t>(env.associated_data.size());
    out.push_back((aadlen >> 24) & 0xFF);
    out.push_back((aadlen >> 16) & 0xFF);
    out.push_back((aadlen >> 8) & 0xFF);
    out.push_back((aadlen) & 0xFF);
    out.insert(out.end(), env.associated_data.begin(), env.associated_data.end());

    uint32_t ctlen = static_cast<uint32_t>(env.ciphertext.size());
    out.push_back((ctlen >> 24) & 0xFF);
    out.push_back((ctlen >> 16) & 0xFF);
    out.push_back((ctlen >> 8) & 0xFF);
    out.push_back((ctlen) & 0xFF);
    out.insert(out.end(), env.ciphertext.begin(), env.ciphertext.end());

    return out;
}

std::optional<Envelope> Dispatcher::deserialize_envelope(const std::vector<uint8_t>& bytes) {
    size_t off = 0;
    if (bytes.size() < 4) return std::nullopt;
    auto read_u32 = [&](uint32_t& out)->bool {
        if (off + 4 > bytes.size()) return false;
        out = (uint32_t(bytes[off])<<24) | (uint32_t(bytes[off+1])<<16) | (uint32_t(bytes[off+2])<<8) | uint32_t(bytes[off+3]);
        off +=4;
        return true;
    };
    Envelope env;
    uint32_t sid_len;
    if (!read_u32(sid_len)) return std::nullopt;
    if (off + sid_len > bytes.size()) return std::nullopt;
    env.session_id = std::vector<uint8_t>(bytes.begin()+off, bytes.begin()+off+sid_len);
    off += sid_len;

    uint32_t mi; if (!read_u32(mi)) return std::nullopt; env.message_index = mi;
    uint32_t pc; if (!read_u32(pc)) return std::nullopt; env.previous_counter = pc;
    if (off + 8 > bytes.size()) return std::nullopt;
    uint64_t ts = 0;
    for (int i=0;i<8;i++) { ts = (ts<<8) | bytes[off++]; }
    env.timestamp = ts;

    uint32_t idlen; if (!read_u32(idlen)) return std::nullopt;
    if (off + idlen > bytes.size()) return std::nullopt;
    env.sender_device_id = std::string(bytes.begin()+off, bytes.begin()+off+idlen);
    off += idlen;

    uint32_t aadlen; if (!read_u32(aadlen)) return std::nullopt;
    if (off + aadlen > bytes.size()) return std::nullopt;
    env.associated_data = std::vector<uint8_t>(bytes.begin()+off, bytes.begin()+off+aadlen);
    off += aadlen;

    uint32_t ctlen; if (!read_u32(ctlen)) return std::nullopt;
    if (off + ctlen > bytes.size()) return std::nullopt;
    env.ciphertext = std::vector<uint8_t>(bytes.begin()+off, bytes.begin()+off+ctlen);
    off += ctlen;

    return env;
}

} // namespace securecomm
