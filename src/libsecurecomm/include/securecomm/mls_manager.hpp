#pragma once

#include "envelope.hpp"
#include "crypto.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <cstdint>

namespace securecomm {

class MLSManager {
public:
    MLSManager();
    ~MLSManager();

    std::vector<uint8_t> create_group(const std::string& group_name);
    void add_member(const std::vector<uint8_t>& group_id, const std::string& member_id);
    void remove_member(const std::vector<uint8_t>& group_id, const std::string& member_id);

    Envelope encrypt_group_message(const std::vector<uint8_t>& group_id,
                                   const std::string& sender_id,
                                   const std::vector<uint8_t>& plaintext);

    std::optional<std::vector<uint8_t>> decrypt_group_message(const std::vector<uint8_t>& group_id,
                                                              const std::string& member_id,
                                                              const Envelope& env);

    std::vector<uint8_t> get_group_epoch_secret(const std::vector<uint8_t>& group_id) const;
    uint64_t get_group_epoch(const std::vector<uint8_t>& group_id) const;

private:
    struct Group {
        std::vector<uint8_t> id;
        uint64_t epoch = 0;
        std::vector<std::vector<uint8_t>> leaf_secrets;
        std::map<std::string, size_t> member_index;
        std::vector<uint8_t> epoch_secret;
    };

    std::unordered_map<std::string, std::vector<uint8_t>> members_keys_;
    std::unordered_map<std::string, std::vector<uint8_t>> device_ids_;
    std::map<std::vector<uint8_t>, Group> groups_;
    AEAD aead_;

    std::vector<uint8_t> derive_epoch_secret(const Group& g) const;
    std::vector<uint8_t> derive_epoch_key(const std::vector<uint8_t>& epoch_secret,
                                          const std::vector<uint8_t>& group_id,
                                          uint64_t epoch) const;
    std::vector<uint8_t> random_bytes(size_t n) const;
};

} 
