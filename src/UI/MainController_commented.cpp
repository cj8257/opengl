// 包含MainController的头文件
#include "UI/MainController.h"
// 包含nlohmann/json库的头文件，用于处理JSON数据
#include <nlohmann/json.hpp>
// 包含ImGui库的头文件
#include <imgui.h>
// 包含ImPlot库的头文件
#include <implot.h>
// 包含算法库的头文件
#include <algorithm>
// 包含vector容器的头文件
#include <vector>
// 包含string类的头文件
#include <string>
// 包含C标准输入输出库的头文件
#include <cstdio>

// 使用nlohmann::json的别名json
using json = nlohmann::json;

// MainController类的构造函数，初始化ZeroMQ订阅者
MainController::MainController(const std::string& endpoint) : subscriber(endpoint) {}

// MainController类的析构函数
MainController::~MainController() {
    // 停止ZeroMQ订阅者
    subscriber.stop();
    // 禁用数据管理器的数据处理
    dataManager.setProcessingEnabled(false);
}

// 切换运行状态的方法
void MainController::toggle() {
    // 如果当前正在运行
    if (running) {
        // 停止ZeroMQ订阅者
        subscriber.stop();
        // 禁用数据管理器的数据处理
        dataManager.setProcessingEnabled(false);
        // 更新运行状态为false
        running = false;
    } else {
        // 如果当前未运行，则启动
        // 启动ZeroMQ订阅者，并设置二进制数据回调
        subscriber.start([this](const std::vector<uint8_t>& packet_data) {
            // 将接收到的二进制数据包传递给数据管理器进行处理
            dataManager.addBinaryPacket(packet_data);
        });
        // 启用数据管理器的数据处理
        dataManager.setProcessingEnabled(true);
        // 更新运行状态为true
        running = true;
    }
}

// 清空数据的方法
void MainController::clear() {
    // 调用数据管理器的clear方法清空所有数据
    dataManager.clear();
}

// 新增：切换播放/暂停状态的方法
void MainController::togglePlayback() {
    // 获取当前的播放状态
    bool current_state = dataManager.isPlaying();
    // 设置为相反的播放状态
    dataManager.setPlayState(!current_state);
}

// 更新方法（当前未使用）
void MainController::update() {
    // 如果需要定期更新，可以在这里添加逻辑
}

// 绘制UI界面的方法
void MainController::drawUI() {
    // 创建一个控制按钮区域
    if (ImGui::Button(running ? "Stop" : "Start", ImVec2(80, 30))) toggle();
    // 将下一个控件放在同一行
    ImGui::SameLine();
    
    // 创建播放/暂停按钮
    if (ImGui::Button(dataManager.isPlaying() ? "Pause" : "Play", ImVec2(80, 30))) {
        // 点击时切换播放状态
        togglePlayback();
    }
    // 将下一个控件放在同一行
    ImGui::SameLine();
    
    // 创建清空按钮
    if (ImGui::Button("Clear", ImVec2(80, 30))) clear();
    // 将下一个控件放在同一行
    ImGui::SameLine();
    // 显示状态文本
    ImGui::Text("Status: %s | %s", 
                running ? "Running" : "Stopped",
                dataManager.isPlaying() ? "Playing" : "Paused");
    
    // 从数据管理器获取通道数据和时间值
    auto channel_data = dataManager.getChannelDisplayData();
    auto time_values = dataManager.getTimeValues();
    
    // 如果没有数据，则显示等待信息
    if (channel_data.empty() || time_values.empty()) {
        ImGui::Text("Waiting for data...");
        return;
    }
    
    // 添加分隔线
    ImGui::Separator();
    
    // 定义显示控制面板的静态变量
    static int display_channels = 8;
    static float plot_height = 400.0f;
    static bool auto_scale = true;
    
    // 创建一个3列的控制面板布局
    ImGui::Columns(3, "Control Panel", false);
    // 创建一个滑块来控制显示的通道数
    ImGui::SliderInt("Display Channels", &display_channels, 1, std::min(128, static_cast<int>(channel_data.size())));
    // 切换到下一列
    ImGui::NextColumn();
    // 创建一个滑块来控制图表的高度
    ImGui::SliderFloat("Plot Height", &plot_height, 200.0f, 800.0f);
    // 切换到下一列
    ImGui::NextColumn();
    // 创建一个复选框来控制是否自动缩放Y轴
    ImGui::Checkbox("Auto Scale", &auto_scale);
    // 结束列布局
    ImGui::Columns(1);
    
    // 开始绘制ImPlot图表
    if (ImPlot::BeginPlot("Multi-Channel Sensor Data (128 Channels @ 22.5kHz)", ImVec2(-1, plot_height))) {
        
        // 如果启用了自动缩放
        if (auto_scale) {
            // 初始化最小和最大值为浮点数的极限值
            float min_val = FLT_MAX, max_val = -FLT_MAX;
            // 遍历需要显示的通道
            for (int ch = 0; ch < display_channels && ch < static_cast<int>(channel_data.size()); ++ch) {
                // 如果通道数据不为空
                if (!channel_data[ch].empty()) {
                    // 为了性能，只计算最近500个数据点的范围
                    size_t sample_start = channel_data[ch].size() > 500 ? channel_data[ch].size() - 500 : 0;
                    // 遍历样本以找到最小值和最大值
                    for (size_t i = sample_start; i < channel_data[ch].size(); ++i) {
                        min_val = std::min(min_val, channel_data[ch][i]);
                        max_val = std::max(max_val, channel_data[ch][i]);
                    }
                }
            }
            
            // 如果找到了有效的范围
            if (min_val != FLT_MAX && max_val != -FLT_MAX) {
                // 设置Y轴的显示范围
                ImPlot::SetupAxisLimits(ImAxis_Y1, min_val, max_val, ImGuiCond_Always);
            }
        }
        
        // 如果时间值不为空，则设置X轴的范围以实现滑动窗口
        if (!time_values.empty()) {
            ImPlot::SetupAxisLimits(ImAxis_X1, time_values.front(), time_values.back(), ImGuiCond_Always);
        }
        
        // 设置X轴和Y轴的标签
        ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        
        // 遍历并绘制选定的通道
        for (int ch = 0; ch < display_channels && ch < static_cast<int>(channel_data.size()); ++ch) {
            // 确保通道数据和时间数据有效且大小匹配
            if (!channel_data[ch].empty() && channel_data[ch].size() == time_values.size()) {
                // 创建通道标签
                char label[32];
                snprintf(label, sizeof(label), "Ch%d", ch);
                
                // 基于HSV色彩空间为每个通道生成独特的颜色
                float hue = (ch * 360.0f) / display_channels;
                float saturation = 0.8f + 0.2f * ((ch % 5) / 4.0f);
                float value = 0.7f + 0.3f * ((ch % 3) / 2.0f);
                
                // HSV到RGB的转换
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
                
                // 创建ImVec4颜色对象
                ImVec4 color = ImVec4(r + m, g + m, b + m, 0.8f);
                // 设置下一条线的颜色和样式
                ImPlot::SetNextLineStyle(color, 1.0f);
                
                // 如果数据点过多，则进行抽样以提高性能
                if (time_values.size() > 2000) {
                    // 创建用于存储抽样数据的向量
                    std::vector<float> sampled_time, sampled_data;
                    // 计算抽样步长，目标是1000个点
                    size_t step = time_values.size() / 1000;
                    // 进行抽样
                    for (size_t i = 0; i < time_values.size(); i += step) {
                        sampled_time.push_back(time_values[i]);
                        sampled_data.push_back(channel_data[ch][i]);
                    }
                    // 绘制抽样后的线图
                    ImPlot::PlotLine(label, sampled_time.data(), sampled_data.data(), 
                                    static_cast<int>(sampled_time.size()));
                } else {
                    // 如果数据点不多，则直接绘制完整的线图
                    ImPlot::PlotLine(label, time_values.data(), channel_data[ch].data(), 
                                    static_cast<int>(time_values.size()));
                }
            }
        }
        
        // 结束图表绘制
        ImPlot::EndPlot();
    }
    
    // 添加分隔线
    ImGui::Separator();
    // 显示性能统计信息
    ImGui::Text("Performance: %.1f FPS | Display %d/%zu channels | %zu data points | Sample Rate: 22.5kHz", 
                ImGui::GetIO().Framerate, 
                display_channels, 
                channel_data.size(),
                time_values.size());
}