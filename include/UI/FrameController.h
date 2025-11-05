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
    void drawUI(const int x, int y, int height, int width,const std::string& name, bool showData = true);
    void togglePlayback();

    // 数据接收控制方法
    void startReceiving();
    void stopReceiving();

    DataManager* getDataManager() { return dataManager; }  // 获取数据管理器的方法

private:
    DataManager* dataManager;  // 改为指针，支持共享
    SocketSubscriber* subscriber;  // 改为指针，可能为空
    std::atomic<bool> running{false};
    std::atomic<bool> processing_data{false};  // 控制是否处理接收到的数据
    bool show_frame_window = true;
    bool owns_data_manager = false;  // 标记是否拥有DataManager

    // 网络连接信息
    std::string host;
    int port;

    int selected_channel = 0;
    bool auto_scale = true;
    bool reset_zoom = false;

    // 数据流状态管理
    bool last_data_stream_state = false;  // 上次数据流状态
    bool auto_reset_on_reconnect = true;  // 重新连接时自动重置
};