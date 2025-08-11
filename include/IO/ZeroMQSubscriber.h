
#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <string>

class ZeroMQSubscriber {
public:
    using Callback = std::function<void(const std::string&)>;

    ZeroMQSubscriber(const std::string& endpoint);
    ~ZeroMQSubscriber();

    void start(Callback cb);
    void stop();

private:
    void run();
    std::string endpoint;
    Callback callback;
    std::thread worker;
    std::atomic<bool> running{false};
};
