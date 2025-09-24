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
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(height, width), ImGuiCond_Always);

    // 设置现代窗口样式
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.14f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.30f, 0.38f, 1.0f));

    // 2. 创建现代风格的窗口
    if (!ImGui::Begin(name.c_str(), NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse)) {
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::End();
        return;
    }

    // 使用回调安全地访问和显示数据
    dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {

        // 现代化的控制面板
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

        // 通道选择区域
        ImGui::Text("通道:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);

        std::string channel_id = "##channel_" + name;
        ImGui::InputInt(channel_id.c_str(), &selected_channel);

        ImGui::SameLine();

        // 现代化按钮样式
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.60f, 1.00f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.40f, 0.80f, 1.0f));
        if (ImGui::Button("确定")) {
            selected_channel = std::max(0, std::min(selected_channel, static_cast<int>(display_data.size()) - 1));
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // 自动缩放开关 - 现代化样式
        std::string scale_id = "自动缩放##" + name;
        ImGui::Checkbox(scale_id.c_str(), &auto_scale);

        ImGui::SameLine();

        // 重置按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.40f, 0.50f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.50f, 0.60f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.30f, 0.40f, 1.0f));
        if (ImGui::Button("重置")) {
            reset_zoom = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar(2);

        // 添加分隔线
        ImGui::Separator();

        // 计算剩余空间给图表使用
        float remaining_height = ImGui::GetContentRegionAvail().y;

        // 开始绘制现代化图表 - 使用剩余的全部高度
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.08f, 0.10f, 0.14f, 1.0f));
        if (ImPlot::BeginPlot("##Chart", ImVec2(-1, remaining_height),
            ImPlotFlags_NoTitle | ImPlotFlags_NoMenus)) {

            if (!display_data.empty() && !time_data.empty() &&
                selected_channel >= 0 && selected_channel < display_data.size() &&
                !display_data[selected_channel].empty()) {

                auto& channel_data = display_data[selected_channel];
                size_t data_size = std::min(channel_data.size(), time_data.size());

                if (data_size > 0) {
                    // 设置坐标轴 - 现代化样式
                    ImPlot::SetupAxis(ImAxis_X1, "时间");
                    ImPlot::SetupAxis(ImAxis_Y1, "角度");

                    // 计算数据范围
                    float min_time = *std::min_element(time_data.begin(), time_data.begin() + data_size);
                    float max_time = *std::max_element(time_data.begin(), time_data.begin() + data_size);
                    float min_data = *std::min_element(channel_data.begin(), channel_data.begin() + data_size);
                    float max_data = *std::max_element(channel_data.begin(), channel_data.begin() + data_size);

                    // 自动缩放设置
                    if (auto_scale || reset_zoom) {
                        if (min_time != max_time) {
                            float time_margin = (max_time - min_time) * 0.05f;
                            ImPlot::SetupAxisLimits(ImAxis_X1, min_time - time_margin, max_time + time_margin,
                                                   reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                        }
                        if (min_data != max_data) {
                            float data_margin = (max_data - min_data) * 0.1f;
                            ImPlot::SetupAxisLimits(ImAxis_Y1, min_data - data_margin, max_data + data_margin,
                                                   reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                        }
                        reset_zoom = false;
                    }

                    // 使用蓝色渐变线条 - 匹配设计
                    ImPlot::SetNextLineStyle(ImVec4(0.30f, 0.60f, 1.00f, 0.9f), 1.5f);
                    ImPlot::SetNextFillStyle(ImVec4(0.20f, 0.50f, 0.90f, 0.3f));

                    std::string legend_name = "Ch " + std::to_string(selected_channel);
                    ImPlot::PlotLine(legend_name.c_str(), time_data.data(), channel_data.data(), data_size);

                    // 可选：添加填充效果
                    ImPlot::PlotShaded(legend_name.c_str(), time_data.data(), channel_data.data(), data_size);
                }
            } else {
                // 数据为空或无效通道时的显示
                ImPlot::SetupAxis(ImAxis_X1, "时间");
                ImPlot::SetupAxis(ImAxis_Y1, "角度");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, 100, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);
            }

            ImPlot::EndPlot();
        }
        ImPlot::PopStyleColor(1);
    });

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();
}