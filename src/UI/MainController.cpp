#include "UI/MainController.h"
#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>
#include <iostream>
// MainController类的构造函数 - 初始化主控制器并启动数据订阅
MainController::MainController(const std::string& host, int port)
    : subscriber(host, port) // 初始化列表：构造SocketSubscriber对象
{
    subscriber.start([this](const std::vector<uint8_t>& packet_data) {
        dataManager.addBinaryPacket(packet_data);
    });
    running = true;
}

// MainController类的析构函数 - 停止订阅器并清理资源
MainController::~MainController() {
    subscriber.stop();
}

// 清除所有数据的方法 - 清空数据管理器中的所有数据
void MainController::clear() {
    dataManager.clear();
}

// 切换播放/暂停状态的方法 - 控制数据的播放状态
void MainController::togglePlayback() {
    dataManager.setPlayState(!dataManager.isPlaying());
}

// 更新方法（预留） - 目前为空，可用于未来的功能扩展
void MainController::update() {
    // 预留的更新方法，可用于处理逻辑更新
}

// 绘制用户界面的方法 - 包含了所有修复的最终版本
void MainController::drawUI() {
    // 1. 设置窗口的固定位置和大小，禁止拖动
    ImGui::SetNextWindowPos(ImVec2(670, 50), ImGuiCond_Always);      // 固定位置在(670,50)
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Always);    // 固定大小

    // 2. 创建一个标准的ImGui窗口作为所有控件的"容器"，禁止移动和调整大小
    if (!ImGui::Begin("时域信号处理窗口", &show_main_window, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        // 如果用户关闭了这个窗口, Begin会返回false, 我们必须调用End并提前返回
        ImGui::End();
        return;
    }

    // --- 从这里开始，所有的UI控件都安全地放在这个窗口里 ---
    // 4. 使用回调安全地访问和显示数据
    dataManager.accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {

        // --- 控制面板部分 ---
        static int display_channels = 8;
        static bool auto_scale = true;
        
        ImGui::SliderInt("绘制通道", &display_channels, 1, std::min(128, static_cast<int>(display_data.size())));
        ImGui::SameLine();
        ImGui::Checkbox("自动缩放 Y 轴", &auto_scale);
        
        ImGui::Separator();

        // 5. 开始绘制ImPlot图表，让它填满窗口的剩余空间
        if (ImPlot::BeginPlot("时序图", ImVec2(-1, -1))) {

            // 6. [关键修复] 使用安全的 if-else 结构，而不是提前返回
            if (display_data.empty() || display_data[0].empty() || time_data.empty())
            {
                // 分支1: 数据为空时, 不做任何绘图操作, 也可以在这里显示提示信息
                // 因为没有提前 return, 所以 EndPlot() 会被正确调用
            }
            else
            {
                // 分支2: 数据存在, 执行所有绘图逻辑
                if (auto_scale) {
                    float min_val = FLT_MAX, max_val = -FLT_MAX;
                    for (int ch = 0; ch < display_channels && ch < static_cast<int>(display_data.size()); ++ch) {
                        for (const float& val : display_data[ch]) {
                            if (val < min_val) min_val = val;
                            if (val > max_val) max_val = val;
                        }
                    }
                    if (min_val != FLT_MAX && max_val != -FLT_MAX) {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, min_val, max_val, ImGuiCond_Always);
                    }
                }

                ImPlot::SetupAxisLimits(ImAxis_X1, time_data.front(), time_data.back(), ImGuiCond_Always);
                ImPlot::SetupAxis(ImAxis_X1, "时间 (s)");
                ImPlot::SetupAxis(ImAxis_Y1, "值");

                static std::vector<ImVec4> channel_colors;
                if (channel_colors.size() < static_cast<size_t>(display_channels)) {
                    channel_colors.resize(display_channels);
                    for (int ch = 0; ch < display_channels; ++ch) {
                        float hue = static_cast<float>(ch) / display_channels;
                        ImGui::ColorConvertHSVtoRGB(hue, 0.85f, 0.95f, channel_colors[ch].x, channel_colors[ch].y, channel_colors[ch].z);
                        channel_colors[ch].w = 0.9f;
                    }
                }

                for (int ch = 0; ch < display_channels && ch < static_cast<int>(display_data.size()); ++ch) {
                    if (!display_data[ch].empty()) {
                        char label[32];
                        snprintf(label, sizeof(label), "Ch %d", ch);
                        ImPlot::SetNextLineStyle(channel_colors[ch]);
                        int plot_points = std::min(static_cast<int>(display_data[ch].size()), static_cast<int>(time_data.size()));
                        if (plot_points > 0) {
                            ImPlot::PlotLine(label, time_data.data(), display_data[ch].data(), plot_points);
                        }
                    }
                }
            }

            // 7. 确保 ImPlot::EndPlot() 在 if (BeginPlot...) 内部的最后被调用
            ImPlot::EndPlot();
        }
    });
    ImGui::End();
}