#include "securecomm/envelope.hpp"
#include <stdexcept>

namespace securecomm {

void Envelope::push_u32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back((v >> 24) & 0xFF);
    out.push_back((v >> 16) & 0xFF);
    out.push_back((v >> 8) & 0xFF);
    out.push_back(v & 0xFF);
}

uint32_t Envelope::read_u32(const std::vector<uint8_t>& in, size_t& offset) {
    if (offset + 4 > in.size())
        throw std::runtime_error("Envelope: invalid length");
    uint32_t v =
        (in[offset] << 24) |
        (in[offset+1] << 16) |
        (in[offset+2] << 8) |
        (in[offset+3]);
    offset += 4;
    return v;
}

std::vector<uint8_t> Envelope::serialize() const {
    std::vector<uint8_t> out;

    push_u32(out, version);

    push_u32(out, sender_id.size());
    out.insert(out.end(), sender_id.begin(), sender_id.end());

    push_u32(out, receiver_id.size());
    out.insert(out.end(), receiver_id.begin(), receiver_id.end());

    push_u32(out, ciphertext.size());
    out.insert(out.end(), ciphertext.begin(), ciphertext.end());

    push_u32(out, signature.size());
    out.insert(out.end(), signature.begin(), signature.end());

    push_u32(out, aad.size());
    out.insert(out.end(), aad.begin(), aad.end());

    return out;
}

Envelope Envelope::deserialize(const std::vector<uint8_t>& input) {
    Envelope env;
    size_t offset = 0;

    env.version = read_u32(input, offset);

    uint32_t sl = read_u32(input, offset);
    env.sender_id.assign(input.begin() + offset, input.begin() + offset + sl);
    offset += sl;

    uint32_t rl = read_u32(input, offset);
    env.receiver_id.assign(input.begin() + offset, input.begin() + offset + rl);
    offset += rl;

    uint32_t cl = read_u32(input, offset);
    env.ciphertext.assign(input.begin() + offset, input.begin() + offset + cl);
    offset += cl;

    uint32_t sigl = read_u32(input, offset);
    env.signature.assign(input.begin() + offset, input.begin() + offset + sigl);
    offset += sigl;

    uint32_t aadl = read_u32(input, offset);
    env.aad.assign(input.begin() + offset, input.begin() + offset + aadl);
    offset += aadl;

    return env;
}

} // namespace securecomm
