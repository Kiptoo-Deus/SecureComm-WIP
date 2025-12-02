#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace securecomm {

// Unified Envelope structure
struct Envelope {
    // Header fields
    uint32_t version = 1;
    std::vector<uint8_t> session_id;
    uint32_t message_index = 0;
    uint32_t previous_counter = 0;
    uint64_t timestamp = 0;
    std::string sender_device_id;
    std::string receiver_device_id;
    
    // Payload fields
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> signature;
    std::vector<uint8_t> aad;
    
    // Serialization methods
    std::vector<uint8_t> serialize() const;
    static Envelope deserialize(const std::vector<uint8_t>& input);
    
private:
    static void push_u32(std::vector<uint8_t>& out, uint32_t v);
    static uint32_t read_u32(const std::vector<uint8_t>& in, size_t& offset);
};

} // namespace securecomm