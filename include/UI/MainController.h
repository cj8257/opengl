
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

private:
    DataManager dataManager;
    ZeroMQSubscriber subscriber;
    bool running = false;
};
