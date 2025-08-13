#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

class SocketSubscriber {
public:
    using BinaryCallback = std::function<void(const std::vector<uint8_t>&)>;

    SocketSubscriber(const std::string& host, int port);
    ~SocketSubscriber();

    void start(BinaryCallback cb);
    void stop();

private:
    void run();

    std::string host;
    int port;
    BinaryCallback binary_callback;
    std::thread worker;
    std::atomic<bool> running{false};
    int server_socket = -1;
    int client_socket = -1;
};
