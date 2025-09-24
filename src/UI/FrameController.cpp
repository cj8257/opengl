#include "UI/FrameController.h"
#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>
#include <iostream>
#include <limits>
// FrameController类的构造函数 - 初始化帧控制器并启动数据订阅
FrameController::FrameController(const std::string& host, int port)
    : dataManager(new DataManager()), subscriber(new SocketSubscriber(host, port)), owns_data_manager(true)
{
    dataManager->setChartType("putu");
    subscriber->start([this](const std::vector<uint8_t>& packet_data) {
        dataManager->addBinaryPacket(packet_data);
    });
    running = true;
}

// 新增构造函数：使用共享DataManager
FrameController::FrameController(DataManager* sharedDataManager)
    : dataManager(sharedDataManager), subscriber(nullptr), owns_data_manager(false)
{
    running = true;
}

// FrameController类的析构函数 - 停止订阅器并清理资源
FrameController::~FrameController() {
    if (subscriber) {
        subscriber->stop();
        delete subscriber;
    }
    if (owns_data_manager && dataManager) {
        delete dataManager;
    }
}

// 切换方法（预留） - 与MainController保持一致
void FrameController::toggle() {
    // 预留方法，与MainController保持一致
}

// 清除所有数据的方法 - 清空数据管理器中的所有数据
void FrameController::clear() {
    dataManager->clear();
}

// 切换播放/暂停状态的方法 - 控制数据的播放状态
void FrameController::togglePlayback() {
    dataManager->setPlayState(!dataManager->isPlaying());
}

// 更新方法（预留） - 目前为空，可用于未来的功能扩展
void FrameController::update() {
    // 预留的更新方法，可用于处理逻辑更新
}

// 绘制用户界面的方法 - 显示当前帧数据的快照图表
void FrameController::drawUI(const int x, int y, int height, int width,const std::string& name){
    // 1. 设置窗口的固定位置和大小，禁止拖动
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);      // 固定位置在(50,50)
    ImGui::SetNextWindowSize(ImVec2(height, width), ImGuiCond_Always);    // 固定大小

    // 2. 创建一个标准的ImGui窗口作为所有控件的"容器"，禁止移动和调整大小
    if (!ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize| ImGuiWindowFlags_NoCollapse )) {
        // 如果用户关闭了这个窗口, Begin会返回false, 我们必须调用End并提前返回
        ImGui::End();
        return;
    }

    // --- 从这里开始，所有的UI控件都安全地放在这个窗口里 ---
    // 4. 使用回调安全地访问和显示数据
    dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {


                // std::cout << "--- Printing display_data (range-based for) ---" << std::endl;
        // 安全检查：确保 display_data 不是空的
// if (!display_data.empty()) {
//     const auto& first_row = display_data[0]; // 创建一个引用，让代码更清晰

//     // 计算要打印的元素数量：取 10 和数组实际大小中较小的一个
//     size_t items_to_print = std::min(10, (int)first_row.size());

//     std::cout << "第一行数据的前 " << items_to_print << " 项: [ ";
//     for (size_t i = 0; i < items_to_print; ++i) {
//         std::cout << first_row[i] << " ";
//     }
//     std::cout << "]" << std::endl;
//     std::cout << "第二行数据的前 " << items_to_print << " 项: [ ";
//     for (size_t i = 0; i < items_to_print; ++i) {
//         std::cout << time_data[i] << " ";
//     }
//     std::cout << "]" << std::endl;

// } else {
//     std::cout << "display_data 是空的。" << std::endl;
// }
        // std::cout << "--------------------------------------------" << std::endl;
        // --- 控制面板部分 ---
        ImGui::Text("通道选择:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);

        // 为每个FrameController实例创建唯一的控件ID
        std::string channel_id = "##channel_" + name;
        std::string button_id = "确定##" + name;
        std::string scale_id = "自动缩放##" + name;
        std::string reset_id = "重置缩放##" + name;

        ImGui::InputInt(channel_id.c_str(), &selected_channel);
        ImGui::SameLine();
        if (ImGui::Button(button_id.c_str())) {
            // 确保通道号在有效范围内
            selected_channel = std::max(0, std::min(selected_channel, static_cast<int>(display_data.size()) - 1));
        }
        ImGui::SameLine();
        ImGui::Text("(范围: 0-%d)", static_cast<int>(display_data.size()) - 1);

        ImGui::Checkbox(scale_id.c_str(), &auto_scale);
        ImGui::SameLine();
        if (ImGui::Button(reset_id.c_str())) {
            reset_zoom = true;
        }

        ImGui::Separator();

        // 5. 开始绘制ImPlot图表，让它填满窗口的剩余空间
        if (ImPlot::BeginPlot("图表", ImVec2(-1, -1))) {

            // 6. [关键修复] 使用安全的 if-else 结构，而不是提前返回
            if (display_data.empty() || time_data.empty())
            {
                // 分支1: 数据为空时, 不做任何绘图操作
            }
            else
            {
                // 分支2: 数据存在, 执行所有绘图逻辑

                // 确保选中的通道在有效范围内
                if (selected_channel >= 0 && selected_channel < display_data.size() && !display_data[selected_channel].empty()) {
                    auto& channel_data = display_data[selected_channel];
                    size_t data_size = std::min(channel_data.size(), time_data.size());

                    if (data_size > 0) {
                        // 设置坐标轴标签
                        ImPlot::SetupAxis(ImAxis_X1, "数据点");
                        ImPlot::SetupAxis(ImAxis_Y1, "数据值");

                        // 计算数据范围用于自动缩放
                        float min_time = *std::min_element(time_data.begin(), time_data.begin() + data_size);
                        float max_time = *std::max_element(time_data.begin(), time_data.begin() + data_size);
                        float min_data = *std::min_element(channel_data.begin(), channel_data.begin() + data_size);
                        float max_data = *std::max_element(channel_data.begin(), channel_data.begin() + data_size);

                        // 自动缩放设置
                        if (auto_scale || reset_zoom) {
                            if (min_time != max_time) {
                                float time_margin = (max_time - min_time) * 0.05f;  // 5%边距
                                ImPlot::SetupAxisLimits(ImAxis_X1, min_time - time_margin, max_time + time_margin,
                                                       reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                            }
                            if (min_data != max_data) {
                                float data_margin = (max_data - min_data) * 0.1f;  // 10%边距
                                ImPlot::SetupAxisLimits(ImAxis_Y1, min_data - data_margin, max_data + data_margin,
                                                       reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                            }
                            reset_zoom = false;
                        }

                        // 绘制选中通道的数据
                        ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 2.0f);  // 绿色线条
                        std::string legend_name = "通道 " + std::to_string(selected_channel);
                        ImPlot::PlotLine(legend_name.c_str(),
                                       time_data.data(),
                                       channel_data.data(),
                                       data_size);

                        // // 显示统计信息
                        // if (ImPlot::IsPlotHovered()) {
                        //     float mean = 0.0f;
                        //     for (size_t i = 0; i < data_size; ++i) {
                        //         mean += channel_data[i];
                        //     }
                        //     mean /= data_size;

                        //     std::string info_text = "通道" + std::to_string(selected_channel) + " 均值: " + std::to_string(mean);
                        //     ImPlot::PlotText(info_text.c_str(),
                        //                    min_time + (max_time - min_time) * 0.05f,
                        //                    min_data + (max_data - min_data) * 0.95f);
                        // }
                    }
                } else {
                    // 如果选中通道无效，显示提示信息
                    ImPlot::SetupAxis(ImAxis_X1, "时间");
                    ImPlot::SetupAxis(ImAxis_Y1, "数据值");
                    ImPlot::PlotText("无效的通道或无数据", 0.5f, 0.5f);
                }
            }

            // 7. 确保 ImPlot::EndPlot() 在 if (BeginPlot...) 内部的最后被调用
            ImPlot::EndPlot();
        }
    });
    ImGui::End();
}