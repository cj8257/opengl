// 引入FrameController类的头文件，该文件定义了FrameController类的接口。
#include "UI/FrameController.h"
// 引入Dear ImGui库的头文件，用于创建图形用户界面。
#include <imgui.h>
// 引入ImPlot库的头文件，一个用于Dear ImGui的图表库。
#include <implot.h>
// 引入C++标准库中的<algorithm>头文件，提供了一系列算法函数，如std::min和std::max。
#include <algorithm>
// 引入C++标准库中的<vector>头文件，用于使用std::vector容器。
#include <vector>
// 引入C++标准库中的<string>头文件，用于使用std::string类。
#include <string>
// 引入C标准输入输出库，虽然在这个文件中可能没有直接使用，但通常为了兼容性或某些底层操作而包含。
#include <cstdio>
// 引入C++标准输入输出流库，常用于控制台调试输出（如std::cout）。
#include <iostream>
// 引入C++标准库中的<limits>头文件，用于查询数值类型的属性（例如最大值、最小值）。
#include <limits>

// FrameController类的构造函数 - 初始化帧控制器并启动socket连接但不处理数据
// @param host: 服务器的主机名或IP地址。
// @param port: 服务器的端口号。
FrameController::FrameController(const std::string& host, int port)
    // 成员初始化列表：在构造函数体执行前初始化成员变量。
    : dataManager(new DataManager()),             // 创建一个新的DataManager实例，并将其指针赋给dataManager成员。
      subscriber(new SocketSubscriber(host, port)), // 创建一个新的SocketSubscriber实例，并将其指针赋给subscriber成员。
      owns_data_manager(true),                    // 设置标志位owns_data_manager为true，表示此对象拥有并负责释放dataManager。
      host(host),                                 // 使用传入的host参数初始化host成员变量。
      port(port)                                  // 使用传入的port参数初始化port成员变量。
{
    // 调用DataManager的方法，设置图表类型为"putu"。
    dataManager->setChartType("putu");
    // 调用DataManager的方法，启用帧模式，这样在数据流断开时图表内容不会被清空。
    dataManager->setFrameMode(true); 

    // 启动socket订阅器开始接收数据，并提供一个lambda函数作为数据到达时的回调。
    subscriber->start([this](const std::vector<uint8_t>& packet_data) {
        // 检查processing_data标志位，决定是否处理接收到的数据。
        if (processing_data) {  // 只有在处理标志为true时才处理数据
            // 如果允许处理数据，则将接收到的二进制数据包添加到DataManager中。
            dataManager->addBinaryPacket(packet_data);
        }
    });

    // 设置运行状态为true。
    running = true;
    // 初始时将数据处理标志设置为false，需要显式调用startReceiving()方法才能开始处理数据。
    processing_data = false;  
}

// 新增构造函数：使用一个外部传入的、共享的DataManager实例。
// @param sharedDataManager: 一个指向已存在的DataManager实例的指针。
FrameController::FrameController(DataManager* sharedDataManager)
    // 成员初始化列表
    : dataManager(sharedDataManager), // 使用传入的共享DataManager指针初始化dataManager成员。
      subscriber(nullptr),            // subscriber设置为空指针，因为此实例不负责数据接收。
      owns_data_manager(false)      // 设置标志位为false，表示此对象不拥有DataManager，因此析构时不会释放它。
{
    // 设置运行状态为false，因为共享DataManager的实例不负责数据接收。
    running = false;  
    // 初始不处理数据。
    processing_data = false;  
}

// FrameController类的析构函数 - 停止订阅器并清理动态分配的资源。
FrameController::~FrameController() {
    // 检查subscriber指针是否有效（非空）。
    if (subscriber) {
        // 停止socket订阅器，断开连接。
        subscriber->stop();
        // 释放为subscriber动态分配的内存。
        delete subscriber;
    }
    // 检查本实例是否拥有DataManager并且DataManager指针是否有效。
    if (owns_data_manager && dataManager) {
        // 释放为dataManager动态分配的内存。
        delete dataManager;
    }
}

// 切换方法（预留） - 为了与MainController类保持接口一致性而定义，当前没有具体实现。
void FrameController::toggle() {
    // 预留方法，与MainController保持一致
}

// 清除所有数据的方法 - 清空数据管理器中的所有缓存数据。
void FrameController::clear() {
    // 调用DataManager的clear方法来清空数据。
    dataManager->clear();
}

// 切换播放/暂停状态的方法 - 控制数据的播放状态。
void FrameController::togglePlayback() {
    // 调用DataManager的setPlayState方法，将其设置为当前播放状态的相反状态（播放->暂停，暂停->播放）。
    dataManager->setPlayState(!dataManager->isPlaying());
}

// 更新方法 - 每帧调用一次，用于检测状态变化，如数据流重连。
void FrameController::update() {
    // 从DataManager获取当前的数据流活动状态。
    bool current_data_stream_state = dataManager->isDataStreamActive();

    // 检查数据流是否从“不活动”变为“活动”，并且“自动重置”选项是开启的。
    if (!last_data_stream_state && current_data_stream_state && auto_reset_on_reconnect) {
        // 如果条件满足，设置reset_zoom标志位为true，这将在下一次UI绘制时触发图表缩放重置。
        reset_zoom = true;  
    }

    // 更新上一次的数据流状态，用于下一帧的比较。
    last_data_stream_state = current_data_stream_state;
}

// 绘制用户界面的方法 - 使用ImGui和ImPlot显示当前帧数据的快照图表。
// @param x, y: 窗口左上角的屏幕坐标。
// @param height, width: 窗口的尺寸。
// @param name: 窗口的标题和唯一标识符。
// @param showData: 一个布尔值，控制是否实际绘制图表数据。
void FrameController::drawUI(const int x, int y, int height, int width,const std::string& name, bool showData){
    // 1. 设置下一个要创建的窗口的固定位置和大小，ImGuiCond_Always表示每次渲染都强制设置。
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(height, width), ImGuiCond_Always);

    // 开始设置现代化的窗口样式，通过压入样式变量到堆栈。
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f)); // 设置窗口内部边距。
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);                 // 设置窗口边角的圆角半径。
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.14f, 0.18f, 1.0f)); // 设置窗口背景颜色。
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.30f, 0.38f, 1.0f));     // 设置窗口边框颜色。

    // 2. 开始创建一个ImGui窗口。
    // ImGui::Begin返回false表示窗口被折叠或不可见，此时应提前结束绘制。
    if (!ImGui::Begin(name.c_str(), NULL,
          ImGuiWindowFlags_NoMove |      // 禁止用户通过拖动来移动窗口。
          ImGuiWindowFlags_NoResize |    // 禁止用户调整窗口大小。
          ImGuiWindowFlags_NoCollapse)) { // 隐藏窗口的折叠按钮。
        // 如果窗口不可见，弹出之前压入的样式设置，以保持堆栈平衡。
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        // 结束窗口绘制。
        ImGui::End();
        // 提前返回，不执行后续的UI绘制代码。
        return;
    }

    // 使用DataManager的accessDisplayData方法安全地访问数据。它接收一个lambda函数，在数据锁定的情况下执行。
    dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {

        // 推入新的样式变量，用于美化控制面板中的控件。
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f)); // 设置控件（如按钮、输入框）的内边距。
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f)); // 设置控件之间的间距。

        // --- 通道选择区域 ---
        // 显示“通道:”文本标签。
        ImGui::Text("通道:");
        // 使下一个控件与上一个控件在同一行显示。
        ImGui::SameLine();
        // 设置下一个控件的宽度。
        ImGui::SetNextItemWidth(120);

        // 创建一个唯一的ID，以避免在多个窗口中使用相同标签时发生冲突。"##"后的文本是ID，但不会显示为标签。
        std::string channel_id = "##channel_" + name;
        // 创建一个整型输入框，绑定到selected_channel变量。如果用户修改了值，InputInt返回true。
        bool value_changed = ImGui::InputInt(channel_id.c_str(), &selected_channel);

        // 如果输入框的值被修改了，则执行处理逻辑。
        if (value_changed) {
            // 将用户输入的数值限制在有效的通道索引范围内。
            if (!display_data.empty()) { // 检查是否有数据。
                // 使用std::max和std::min将selected_channel限制在 [0, display_data.size() - 1] 的范围内。
                selected_channel = std::max(0, std::min(selected_channel, static_cast<int>(display_data.size()) - 1));
            } else {
                // 如果没有数据，则将所选通道重置为0。
                selected_channel = 0; 
            }
        }

        // 使下一个控件在同一行显示。
        ImGui::SameLine();

        // **新增功能**：显示当前可用的通道范围。
        if (!display_data.empty()) {
            // 如果有数据，显示 "(0 - 最大通道号)"。
            ImGui::Text("(0 - %d)", static_cast<int>(display_data.size()) - 1);
        } else {
            // 如果没有数据，显示 "(N/A)"。
            ImGui::Text("(N/A)");
        }

        // 使下一个控件在同一行显示。
        ImGui::SameLine();

        // 创建一个唯一的ID用于“自动缩放”复选框。
        std::string scale_id = "自动缩放##" + name;
        // 创建一个复选框，标签为“自动缩放”，其状态与布尔变量auto_scale双向绑定。
        ImGui::Checkbox(scale_id.c_str(), &auto_scale);

        // 使下一个控件在同一行显示。
        ImGui::SameLine();

        // --- 重置按钮 ---
        // 推入三种不同状态下的按钮颜色，以自定义按钮外观。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.40f, 0.50f, 1.0f));        // 正常状态
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.50f, 0.60f, 1.0f)); // 悬停状态
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.30f, 0.40f, 1.0f));  // 按下状态
        // 创建一个标签为“重置”的按钮。如果按钮被点击，ImGui::Button返回true。
        if (ImGui::Button("重置")) {
            // 设置reset_zoom标志位，以在后续逻辑中触发图表缩放的重置。
            reset_zoom = true;
        }
        // 弹出刚刚为按钮设置的3种颜色，恢复之前的样式。
        ImGui::PopStyleColor(3);

        // 弹出之前为控件设置的样式变量（FramePadding, ItemSpacing）。
        ImGui::PopStyleVar(2);

        // 在控制面板和图表之间添加一条水平分隔线。
        ImGui::Separator();

        // 获取窗口中剩余的可用垂直空间，用于设置图表的高度。
        float remaining_height = ImGui::GetContentRegionAvail().y;

        // --- 图表绘制 ---
        // 推入图表区域的背景颜色。
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.08f, 0.10f, 0.14f, 1.0f));
        // 开始绘制一个ImPlot图表区域。
        // ImPlot::BeginPlot返回true表示图表可见且正在绘制。
        if (ImPlot::BeginPlot("##Chart", ImVec2(-1, remaining_height),
              ImPlotFlags_NoTitle | ImPlotFlags_NoMenus)) { // 使用标志禁用标题和右键菜单。

            // 检查是否有有效数据可供绘制，并且showData标志为true。
            if (!display_data.empty() && !time_data.empty() &&
                selected_channel >= 0 && selected_channel < display_data.size() &&
                !display_data[selected_channel].empty() && showData) { // 添加showData条件

                // 获取对当前选定通道数据的引用，以提高可读性。
                auto& channel_data = display_data[selected_channel];
                // 计算实际要绘制的数据点数量，取时间和数据向量中较小者的大小，防止越界。
                size_t data_size = std::min(channel_data.size(), time_data.size());

                // 确保有数据点可以绘制。
                if (data_size > 0) {
                    // 设置X轴和Y轴的标签。
                    ImPlot::SetupAxis(ImAxis_X1, "数据点");
                    ImPlot::SetupAxis(ImAxis_Y1, "数据值");

                    // 使用std::min_element和std::max_element找到数据和时间的范围。
                    float min_time = *std::min_element(time_data.begin(), time_data.begin() + data_size);
                    float max_time = *std::max_element(time_data.begin(), time_data.begin() + data_size);
                    float min_data = *std::min_element(channel_data.begin(), channel_data.begin() + data_size);
                    float max_data = *std::max_element(channel_data.begin(), channel_data.begin() + data_size);

                    // 如果启用了自动缩放或触发了重置，则调整坐标轴范围。
                    if (auto_scale || reset_zoom) {
                        // 检查时间范围是否有效，避免所有点时间相同时除以零。
                        if (min_time != max_time) {
                            // 计算5%的边距，使图表两端留有空白。
                            float time_margin = (max_time - min_time) * 0.05f;
                            // 设置X轴的显示范围。ImGuiCond_Always用于强制重置，ImGuiCond_Once用于初次自动缩放。
                            ImPlot::SetupAxisLimits(ImAxis_X1, min_time - time_margin, max_time + time_margin,
                                                  reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                        }
                        // 检查数据范围是否有效。
                        if (min_data != max_data) {
                            // 计算10%的边距。
                            float data_margin = (max_data - min_data) * 0.1f;
                            // 设置Y轴的显示范围。
                            ImPlot::SetupAxisLimits(ImAxis_Y1, min_data - data_margin, max_data + data_margin,
                                                  reset_zoom ? ImGuiCond_Always : ImGuiCond_Once);
                        }
                        // 在应用缩放后，重置标志位。
                        reset_zoom = false;
                    }

                    // 设置下一条要绘制的线条的样式（颜色和粗细）。
                    ImPlot::SetNextLineStyle(ImVec4(0.30f, 0.60f, 1.00f, 0.9f), 1.5f);
                    // 设置下一个要绘制的填充区域的样式（颜色）。
                    ImPlot::SetNextFillStyle(ImVec4(0.20f, 0.50f, 0.90f, 0.3f));

                    // 创建图例名称，例如 "Ch 0"。
                    std::string legend_name = "Ch " + std::to_string(selected_channel);
                    // 绘制折线图。
                    ImPlot::PlotLine(legend_name.c_str(), time_data.data(), channel_data.data(), data_size);

                    // 在折线图下方绘制一个填充（阴影）区域，以增强可视化效果。
                    ImPlot::PlotShaded(legend_name.c_str(), time_data.data(), channel_data.data(), data_size);
                }
            } else {
                // 如果没有有效数据或showData为false，则显示一个空的默认图表。
                ImPlot::SetupAxis(ImAxis_X1, "数据点");
                ImPlot::SetupAxis(ImAxis_Y1, "数据值");
                // 设置固定的默认坐标轴范围。
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, 100, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);
            }

            // 结束图表绘制。
            ImPlot::EndPlot();
        }
        // 弹出图表背景颜色，恢复之前的样式。
        ImPlot::PopStyleColor(1);
    });

    // 弹出窗口的颜色和样式变量，恢复之前的样式，并保持堆栈平衡。
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    // 结束当前ImGui窗口的绘制。
    ImGui::End();
}

// 开始数据处理的方法。
void FrameController::startReceiving() {
    // 将processing_data标志设置为true，这样socket回调函数就会开始处理接收到的数据包。
    processing_data = true;  
}

// 停止数据处理但保持socket连接的方法。
void FrameController::stopReceiving() {
    // 将processing_data标志设置为false，socket回调函数将忽略之后收到的数据包，但连接本身不中断。
    processing_data = false; 
}