#pragma once

#include "envelope.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace securecomm {

class Transport {
public:
    using OnMessageCb = std::function<void(const std::vector<uint8_t>&)>;

    virtual ~Transport() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const std::vector<uint8_t>& bytes) = 0;
    virtual void set_on_message(OnMessageCb cb) = 0;
};

using TransportPtr = std::shared_ptr<Transport>;

} // namespace securecomm
