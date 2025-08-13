#include "IO/SocketSubscriber.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

SocketSubscriber::SocketSubscriber(const std::string& host, int port) : host(host), port(port) {}

SocketSubscriber::~SocketSubscriber() {
    stop();
}

void SocketSubscriber::start(BinaryCallback cb) {
    if (running) return;
    binary_callback = cb;
    running = true;
    worker = std::thread(&SocketSubscriber::run, this);
}

void SocketSubscriber::stop() {
    if (!running) return;
    running = false;

    // Close sockets to unblock the worker thread
    if (client_socket != -1) {
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        client_socket = -1;
    }
    if (server_socket != -1) {
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        server_socket = -1;
    }

    if (worker.joinable()) {
        worker.join();
    }
}

void SocketSubscriber::run() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        close(server_socket);
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to " << host << ":" << port << " - " << strerror(errno) << std::endl;
        close(server_socket);
        return;
    }

    if (listen(server_socket, 1) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return;
    }

    std::cout << "SocketSubscriber started, listening on " << host << ":" << port << std::endl;

    const size_t CHANNEL_COUNT = 128;
    const size_t SAMPLES_PER_PACKET = 8;
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096 bytes

    while (running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            if (!running) break; // Shutdown requested
            std::cerr << "Failed to accept connection" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

        std::vector<uint8_t> buffer(PACKAGE_SIZE);
        while (running) {
            ssize_t total_bytes_read = 0;
            while (total_bytes_read < PACKAGE_SIZE) {
                ssize_t bytes_read = recv(client_socket, buffer.data() + total_bytes_read, PACKAGE_SIZE - total_bytes_read, 0);
                if (bytes_read > 0) {
                    total_bytes_read += bytes_read;
                } else if (bytes_read == 0) {
                    // Connection closed by client
                    std::cout << "Client disconnected." << std::endl;
                    break;
                } else {
                    // Error
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        std::cerr << "Recv error: " << strerror(errno) << std::endl;
                        break;
                    }
                    // No data available right now, sleep a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            if (total_bytes_read == PACKAGE_SIZE) {
                if (binary_callback) {
                    binary_callback(buffer);
                }
            } else {
                // Connection was closed or an error occurred
                break;
            }
        }

        if (client_socket != -1) {
            close(client_socket);
            client_socket = -1;
        }
    }

    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    std::cout << "SocketSubscriber stopped" << std::endl;
}
