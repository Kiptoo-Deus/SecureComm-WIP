#include "securecomm/transport.hpp"
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace securecomm {

class InMemoryTransport : public Transport {
public:
    InMemoryTransport() : running_(false) {}
    ~InMemoryTransport() override { stop(); }

    void start() override {
        running_ = true;
        worker_ = std::thread([this] {
            std::unique_lock<std::mutex> lk(mutex_);
            while (running_) {
                cond_.wait(lk, [this]{ return !queue_.empty() || !running_; });
                while (!queue_.empty()) {
                    auto msg = queue_.front(); queue_.pop();
                    lk.unlock();
                    if (on_message_) on_message_(msg);
                    lk.lock();
                }
            }
        });
    }

    void stop() override {
        running_ = false;
        cond_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    void send(const std::vector<uint8_t>& bytes) override {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            queue_.push(bytes);
        }
        cond_.notify_one();
    }

    void set_on_message(OnMessageCb cb) override { on_message_ = cb; }

private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::vector<uint8_t>> queue_;
    OnMessageCb on_message_;
    std::thread worker_;
    std::atomic<bool> running_;
};

} // namespace securecomm

// Factory for test/demo usage
extern "C" securecomm::Transport* create_inmemory_transport() {
    return new securecomm::InMemoryTransport();
}
