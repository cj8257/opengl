#include "UI/MainController.h"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

using json = nlohmann::json;

MainController::MainController(const std::string& endpoint) : subscriber(endpoint) {}

MainController::~MainController() {
    subscriber.stop();
    dataManager.setProcessingEnabled(false);
}

void MainController::toggle() {
    if (running) {
        subscriber.stop();
        dataManager.setProcessingEnabled(false);
        running = false;
    } else {
        // 使用二进制数据回调而不是JSON
        subscriber.start([this](const std::vector<uint8_t>& packet_data) {
            // 将二进制数据包传递给DataManager处理
            dataManager.addBinaryPacket(packet_data);
        });
        dataManager.setProcessingEnabled(true);
        running = true;
    }
}

void MainController::clear() {
    dataManager.clear();
}

// 新增：播放/暂停控制
void MainController::togglePlayback() {
    bool current_state = dataManager.isPlaying();
    dataManager.setPlayState(!current_state);
}

void MainController::update() {
    // 如果需要定期更新，可以在这里添加逻辑
}

void MainController::drawUI() {
    // 控制按钮区域（优化布局）
    if (ImGui::Button(running ? "Stop" : "Start", ImVec2(80, 30))) toggle();
    ImGui::SameLine();
    
    // 播放/暂停按钮 - 使用英文避免字体问题
    if (ImGui::Button(dataManager.isPlaying() ? "Pause" : "Play", ImVec2(80, 30))) {
        togglePlayback();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Clear", ImVec2(80, 30))) clear();
    ImGui::SameLine();
    ImGui::Text("Status: %s | %s", 
                running ? "Running" : "Stopped",
                dataManager.isPlaying() ? "Playing" : "Paused");
    
    auto channel_data = dataManager.getChannelDisplayData();
    auto time_values = dataManager.getTimeValues();
    
    if (channel_data.empty() || time_values.empty()) {
        ImGui::Text("Waiting for data...");
        return;
    }
    
    ImGui::Separator();
    
    // 显示控制面板
    static int display_channels = 8;
    static float plot_height = 400.0f;
    static bool auto_scale = true;
    
    // 控制面板布局
    ImGui::Columns(3, "Control Panel", false);
    ImGui::SliderInt("Display Channels", &display_channels, 1, std::min(128, static_cast<int>(channel_data.size())));
    ImGui::NextColumn();
    ImGui::SliderFloat("Plot Height", &plot_height, 200.0f, 800.0f);
    ImGui::NextColumn();
    ImGui::Checkbox("Auto Scale", &auto_scale);
    ImGui::Columns(1);
    
    // 使用ImPlot绘制图表
    if (ImPlot::BeginPlot("Multi-Channel Sensor Data (128 Channels @ 22.5kHz)", ImVec2(-1, plot_height))) {
        
        // 计算Y轴范围（仅计算显示的通道以提升性能）
        if (auto_scale) {
            float min_val = FLT_MAX, max_val = -FLT_MAX;
            for (int ch = 0; ch < display_channels && ch < static_cast<int>(channel_data.size()); ++ch) {
                if (!channel_data[ch].empty()) {
                    // 只计算最近的数据点以提升性能
                    size_t sample_start = channel_data[ch].size() > 500 ? channel_data[ch].size() - 500 : 0;
                    for (size_t i = sample_start; i < channel_data[ch].size(); ++i) {
                        min_val = std::min(min_val, channel_data[ch][i]);
                        max_val = std::max(max_val, channel_data[ch][i]);
                    }
                }
            }
            
            if (min_val != FLT_MAX && max_val != -FLT_MAX) {
                ImPlot::SetupAxisLimits(ImAxis_Y1, min_val, max_val, ImGuiCond_Always);
            }
        }
        
        // 设置X轴范围 - 实现滑动时间窗口
        if (!time_values.empty()) {
            ImPlot::SetupAxisLimits(ImAxis_X1, time_values.front(), time_values.back(), ImGuiCond_Always);
        }
        
        ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        
        // 绘制选定的通道（性能优化：只绘制请求的通道数量）
        for (int ch = 0; ch < display_channels && ch < static_cast<int>(channel_data.size()); ++ch) {
            if (!channel_data[ch].empty() && channel_data[ch].size() == time_values.size()) {
                char label[32];
                snprintf(label, sizeof(label), "Ch%d", ch);
                
                // 使用优化的颜色生成（基于HSV色彩空间）
                float hue = (ch * 360.0f) / display_channels;
                float saturation = 0.8f + 0.2f * ((ch % 5) / 4.0f);
                float value = 0.7f + 0.3f * ((ch % 3) / 2.0f);
                
                // HSV转RGB
                float c = value * saturation;
                float x = c * (1 - fabs(fmod(hue / 60.0f, 2) - 1));
                float m = value - c;
                
                float r, g, b;
                if (hue < 60) { r = c; g = x; b = 0; }
                else if (hue < 120) { r = x; g = c; b = 0; }
                else if (hue < 180) { r = 0; g = c; b = x; }
                else if (hue < 240) { r = 0; g = x; b = c; }
                else if (hue < 300) { r = x; g = 0; b = c; }
                else { r = c; g = 0; b = x; }
                
                ImVec4 color = ImVec4(r + m, g + m, b + m, 0.8f);
                ImPlot::SetNextLineStyle(color, 1.0f);
                
                // 使用数据抽样来提升性能（如果数据点太多）
                if (time_values.size() > 2000) {
                    // 抽样绘制以提升性能
                    std::vector<float> sampled_time, sampled_data;
                    size_t step = time_values.size() / 1000;  // 抽样到1000个点
                    for (size_t i = 0; i < time_values.size(); i += step) {
                        sampled_time.push_back(time_values[i]);
                        sampled_data.push_back(channel_data[ch][i]);
                    }
                    ImPlot::PlotLine(label, sampled_time.data(), sampled_data.data(), 
                                    static_cast<int>(sampled_time.size()));
                } else {
                    ImPlot::PlotLine(label, time_values.data(), channel_data[ch].data(), 
                                    static_cast<int>(time_values.size()));
                }
            }
        }
        
        ImPlot::EndPlot();
    }
    
    // 性能统计信息
    ImGui::Separator();
    ImGui::Text("Performance: %.1f FPS | Display %d/%zu channels | %zu data points | Sample Rate: 22.5kHz", 
                ImGui::GetIO().Framerate, 
                display_channels, 
                channel_data.size(),
                time_values.size());
}