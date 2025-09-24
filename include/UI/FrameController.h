#pragma once

#include "Core/DataManager.h"
#include "IO/SocketSubscriber.h"
#include <string>
#include <atomic>

class FrameController {
public:
    FrameController(const std::string& host, int port);
    FrameController(DataManager* sharedDataManager);  // 新增：使用共享DataManager的构造函数
    ~FrameController();

    void toggle();
    void clear();
    void update();
    void drawUI(const int x, int y, int height, int width,const std::string& name);
    void togglePlayback();

    DataManager* getDataManager() { return dataManager; }  // 获取数据管理器的方法

private:
    DataManager* dataManager;  // 改为指针，支持共享
    SocketSubscriber* subscriber;  // 改为指针，可能为空
    std::atomic<bool> running{false};
    bool show_frame_window = true;
    bool owns_data_manager = false;  // 标记是否拥有DataManager

    int selected_channel = 0;
    bool auto_scale = true;
    bool reset_zoom = false;
};