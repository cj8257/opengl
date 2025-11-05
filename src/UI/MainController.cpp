// 引入MainController类的头文件，该文件定义了MainController类的接口。
#include "UI/MainController.h"
// 引入Dear ImGui库的头文件，用于创建图形用户界面。
#include <imgui.h>
// 引入ImPlot库的头文件，一个用于Dear ImGui的图表库。
#include <implot.h>
// 引入C++标准库中的<algorithm>头文件，提供了一系列算法函数，如std::min。
#include <algorithm>
// 引入C++标准库中的<vector>头文件，用于使用std::vector容器。
#include <vector>
// 引入C++标准库中的<string>头文件，用于使用std::string类。
#include <string>
// 引入C标准输入输出库，提供了如snprintf之类的函数。
#include <cstdio>
// 引入C++标准输入输出流库，常用于控制台调试输出（如std::cout）。
#include <iostream>

// MainController类的构造函数 - 初始化主控制器并启动数据订阅。
// @param host: 服务器的主机名或IP地址。
// @param port: 服务器的端口号。
MainController::MainController(const std::string& host, int port)
    : subscriber(host, port) // 成员初始化列表：直接构造SocketSubscriber成员对象，传入主机和端口。
{
    // 调用订阅器的start方法，并传入一个lambda函数作为数据接收回调。
    subscriber.start([this](const std::vector<uint8_t>& packet_data) {
        // 在回调函数中，将接收到的二进制数据包添加到dataManager中进行处理。
        dataManager.addBinaryPacket(packet_data);
    });
    // 设置运行状态标志为true。
    running = true;
}

// MainController类的析构函数 - 停止订阅器并清理资源。
MainController::~MainController() {
    // 调用订阅器的stop方法，以优雅地断开连接并停止接收数据。
    subscriber.stop();
}

// 清除所有数据的方法 - 清空数据管理器中的所有缓存数据。
void MainController::clear() {
    // 调用DataManager的clear方法来清空其内部存储的所有数据。
    dataManager.clear();
}

// 切换播放/暂停状态的方法 - 控制数据的播放状态。
void MainController::togglePlayback() {
    // 调用DataManager的setPlayState方法，将其设置为当前播放状态的相反值（播放->暂停, 暂停->播放）。
    dataManager.setPlayState(!dataManager.isPlaying());
}

// 获取数据流状态的方法 - 用于UI显示。
// @return: 如果数据流在最近一段时间内是活跃的，则返回true，否则返回false。
bool MainController::isDataStreamActive() const {
    // 调用DataManager的同名方法并返回其结果。
    return dataManager.isDataStreamActive();
}

// 更新方法（预留） - 目前为空，可用于未来的功能扩展。
void MainController::update() {
    // 预留的更新方法，可以在未来的开发中添加每帧需要执行的逻辑。
}

// 绘制用户界面的方法 - 包含了所有UI元素的创建和数据可视化逻辑。
void MainController::drawUI() {
    // 1. 设置下一个要创建的窗口的固定位置和大小，ImGuiCond_Always确保每次都强制设置。
    ImGui::SetNextWindowPos(ImVec2(670, 50), ImGuiCond_Always);      // 固定窗口位置在屏幕坐标(670, 50)。
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Always);    // 固定窗口大小为600x500像素。

    // 2. 开始创建一个ImGui窗口，并设置标志以禁止移动和调整大小。
    // ImGui::Begin返回false表示窗口被折叠或关闭。
    if (!ImGui::Begin("时域信号处理窗口", &show_main_window, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        // 如果用户关闭了这个窗口，Begin会返回false，此时必须调用End并提前返回以结束本帧的绘制。
        ImGui::End();
        return;
    }

    // --- 从这里开始，所有的UI控件都安全地放置在这个窗口内 ---
    // 4. 使用DataManager的线程安全回调函数来访问和显示数据。
    dataManager.accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {

        // --- 控制面板部分 ---
        // 使用static变量来在多次调用drawUI之间保持其值。
        static int display_channels = 8;    // 默认显示的通道数。
        static bool auto_scale = true;      // 默认开启Y轴自动缩放。

        // 数据流状态指示器。
        bool stream_active = dataManager.isDataStreamActive(); // 获取当前数据流是否活跃。
        bool is_playing = dataManager.isPlaying();             // 获取当前是否处于播放状态。
        ImGui::Text("数据流状态: ");                           // 显示文本标签。
        ImGui::SameLine();                                     // 使下一个UI元素与上一个在同一行。
        if (is_playing) {                                      // 如果是播放状态。
            if (stream_active) {                               // 并且数据流是活跃的。
                // 显示绿色的 "●实时接收"。
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "●实时接收");
            } else {                                           // 如果数据流不活跃。
                // 显示橙色的 "●等待数据"。
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "●等待数据");
            }
        } else {                                               // 如果是暂停状态。
            // 显示灰色的 "●已暂停"。
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "●已暂停");
        }

        // 创建一个滑块来控制要显示的通道数量。
        // 范围从1到128和实际可用通道数中的较小者。
        ImGui::SliderInt("绘制通道", &display_channels, 1, std::min(128, static_cast<int>(display_data.size())));
        ImGui::SameLine(); // 同行显示。
        // 创建一个复选框来控制Y轴是否自动缩放。
        ImGui::Checkbox("自动缩放 Y 轴", &auto_scale);
        
        // 在控制面板和图表之间添加一条水平分隔线。
        ImGui::Separator();

        // 5. 开始绘制ImPlot图表，ImVec2(-1, -1)表示让图表填满窗口的剩余可用空间。
        if (ImPlot::BeginPlot("时序图", ImVec2(-1, -1))) {

            // 6. [关键修复] 使用安全的 if-else 结构来处理数据存在和不存在两种情况。
            if (display_data.empty() || display_data[0].empty() || time_data.empty())
            {
                // 分支1: 如果数据为空，则不执行任何绘图操作。这可以防止因访问空容器而导致的崩溃。
                // 此时，一个空的图表框架会被正确绘制。
            }
            else
            {
                // 分支2: 如果数据存在，则执行所有正常的绘图逻辑。
                if (auto_scale) { // 如果开启了自动缩放。
                    // 初始化最小值为浮点数最大值，最大值为浮点数最小值，以便任何实际值都能替换它们。
                    float min_val = FLT_MAX, max_val = -FLT_MAX;
                    // 遍历所有需要显示的通道。
                    for (int ch = 0; ch < display_channels && ch < static_cast<int>(display_data.size()); ++ch) {
                        // 遍历该通道中的每一个数据点。
                        for (const float& val : display_data[ch]) {
                            // 更新全局的最小值和最大值。
                            if (val < min_val) min_val = val;
                            if (val > max_val) max_val = val;
                        }
                    }
                    // 如果找到了有效的范围（即min_val和max_val被更新过）。
                    if (min_val != FLT_MAX && max_val != -FLT_MAX) {
                        // 则设置Y轴的显示范围，ImGuiCond_Always确保每一帧都强制更新。
                        ImPlot::SetupAxisLimits(ImAxis_Y1, min_val, max_val, ImGuiCond_Always);
                    }
                }

                // 设置X轴的范围为时间数据的起始点到结束点。
                ImPlot::SetupAxisLimits(ImAxis_X1, time_data.front(), time_data.back(), ImGuiCond_Always);
                // 设置X轴的标签。
                ImPlot::SetupAxis(ImAxis_X1, "时间 (s)");
                // 设置Y轴的标签。
                ImPlot::SetupAxis(ImAxis_Y1, "值");

                // 使用static变量来存储每个通道的颜色，避免每帧重新计算。
                static std::vector<ImVec4> channel_colors;
                // 如果当前颜色数量少于需要显示的通道数。
                if (channel_colors.size() < static_cast<size_t>(display_channels)) {
                    // 调整颜色向量的大小。
                    channel_colors.resize(display_channels);
                    // 遍历每个通道以生成一个独特的颜色。
                    for (int ch = 0; ch < display_channels; ++ch) {
                        // 在HSV颜色空间的色相(Hue)上均匀分布，以获得鲜明且区分度高的颜色。
                        float hue = static_cast<float>(ch) / display_channels;
                        // 将HSV颜色转换为ImGui使用的RGB颜色。
                        ImGui::ColorConvertHSVtoRGB(hue, 0.85f, 0.95f, channel_colors[ch].x, channel_colors[ch].y, channel_colors[ch].z);
                        // 设置颜色的不透明度。
                        channel_colors[ch].w = 0.9f;
                    }
                }

                // 循环绘制每一个通道的折线图。
                for (int ch = 0; ch < display_channels && ch < static_cast<int>(display_data.size()); ++ch) {
                    // 确保当前通道有数据。
                    if (!display_data[ch].empty()) {
                        // 创建一个字符数组来存储图例标签，例如 "Ch 0", "Ch 1"。
                        char label[32];
                        // 使用snprintf安全地格式化标签字符串。
                        snprintf(label, sizeof(label), "Ch %d", ch);
                        // 为下一条要绘制的线设置颜色。
                        ImPlot::SetNextLineStyle(channel_colors[ch]);
                        // 计算要绘制的点数，取通道数据和时间数据长度的较小者，以防数组越界。
                        int plot_points = std::min(static_cast<int>(display_data[ch].size()), static_cast<int>(time_data.size()));
                        // 确保有至少一个点可以绘制。
                        if (plot_points > 0) {
                            // 调用ImPlot函数绘制折线图。
                            ImPlot::PlotLine(label, time_data.data(), display_data[ch].data(), plot_points);
                        }
                    }
                }
            }

            // 7. 确保ImPlot::EndPlot()在if (BeginPlot...)块的内部，并且在所有绘图命令之后被调用。
            ImPlot::EndPlot();
        }
    });
    // 结束当前ImGui窗口的绘制，与ImGui::Begin()配对。
    ImGui::End();
}