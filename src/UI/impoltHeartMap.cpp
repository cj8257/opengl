#include "imgui.h"              // ImGui核心库
#include "implot.h"             // ImPlot绘图库
#include "implot_internal.h"    // ImPlot内部头文件，用于高级功能
#include <vector>               // 标准向量容器
#include <cmath>                // 数学函数库
#include <iostream>             // 输入输出流
#include <algorithm>            // 算法库
#include <ctime>                // 时间处理库
#include <iomanip>              // 输入输出格式控制
#include <sstream>              // 字符串流

// 定义常量：数据维度
constexpr int ANGLE_BINS = 128;   // 角度轴上的数据点数 (X轴) - 从0到360度
constexpr int TIME_SLICES = 256;  // 时间轴上的数据点数 (Y轴) - 时间片数

// 静态变量：用于在UI渲染循环中保持数据状态
static std::vector<double> spectrogram_data;  // 频谱数据存储
static bool data_initialized = false;         // 数据是否已初始化标志
static float animation_time = 0.0f;           // 动画时间计数器
static bool is_playing = true;                // 播放/暂停状态
static float animation_speed = 1.0f;          // 动画播放速度倍数
static float time_axis_offset = 0.0f;         // 时间轴偏移量，实现时间轴滚动效果
static time_t start_time;                     // 记录开始时间（用于时间格式化）

// 时间格式化函数：将数值转换为 HH:MM:SS 格式
std::string FormatTime(double time_value) {
    int total_seconds = static_cast<int>(time_value);  // 转换为秒数
    int hours = total_seconds / 3600;                  // 计算小时
    int minutes = (total_seconds % 3600) / 60;         // 计算分钟
    int seconds = total_seconds % 60;                  // 计算秒数

    std::ostringstream oss;                            // 字符串流
    oss << std::setfill('0') << std::setw(2) << hours << ":"    // 格式化小时
        << std::setfill('0') << std::setw(2) << minutes << ":"  // 格式化分钟
        << std::setfill('0') << std::setw(2) << seconds;        // 格式化秒数
    return oss.str();
}

// ImPlot时间轴格式化回调函数 - 修正为正确的函数指针类型
int TimeFormatterCallback(double value, char* buff, int size, void* user_data) {
    std::string time_str = FormatTime(value);          // 调用时间格式化函数
    int len = snprintf(buff, size, "%s", time_str.c_str());  // 将格式化字符串复制到缓冲区
    return len;                                        // 返回写入的字符数
}

// 函数前向声明
void GenerateNewTimeSlice(int time_slice);    // 生成新的时间片数据

// 初始化频谱数据函数
void InitializeSpectrogramData() {
    if (data_initialized) {                   // 如果已经初始化过，直接返回
        return;
    }

    start_time = time(nullptr);               // 记录程序启动时间
    std::cout << "Initializing spectrogram data..." << std::endl;  // 输出初始化信息
    spectrogram_data.resize(TIME_SLICES * ANGLE_BINS, 0.0);       // 调整数据大小：时间×角度

    #ifndef M_PI                              // 如果没有定义π常量
    #define M_PI 3.14159265358979323846      // 定义π常量
    #endif

    // 生成所有时间片的初始数据
    for (int t = 0; t < TIME_SLICES; ++t) {
        GenerateNewTimeSlice(t);              // 为每个时间片生成数据
    }

    data_initialized = true;                  // 设置初始化完成标志
    std::cout << "Data initialization complete." << std::endl;  // 输出完成信息
}

// 生成新的时间片数据函数
void GenerateNewTimeSlice(int time_slice) {
    double time_norm = static_cast<double>(time_slice) / TIME_SLICES;  // 归一化时间值

    // 遍历所有角度点
    for (int a = 0; a < ANGLE_BINS; ++a) {
        double amplitude = 0.0;               // 初始化信号幅度
        double angle_norm = static_cast<double>(a) / ANGLE_BINS;  // 归一化角度值（0-1）
        double angle_deg = angle_norm * 360.0;  // 转换为角度值（0-360度）
        double angle_rad = angle_deg * M_PI / 180.0;  // 转换为弧度值

        // 使用动画时间创建移动效果
        double animated_time = time_norm + animation_time * 0.1;

        // 1. 生成稳定的低角度信号（模拟某个方向的稳定信号源）
        double target_angle1 = 0.1;          // 目标角度位置
        amplitude += 0.5 * exp(-pow(angle_norm - target_angle1, 2) / 0.001);  // 高斯峰

        // 2. 生成时间变化信号（模拟移动目标）
        double target_angle2 = 0.3 + 0.2 * sin(animated_time * 2 * M_PI);  // 周期性移动的角度
        amplitude += 0.8 * exp(-pow(angle_norm - target_angle2, 2) / 0.002);  // 移动的高斯峰

        // 3. 生成旋转扫描信号（模拟雷达扫描）
        double scan_angle = 0.6 + 0.3 * sin(animated_time * M_PI);  // 扫描角度
        amplitude += 1.0 * exp(-pow(angle_norm - scan_angle, 2) / 0.001);  // 扫描信号

        // 4. 添加随机噪声
        amplitude += 0.1 * (rand() / (double)RAND_MAX);  // 0.1倍的随机噪声

        // 存储数据：time_slice * ANGLE_BINS + a 确保正确的数组索引
        spectrogram_data[time_slice * ANGLE_BINS + a] = amplitude;
    }
}

// 更新频谱数据以实现滚动效果
void UpdateSpectrogramData(float delta_time) {
    if (!is_playing) {                        // 如果暂停状态，不更新数据
        return;
    }

    animation_time += delta_time * animation_speed;  // 更新动画时间

    // 实现向上滚动效果（新数据从底部进入）
    static float accumulator = 0.0f;          // 时间累加器
    accumulator += delta_time * animation_speed * 10.0f; // 滚动速度控制

    if (accumulator >= 1.0f) {                // 当累加器达到阈值时执行滚动
        accumulator -= 1.0f;                  // 重置累加器
        time_axis_offset += 1.0f;             // 更新时间轴偏移量（以秒为单位）

        // 将所有时间片向上移动（索引较小的时间片）
        for (int t = 0; t < TIME_SLICES - 1; ++t) {
            for (int a = 0; a < ANGLE_BINS; ++a) {
                // 当前时间片 = 下一个时间片的数据
                spectrogram_data[t * ANGLE_BINS + a] = spectrogram_data[(t + 1) * ANGLE_BINS + a];
            }
        }

        // 在底部（最新时间）生成新的数据
        GenerateNewTimeSlice(TIME_SLICES - 1);
    }
}

// 主窗口渲染函数
void ShowSpectrogramWindow(bool* p_open) {
    // 确保数据已初始化
    InitializeSpectrogramData();

    // 计算帧间时间差
    static float last_time = 0.0f;
    float current_time = ImGui::GetTime();
    float delta_time = current_time - last_time;
    last_time = current_time;

    // 更新动画数据
    UpdateSpectrogramData(delta_time);

    // 设置现代窗口样式
    ImGui::SetNextWindowPos(ImVec2(20, 110), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(860, 940), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.32f, 1.0f));

    if (!ImGui::Begin("时方图 (Time-Frequency Plot)", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse)) {
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::End();
        return;
    }

    // 显示数据统计信息 - 现代化样式
    if (!spectrogram_data.empty()) {
        double min_val = *std::min_element(spectrogram_data.begin(), spectrogram_data.end());
        double max_val = *std::max_element(spectrogram_data.begin(), spectrogram_data.end());

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.85f, 0.90f, 1.0f));
        ImGui::Text("数据范围: %.3f to %.3f", min_val, max_val);
        ImGui::Text("当前时间: %s", FormatTime(time_axis_offset + TIME_SLICES).c_str());

        // 添加状态指示器
        ImGui::SameLine();
        ImGui::Text("● 状态: ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.80f, 0.30f, 1.0f));
        ImGui::Text("运行中");
        ImGui::PopStyleColor(1);
        ImGui::PopStyleColor(1);
    }

    ImGui::Spacing();

    // 使用蓝色色彩映射以匹配设计
    ImPlot::PushColormap(ImPlotColormap_Cool); // 使用冷色调色彩映射

    // 计算剩余空间给热力图使用
    float remaining_height = ImGui::GetContentRegionAvail().y;

    // 开始绘制现代化热力图 - 使用剩余的全部高度
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.05f, 0.06f, 0.08f, 1.0f));
    if (ImPlot::BeginPlot("##TimeFrequencyHeatmap", ImVec2(-1, remaining_height),
        ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoTitle)) {

        // 设置坐标轴标签 - 现代化风格
        ImPlot::SetupAxes("角度", "时间");

        // 计算时间轴范围（实现滚动效果）
        float time_min = time_axis_offset;
        float time_max = time_axis_offset + TIME_SLICES;

        // 设置坐标轴范围
        ImPlot::SetupAxesLimits(0, 360, time_max, time_min, ImGuiCond_Always);

        // 设置Y轴（时间轴）的自定义格式化器
        ImPlot::SetupAxisFormat(ImAxis_Y1, TimeFormatterCallback);

        // 绘制热力图 - 使用蓝色渐变
        ImPlot::PlotHeatmap(
            "Angular Spectrogram",
            spectrogram_data.data(),
            TIME_SLICES,
            ANGLE_BINS,
            0.0,                        // 颜色映射最小值
            2.0,                        // 颜色映射最大值
            NULL,                       // 标签格式（使用默认）
            ImPlotPoint(0, time_max),   // 热力图起始点坐标
            ImPlotPoint(360, time_min)  // 热力图结束点坐标
        );

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor(1);

    ImPlot::PopColormap();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();
}