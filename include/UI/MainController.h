#pragma once

#include "Core/DataManager.h"
#include "IO/SocketSubscriber.h" // 请确保这是您订阅器类的正确路径
#include <string>   // 1. 添加 <string> 头文件
#include <atomic>   // 2. 添加 <atomic> 头文件

class MainController {
public:
    MainController(const std::string& host, int port);
    ~MainController();

    void toggle();
    void clear();
    void update();
    void drawUI();
    void togglePlayback();

    // 获取数据流状态 - 用于UI显示
    bool isDataStreamActive() const;

private:
    DataManager dataManager;
    SocketSubscriber subscriber;
    std::atomic<bool> running{false}; // 3. 使用 std::atomic 保证线程安全
    bool show_main_window = true; // 
};