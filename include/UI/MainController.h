
#pragma once
#include "Core/DataManager.h"
#include "IO/ZeroMQSubscriber.h"

class MainController {
public:
    MainController(const std::string& endpoint);
    ~MainController();

    void toggle();
    void clear();
    void update();
    void drawUI();
    
    // 新增：播放控制
    void togglePlayback();

private:
    DataManager dataManager;
    ZeroMQSubscriber subscriber;
    bool running = false;
};
