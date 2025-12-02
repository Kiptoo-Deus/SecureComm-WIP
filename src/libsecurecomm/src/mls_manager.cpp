#include "securecomm/mls_manager.hpp"
#include <sodium.h>
#include <stdexcept>
#include <chrono>
#include <algorithm>

namespace securecomm {

MLSManager::MLSManager() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium init");
}

MLSManager::~MLSManager() = default;

std::vector<uint8_t> MLSManager::random_bytes(size_t n) const {
    std::vector<uint8_t> out(n);
    randombytes_buf(out.data(), out.size());
    return out;
}

std::vector<uint8_t> MLSManager::create_group(const std::string& group_name) {
    std::vector<uint8_t> gid(16);
    randombytes_buf(gid.data(), gid.size());
    Group g;
    g.id = gid;
    g.epoch = 1;
    g.leaf_secrets.clear();
    g.epoch_secret = derive_epoch_secret(g);
    groups_.emplace(gid, std::move(g));
    return gid;
}

void MLSManager::add_member(const std::vector<uint8_t>& group_id, const std::string& member_id) {
    auto it = groups_.find(group_id);
    if (it == groups_.end()) throw std::runtime_error("group not found");
    Group& g = it->second;
    std::vector<uint8_t> leaf = random_bytes(32);
    g.leaf_secrets.push_back(leaf);
    size_t idx = g.leaf_secrets.size() - 1;
    g.member_index.emplace(member_id, idx);
    g.epoch++;
    g.epoch_secret = derive_epoch_secret(g);
}

void MLSManager::remove_member(const std::vector<uint8_t>& group_id, const std::string& member_id) {
    auto it = groups_.find(group_id);
    if (it == groups_.end()) throw std::runtime_error("group not found");
    Group& g = it->second;
    auto mit = g.member_index.find(member_id);
    if (mit == g.member_index.end()) return;
    size_t idx = mit->second;
    g.member_index.erase(mit);
    g.leaf_secrets.erase(g.leaf_secrets.begin() + idx);
    for (auto &p : g.member_index) {
        if (p.second > idx) p.second--;
    }
    g.epoch++;
    g.epoch_secret = derive_epoch_secret(g);
}

std::vector<uint8_t> MLSManager::derive_epoch_secret(const Group& g) const {
    size_t n = g.leaf_secrets.size();
    std::vector<uint8_t> concat;
    concat.reserve(n * 32);
    for (const auto& s : g.leaf_secrets) concat.insert(concat.end(), s.begin(), s.end());
    if (concat.empty()) {
        std::vector<uint8_t> z(32, 0);
        return z;
    }
    std::vector<uint8_t> prk(32);
    crypto_generichash(prk.data(), prk.size(), concat.data(), concat.size(), nullptr, 0);
    return prk;
}

std::vector<uint8_t> MLSManager::derive_epoch_key(const std::vector<uint8_t>& epoch_secret,
                                                  const std::vector<uint8_t>& group_id,
                                                  uint64_t epoch) const {
    std::vector<uint8_t> info;
    info.insert(info.end(), group_id.begin(), group_id.end());
    info.push_back(static_cast<uint8_t>((epoch >> 56) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 48) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 40) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 32) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 24) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 16) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch >> 8) & 0xFF));
    info.push_back(static_cast<uint8_t>((epoch) & 0xFF));
    std::vector<uint8_t> key(32);
    crypto_generichash(key.data(), key.size(), info.data(), info.size(), epoch_secret.data(), epoch_secret.size());
    return key;
}

Envelope MLSManager::encrypt_group_message(const std::vector<uint8_t>& group_id,
                                           const std::string& sender_id,
                                           const std::vector<uint8_t>& plaintext) {
    auto git = groups_.find(group_id);
    if (git == groups_.end()) throw std::runtime_error("group not found");
    Group& g = git->second;
    std::vector<uint8_t> key = derive_epoch_key(g.epoch_secret, g.id, g.epoch);
    aead_.set_key(key);
    std::vector<uint8_t> aad;
    aad.insert(aad.end(), g.id.begin(), g.id.end());
    aad.push_back(static_cast<uint8_t>((g.epoch >> 56) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 48) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 40) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 32) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 24) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 16) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch >> 8) & 0xFF));
    aad.push_back(static_cast<uint8_t>((g.epoch) & 0xFF));
    auto ct = aead_.encrypt(plaintext, aad);
    Envelope env;
    env.session_id = g.id;
    env.message_index = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() & 0xffffffff);
    env.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count());
    env.sender_device_id = sender_id;
    env.associated_data = aad;
    env.ciphertext = ct;
    return env;
}

std::optional<std::vector<uint8_t>> MLSManager::decrypt_group_message(const std::vector<uint8_t>& group_id,
                                                                      const std::string& member_id,
                                                                      const Envelope& env) {
    auto git = groups_.find(group_id);
    if (git == groups_.end()) return std::nullopt;
    const Group& g = git->second;
    if (env.session_id != g.id) return std::nullopt;
    uint64_t epoch = g.epoch;
    std::vector<uint8_t> key = derive_epoch_key(g.epoch_secret, g.id, epoch);
    aead_.set_key(key);
    auto pt = aead_.decrypt(env.ciphertext, env.associated_data);
    return pt;
}

std::vector<uint8_t> MLSManager::get_group_epoch_secret(const std::vector<uint8_t>& group_id) const {
    auto it = groups_.find(group_id);
    if (it == groups_.end()) return {};
    return it->second.epoch_secret;
}

uint64_t MLSManager::get_group_epoch(const std::vector<uint8_t>& group_id) const {
    auto it = groups_.find(group_id);
    if (it == groups_.end()) return 0;
    return it->second.epoch;
}

} // namespace securecomm
