#include "imgui.h"           // 引入Dear ImGui核心库的头文件，用于创建图形用户界面。
#include "implot.h"          // 引入ImPlot库的头文件，一个为ImGui设计的强大绘图库。
#include "implot_internal.h" // 引入ImPlot的内部头文件，用于访问一些高级或内部功能。
#include <vector>            // 引入C++标准库中的<vector>头文件，用于使用std::vector动态数组。
#include <cmath>             // 引入C++标准库中的<cmath>头文件，提供了数学函数，如abs()。
#include <iostream>          // 引入C++标准输入输出流库，用于控制台输出（如调试信息）。
#include <algorithm>         // 引入C++标准库中的<algorithm>头文件，提供了min/max等算法。
#include <ctime>             // 引入C++标准库中的<ctime>头文件，用于处理时间和日期。
#include <iomanip>           // 引入C++标准库中的<iomanip>头文件，用于格式化输入输出流（如setw, setfill）。
#include <sstream>           // 引入C++标准库中的<sstream>头文件，用于进行字符串流操作。
#include "Core/DataManager.h"      // 引入自定义的数据管理器类头文件。
#include "IO/SocketSubscriber.h" // 引入自定义的Socket数据订阅器类头文件。

// 定义常量：数据维度
constexpr int ANGLE_BINS = 128;  // 角度轴上的数据点数 (X轴)，代表频谱图的列数。
constexpr int TIME_SLICES = 256; // 时间轴上的数据点数 (Y轴)，代表频谱图的行数或时间片数量。

// 全局变量：用于管理Socket数据接收相关的组件。
static DataManager* g_dataManager = nullptr;      // 指向DataManager实例的全局指针，用于处理和缓存数据。
static SocketSubscriber* g_subscriber = nullptr;  // 指向SocketSubscriber实例的全局指针，用于接收网络数据。
static bool g_socket_initialized = false;         // 一个布尔标志，用于记录Socket连接是否已经初始化。

// 清理频谱图相关资源的函数。
void CleanupSpectrogramResources() {
    // 检查g_subscriber指针是否有效。
    if (g_subscriber) {
        // 停止订阅器，断开socket连接。
        g_subscriber->stop();
        // 释放为订阅器分配的内存。
        delete g_subscriber;
        // 将指针设为nullptr，防止悬挂指针。
        g_subscriber = nullptr;
    }
    // 检查g_dataManager指针是否有效。
    if (g_dataManager) {
        // 释放为数据管理器分配的内存。
        delete g_dataManager;
        // 将指针设为nullptr。
        g_dataManager = nullptr;
    }
    // 重置socket初始化标志。
    g_socket_initialized = false;
    // 在控制台输出清理完成的信息。
    std::cout << "Spectrogram resources cleaned up" << std::endl;
}

// 静态变量：用于在UI渲染循环中保持和管理频谱图的数据状态。
static std::vector<double> spectrogram_data; // 用于存储整个频谱图数据的_vector，大小为 TIME_SLICES * ANGLE_BINS。
static bool data_initialized = false;        // 一个布尔标志，用于记录频谱图数据是否已经初始化。
static float time_axis_offset = 0.0f;        // 时间轴的偏移量，通过递增这个值来实现图表的滚动效果。
static time_t start_time;                    // 记录程序或数据流开始的时间点，用作时间格式化的基准。

// 时间格式化函数：将一个表示总秒数的double值转换为 HH:MM:SS 格式的字符串。
std::string FormatTime(double time_value) {
    // 将浮点数秒数转换为整数秒数。
    int total_seconds = static_cast<int>(time_value); 
    // 从总秒数中计算小时部分。
    int hours = total_seconds / 3600;                 
    // 计算分钟部分。
    int minutes = (total_seconds % 3600) / 60;        
    // 计算秒数部分。
    int seconds = total_seconds % 60;                 

    // 创建一个字符串输出流。
    std::ostringstream oss;                          
    // 使用setfill('0')和setw(2)来确保小时、分钟和秒都是两位数，不足则前面补0。
    oss << std::setfill('0') << std::setw(2) << hours << ":"   
        << std::setfill('0') << std::setw(2) << minutes << ":" 
        << std::setfill('0') << std::setw(2) << seconds;      
    // 返回格式化后的字符串。
    return oss.str();
}

// ImPlot时间轴格式化回调函数，用于自定义Y轴刻度的显示方式。
// @param value: ImPlot传入的当前刻度值。
// @param buff: ImPlot提供的用于存储格式化后字符串的缓冲区。
// @param size: 缓冲区的最大大小。
// @param user_data: 用户自定义数据指针（此处未使用）。
// @return: 写入缓冲区的字符数。
int TimeFormatterCallback(double value, char* buff, int size, void* user_data) {
    // 调用我们自定义的FormatTime函数来格式化时间值。
    std::string time_str = FormatTime(value);          
    // 使用snprintf安全地将格式化后的字符串复制到ImPlot提供的缓冲区中。
    int len = snprintf(buff, size, "%s", time_str.c_str()); 
    // 返回实际写入的字符数。
    return len;                                           
}

// 函数前向声明，告知编译器这些函数将在后面定义。
void InitializeSocketConnection(const std::string& host, int port); // 声明初始化Socket连接的函数。
void GenerateNewTimeSliceFromData(int time_slice);                  // 声明从接收的数据生成新时间片的函数。

// 初始化Socket连接的函数定义。
void InitializeSocketConnection(const std::string& host, int port) {
    // 检查是否已经初始化过，防止重复初始化。
    if (g_socket_initialized) {
        // 如果已初始化，则直接返回。
        return; 
    }

    // 在控制台输出正在初始化连接的信息。
    std::cout << "Initializing socket connection to " << host << ":" << port << std::endl;

    // 创建一个新的DataManager实例。
    g_dataManager = new DataManager();
    // 设置数据管理器的图表类型为"spectrogram"。
    g_dataManager->setChartType("spectrogram");

    // 创建一个新的SocketSubscriber实例，并传入主机和端口。
    g_subscriber = new SocketSubscriber(host, port);
    // 启动订阅器，并提供一个lambda表达式作为回调函数，用于处理接收到的数据。
    g_subscriber->start([](const std::vector<uint8_t>& packet_data) {
        // 检查数据管理器是否存在。
        if (g_dataManager) {
            // 如果存在，将接收到的二进制数据包添加到数据管理器中。
            g_dataManager->addBinaryPacket(packet_data);
        }
    });

    // 将初始化标志设置为true。
    g_socket_initialized = true;
    // 在控制台输出初始化成功的信息。
    std::cout << "Socket connection initialized successfully" << std::endl;
}

// 初始化频谱图数据的函数。
void InitializeSpectrogramData() {
    // 检查数据是否已经初始化过，防止重复操作。
    if (data_initialized) {
        return;
    }

    // 获取当前时间作为开始时间。
    start_time = time(nullptr);
    // 在控制台输出提示信息。
    std::cout << "Waiting for socket data to initialize spectrogram..." << std::endl;

    // 调整spectrogram_data向量的大小以容纳所有数据点，但不进行初始化填充。
    spectrogram_data.resize(TIME_SLICES * ANGLE_BINS);

    // 将数据初始化标志设置为true。
    data_initialized = true;
    // 在控制台输出准备接收数据的提示信息。
    std::cout << "Ready to receive socket data..." << std::endl;
}

// 从Socket接收的数据生成一个新的时间片数据的函数。
// @param time_slice: 要生成数据的时间片索引（通常是最新的一个）。
// @param showData: 控制是否执行数据处理的标志。
void GenerateNewTimeSliceFromData(int time_slice, bool showData = true) {
    // 检查数据管理器和socket连接是否存在，以及是否允许显示数据。
    if (!g_dataManager || !g_socket_initialized || !showData) {
        // 如果任一条件不满足，则直接返回，不处理任何数据。
        return; 
    }

    // 使用DataManager的线程安全回调函数来访问显示数据。
    g_dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {
        // 检查从Socket接收的数据是否为空。
        if (display_data.empty()) {
            // 如果为空，则直接返回，不更新显示。
            return; 
        }

        // 遍历所有角度点（频谱图的列）。
        for (int a = 0; a < ANGLE_BINS && a < display_data.size(); ++a) {
            // 初始化当前点的振幅为0.0。
            double amplitude = 0.0;

            // 获取对应角度的通道数据。
            const auto& channel_data = display_data[a];
            // 检查该通道是否有数据。
            if (!channel_data.empty()) {
                // 为了平滑数据，取最新的几个数据点的平均值。
                // 计算要平均的点数，最多取10个点或通道中所有点（如果不足10个）。
                int points_to_avg = std::min(static_cast<int>(channel_data.size()), 10);
                // 初始化总和为0.0。
                double sum = 0.0;
                // 累加最新的`points_to_avg`个数据点的值。
                for (int i = 0; i < points_to_avg; ++i) {
                    sum += channel_data[channel_data.size() - 1 - i];
                }
                // 计算平均值作为振幅。
                amplitude = sum / points_to_avg;

                // 将数据值映射到更适合颜色映射的显示范围。
                // 取绝对值并乘以一个缩放因子，以增强视觉效果。
                amplitude = std::abs(amplitude) * 4.0;  

                // 将计算出的振幅存储到一维向量spectrogram_data的正确位置。
                // (time_slice * ANGLE_BINS + a) 将二维索引转换为一维索引。
                spectrogram_data[time_slice * ANGLE_BINS + a] = amplitude;
            }
        }
    });
}

// 更新频谱数据以实现滚动效果。
// @param delta_time: 上一帧到当前帧的时间差。
// @param showData: 控制是否执行更新的标志。
void UpdateSpectrogramData(float delta_time, bool showData = true) {
    // 严格检查Socket是否初始化、数据管理器是否存在以及是否允许显示数据。
    if (!g_socket_initialized || !g_dataManager || !showData) {
        // 如果任一条件不满足，则完全停止更新。
        return; 
    }

    // 检查DataManager中是否实际接收到了Socket数据。
    bool has_socket_data = false;
    // 使用线程安全的回调函数来检查数据。
    g_dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {
        // 如果外层vector不为空，则认为有数据。
        has_socket_data = !display_data.empty();
    });

    // 如果没有接收到任何Socket数据，则不进行滚动更新。
    if (!has_socket_data) {
        return; 
    }

    // 使用一个累加器来实现基于时间的滚动，而不是基于帧的滚动。
    static float accumulator = 0.0f;
    // 累加时间差，乘以一个速度因子来控制滚动速度。
    accumulator += delta_time * 10.0f; 

    // 当累加器达到或超过1.0时，执行一次滚动操作。
    if (accumulator >= 1.0f) {
        // 从累加器中减去1.0，保留余数部分。
        accumulator -= 1.0f;
        // 增加时间轴的偏移量，使Y轴刻度向上移动。
        time_axis_offset += 1.0f;

        // 将所有时间片的数据向上移动一个位置。
        // t 从0到倒数第二行。
        for (int t = 0; t < TIME_SLICES - 1; ++t) {
            // a 从0到最后一列。
            for (int a = 0; a < ANGLE_BINS; ++a) {
                // 将下一行的数据复制到当前行。
                spectrogram_data[t * ANGLE_BINS + a] = spectrogram_data[(t + 1) * ANGLE_BINS + a];
            }
        }

        // 在最底部（索引为TIME_SLICES - 1）生成新的时间片数据。
        GenerateNewTimeSliceFromData(TIME_SLICES - 1, showData);
    }
}

// 主窗口渲染函数，负责显示整个频谱图界面。
// @param p_open: 指向布尔值的指针，用于控制窗口的开关状态。
// @param host, port: Socket连接的主机和端口。
// @param showData: 控制是否显示和更新数据的总开关。
void ShowSpectrogramWindow(bool* p_open, const std::string& host, int port, bool showData) {
    // 如果Socket连接尚未初始化，则进行初始化。
    InitializeSocketConnection(host, port);

    // 确保频谱图数据结构已初始化。
    InitializeSpectrogramData();

    // 在本帧开始时，首先检查是否有真实的Socket数据。
    bool has_socket_data = false;
    // 确保socket已初始化且数据管理器存在。
    if (g_socket_initialized && g_dataManager) {
        // 使用线程安全回调访问数据。
        g_dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {
            // 遍历所有通道（外层vector的每个元素）。
            for (const auto& channel : display_data) {
                // 只要找到任何一个通道的数据（内层vector）不是空的。
                if (!channel.empty()) {
                    // 就说明接收到了真实数据。
                    has_socket_data = true;
                    // 找到一个就足够了，立即跳出循环。
                    break; 
                }
            }
        });
    }

    // 只有在确认有真实数据的情况下，才更新滚动动画和时间。
    if (has_socket_data) {
        // 计算自上一帧以来的时间差（delta time）。
        static float last_time = 0.0f;
        // 获取ImGui的当前时间。
        float current_time = ImGui::GetTime();
        // 计算时间差。
        float delta_time = current_time - last_time;
        // 更新上一帧的时间。
        last_time = current_time;

        // 调用更新函数，传入时间差和显示开关。
        UpdateSpectrogramData(delta_time, showData);
    }

    // 设置下一个窗口的位置和大小，ImGuiCond_Always确保每次都强制设置。
    ImGui::SetNextWindowPos(ImVec2(20, 110), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(860, 940), ImGuiCond_Always);


    // 开始创建一个ImGui窗口。
    if (!ImGui::Begin("时方图 (Time-Frequency Plot)", NULL,
          ImGuiWindowFlags_NoMove |      // 禁止用户移动窗口。
          ImGuiWindowFlags_NoResize |    // 禁止用户调整窗口大小。
          ImGuiWindowFlags_NoCollapse)) { // 隐藏窗口的折叠按钮。
        // 如果窗口被折叠或关闭，则结束绘制并返回。
        ImGui::End();
        return;
    }

    // 检查Socket连接状态，如果未初始化或数据管理器不存在。
    if (!g_socket_initialized || !g_dataManager) {
        // 推入一个醒目的颜色（橙色）来显示提示文本。
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
        // 显示等待连接的文本。
        ImGui::Text("等待Socket连接...");
        // 显示目标主机和端口。
        ImGui::Text("主机: %s:%d", host.c_str(), port);
        // 弹出颜色设置，恢复默认文本颜色。
        ImGui::PopStyleColor(1);
        // 结束窗口绘制。
        ImGui::End();
        // 返回。
        return;
    }

    // 再次检查是否有实际的Socket数据（此处的has_socket_data是局部变量，需要重新赋值）。
    if (g_dataManager) {
        g_dataManager->accessDisplayData([&](const std::vector<std::vector<float>>& display_data, const std::vector<float>& time_data) {
            has_socket_data = !display_data.empty();
        });
    }

    // 如果已连接但没有数据。
    if (!has_socket_data) {
        // 推入橙色文本样式。
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
        // 显示等待数据的文本。
        ImGui::Text("Socket已连接，等待数据...");
        // 显示连接的主机和端口。
        ImGui::Text("主机: %s:%d", host.c_str(), port);
        // 弹出颜色样式。
        ImGui::PopStyleColor(1);
        // 结束窗口绘制。
        ImGui::End();
        // 返回。
        return;
    }

    // 如果有数据，则显示数据统计信息。
    if (!spectrogram_data.empty()) {
        // 找到当前显示数据中的最小值和最大值。
        double min_val = *std::min_element(spectrogram_data.begin(), spectrogram_data.end());
        double max_val = *std::max_element(spectrogram_data.begin(), spectrogram_data.end());

        // 推入一个柔和的文本颜色。
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.85f, 0.90f, 1.0f));
        // 显示数据范围。
        ImGui::Text("数据范围: %.3f to %.3f", min_val, max_val);
        // 显示图表顶部的当前时间。
        ImGui::Text("当前时间: %s", FormatTime(time_axis_offset + TIME_SLICES).c_str());

        // // 添加Socket连接状态指示器。
        // // 使下一个控件在同一行。
        // ImGui::SameLine();
        // // 显示"Socket: "文本。
        // ImGui::Text("● Socket: ");
        // // 使下一个控件在同一行。
        // ImGui::SameLine();
        // // 推入绿色，表示连接成功。
        // ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.80f, 0.30f, 1.0f));
        // // 显示"已连接"及主机和端口信息。
        // ImGui::Text("已连接 (%s:%d)", host.c_str(), port);
        // // 弹出绿色。
        // ImGui::PopStyleColor(1);
        // 弹出柔和的文本颜色。
        ImGui::PopStyleColor(1);
    }

    // 在信息区和图表之间添加一些垂直间距。
    ImGui::Spacing();

    // 推入一个ImPlot的色彩映射方案（Colormap）。Viridis是一种从蓝到绿到黄的常见科学可视化色谱。
    ImPlot::PushColormap(ImPlotColormap_Viridis); 

    // 获取窗口内剩余的可用高度，让图表填充这个空间。
    float remaining_height = ImGui::GetContentRegionAvail().y;

    // 开始绘制ImPlot图表区域。
    if (ImPlot::BeginPlot("##TimeFrequencyHeatmap", ImVec2(-1, remaining_height),
          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoTitle)) { // 使用标志禁用图例、右键菜单和标题栏。

        // 设置X轴和Y轴的标签。
        ImPlot::SetupAxes("角度", "时间");

        // 计算并设置Y轴（时间轴）的显示范围，以实现滚动效果。
        // 最小值是当前的偏移量。
        float time_min = time_axis_offset;
        // 最大值是偏移量加上时间片的总数。
        float time_max = time_axis_offset + TIME_SLICES;

        // 设置坐标轴的范围。注意Y轴是(max, min)，因为我们希望新数据从底部进入，时间向上增长。
        ImPlot::SetupAxesLimits(0, 360, time_max, time_min, ImGuiCond_Always);

        // 为Y轴设置自定义的刻度标签格式化函数。
        ImPlot::SetupAxisFormat(ImAxis_Y1, TimeFormatterCallback);

        // 只有在确认有Socket数据且频谱数据非空时，才绘制热力图。
        if (has_socket_data && !spectrogram_data.empty()) {
            // 调用ImPlot函数来绘制热力图。
            ImPlot::PlotHeatmap(
                "Socket Spectrogram",         // 图的ID。
                spectrogram_data.data(),      // 指向数据存储的指针。
                TIME_SLICES,                  // 数据的行数。
                ANGLE_BINS,                   // 数据的列数。
                0.0,                          // 颜色映射的最小值。
                2.0,                          // 颜色映射的最大值。
                NULL,                         // 单元格标签的格式（NULL表示不显示）。
                ImPlotPoint(0, time_max),     // 热力图在坐标系中的左上角点 (X=0, Y=滚动后的最大时间)。
                ImPlotPoint(360, time_min)    // 热力图在坐标系中的右下角点 (X=360, Y=滚动后的最小时间)。
            );
        }

        // 结束图表绘制。
        ImPlot::EndPlot();
    }
    // 弹出之前推入的色彩映射方案。
    ImPlot::PopColormap();
    // 结束ImGui窗口的绘制。
    ImGui::End();
}