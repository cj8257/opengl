
#pragma once
#include "Core/DataManager.h"
#include "IO/SocketSubscriber.h"

class MainController {
public:
    MainController(const std::string& host, int port);
    ~MainController();

    void toggle();
    void clear();
    void update();
    void drawUI();
    
    // 新增：播放控制
    void togglePlayback();

private:
    DataManager dataManager;
    SocketSubscriber subscriber;
    bool running = false;
};
