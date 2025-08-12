#include <glad/glad.h>  // OpenGL加载库，用于加载OpenGL函数指针
#include <GLFW/glfw3.h>  // OpenGL框架库，用于创建窗口和管理OpenGL上下文
#include "imgui.h"      // 立即模式图形用户界面库
#include "imgui_impl_glfw.h"  // ImGui的GLFW后端实现
#include "imgui_impl_opengl3.h"  // ImGui的OpenGL 3后端实现
#include <iostream>   // 输入输出流库
#include <vector>     // 向量容器库
#include <cmath>      // 数学函数库
#include <algorithm>  // 算法库
#include <fstream>    // 文件流库
#include <zmq.h>      // ZeroMQ消息库头文件
#include <nlohmann/json.hpp>  // JSON库头文件
#include <thread>     // 线程库
#include <string>    // 字符串库

// 打印通道样本的辅助函数
void printChannelSamples(const std::vector<std::vector<float>>& samples, int channel_count = 1) {
    std::cout << "Channel samples (first " << channel_count << " channels):" << std::endl;
    for (int ch = 0; ch < channel_count && ch < samples.size(); ++ch) {
        std::cout << "  Ch" << ch << " [" << samples[ch].size() << "]: ";
        for (size_t i = 0; i < std::min(samples[ch].size(), size_t(5)); ++i) {
            std::cout << samples[ch][i] << " ";
        }
        if (samples[ch].size() > 5) std::cout << "...";
        std::cout << std::endl;
    }
}

// GLFW错误回调函数，用于处理GLFW错误
static void glfw_error_callback(int error, const char* description)
{
    // 输出GLFW错误信息到错误流
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// 主函数程序入口
int main()
{
    // 设置GLFW错误回调函数，用于捕获GLFW错误
    glfwSetErrorCallback(glfw_error_callback);

    // 初始化GLFW库，如果失败则返回错误代码
    if (!glfwInit())
        return -1;

    // 设置OpenGL版本参数，使用3.3核心模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 设置OpenGL主版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 设置OpenGL次版本号为3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 设置OpenGL为核心模式，不使用向后兼容特性

    // 创建1280x720分辨率的窗口，标题为SensorMonitorApp
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SensorMonitorApp", NULL, NULL);
    // 检查窗口是否创建成功，失败则终止GLFW并返回错误
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);  // 将创建的窗口设置为当前上下文
    glfwSwapInterval(1);  // 设置垂直同步间隔为1，开启垂直同步

    // 初始化GLAD库，用于加载OpenGL函数指针
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        // 输出GLAD初始化失败信息并返回错误
        std::cerr << "Failed to initialize glad" << std::endl;
        return -1;
    }

    // 初始化ImGui图形界面库
    IMGUI_CHECKVERSION();  // 检查ImGui版本
    ImGui::CreateContext();  // 创建ImGui上下文
    ImGuiIO& io = ImGui::GetIO();  // 获取ImGui的IO接口对象
    ImGui::StyleColorsDark();  // 设置ImGui为深色主题
    ImGui_ImplGlfw_InitForOpenGL(window, true);  // 初始化ImGui的GLFW后端，用于OpenGL
    ImGui_ImplOpenGL3_Init("#version 330 core");  // 初始化ImGui的OpenGL 3.3后端

    // ZeroMQ网络通信设置，使用PULL模式接收数据
    void* context = zmq_ctx_new();  // 创建ZeroMQ上下文
    // 检查ZeroMQ上下文是否创建成功，失败则输出错误并返回
    if (!context) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return -1;
    }
    
    void* receiver = zmq_socket(context, ZMQ_PULL);  // 创建ZMQ_PULL类型的套接字，用于接收数据
    // 检查套接字是否创建成功，失败则销毁上下文并返回错误
    if (!receiver) {
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        zmq_ctx_destroy(context);
        return -1;
    }
    
    // 设置接收高水位线，控制缓冲区大小
    int hwm = 1;  // 设置高水位线值为1000
    zmq_setsockopt(receiver, ZMQ_RCVHWM, &hwm, sizeof(hwm));  // 应用高水位线设置到接收套接字
    
    // 设置非阻塞接收模式
    int timeout = 100; // 100ms  // 设置超时时间为100毫秒
    zmq_setsockopt(receiver, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));  // 应用超时设置到接收套接字
    
    // 绑定网络地址作为服务器接收数据
    const std::string ADDRESS = "tcp://*:5555";  // 设置TCP地址，监听所有网络接口的5555端口
    // 绑定套接字到指定地址，如果失败则输出错误信息并清理资源
    if (zmq_bind(receiver, ADDRESS.c_str()) != 0) {
        std::cerr << "Failed to bind socket: " << zmq_strerror(errno) << std::endl;
        zmq_close(receiver);
        zmq_ctx_destroy(context);
        return -1;
    }
    
    // 输出ZeroMQ接收器启动信息
    std::cout << "ZeroMQ receiver started, listening on port 5555" << std::endl;
    
    // 定义与receiver.cpp一致的数据处理参数
    const size_t CHANNEL_COUNT = 128;  // 定义通道数量为128个
    const size_t SAMPLES_PER_PACKET = 8;  // 定义每个数据包包含8个采样点
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096字节  // 计算数据包大小：4字节 * 128通道 * 8采样 = 4096字节
    const size_t MAX_SAMPLES_PER_CHANNEL = 10000;  // 定义每个通道最大缓存样本数
    
    // 创建多通道采样数据缓存，基于receiver.cpp的实现
    std::vector<std::vector<float>> channel_samples(CHANNEL_COUNT);  // 创建128个通道的样本数据容器
    // 为每个通道预分配1000个样本的内存空间，避免频繁内存分配
    for (auto& samples : channel_samples) {
        samples.reserve(1000); // 预分配内存
    }
    
    // 定义多通道图表显示用的数据容器
    const size_t MAX_DISPLAY_SAMPLES = 1000;  // 每个通道最多显示1000个数据点
    const size_t DISPLAY_CHANNELS = CHANNEL_COUNT;  // 显示所有128个通道的曲线
    std::vector<std::vector<float>> channel_display_data(DISPLAY_CHANNELS);  // 多通道显示数据
    std::vector<float> time_values;            // 共享时间轴数据容器
    
    // 时间管理变量
    size_t total_samples_received = 0;         // 总接收样本数（用于时间计算）
    
    // 初始化各通道显示数据
    for (auto& channel_data : channel_display_data) {
        channel_data.reserve(MAX_DISPLAY_SAMPLES);  // 预分配内存
    }
    time_values.reserve(MAX_DISPLAY_SAMPLES);  // 预分配时间数组内存
    
    // 主程序循环，持续运行直到窗口关闭
    while (!glfwWindowShouldClose(window))
    {
        // 处理GLFW事件，如键盘输入、鼠标移动等
        glfwPollEvents();

        // 开始新的ImGui帧，OpenGL3后端
        ImGui_ImplOpenGL3_NewFrame();
        // 开始新的ImGui帧，GLFW后端
        ImGui_ImplGlfw_NewFrame();
        // 开始新的ImGui帧
        ImGui::NewFrame();

        // 按照receiver.cpp的方式接收数据
        std::vector<uint8_t> buffer(PACKAGE_SIZE);  // 创建缓冲区，大小为一个数据包的尺寸
        int recv_size = zmq_recv(receiver, buffer.data(), buffer.size(), ZMQ_DONTWAIT);  // 非阻塞方式接收数据，返回接收到的字节数
        
        // 定义静态变量，用于统计接收次数和上次接收大小
        static int count = 0;
        static int last_recv_size = 0;
        // 如果接收到数据（recv_size > 0）
        if (recv_size > 0) {
            last_recv_size = recv_size;
            // 检查接收到的数据包大小是否符合预期
            if (recv_size != static_cast<int>(PACKAGE_SIZE)) {
                // 如果接收大小与预期不符，输出错误信息
                std::cerr << "Received unexpected packet size: " << recv_size 
                          << " (expected " << PACKAGE_SIZE << ")" << std::endl;
            }
            // 如果数据包大小正确
            else {
                // 将接收到的字节数据转换为浮点数样本
                const float* samples = reinterpret_cast<const float*>(buffer.data());  // 将缓冲区数据转换为浮点数指针
                size_t sample_count = PACKAGE_SIZE / sizeof(float); // 128 * 8 = 1024 floats  // 计算样本总数：4096字节 / 4字节 = 1024个浮点数
                
                // 按照receiver.cpp的逻辑缓存每个通道的采样数据
                // 遍历所有通道
                    // 检查通道缓存是否已满
                    // if (channel_samples[channel].size() >= MAX_SAMPLES_PER_CHANNEL) {
                    //     // 如果通道缓存已满，输出错误信息并跳过此通道
                    //     std::cerr << "Channel " << channel << " cache full, discarding packet" << std::endl;
                    //     continue;
                    // }
                    // 遍历数据包中的每个采样点
                    for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
                          for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {

                        size_t index = channel * SAMPLES_PER_PACKET + sample;  // 计算在样本数组中的索引位置
                        // 检查索引是否在有效范围内
                        if (index < sample_count) {
                            // 将样本数据添加到对应通道的缓存中
                            channel_samples[channel].push_back(samples[index]);
                        }
                    }
                }
                // 按照receiver.cpp的格式输出接收日志
                std::cout << "[RECV] #" << (++count) << " | Size: " << recv_size << " bytes" << std::endl;
                
                // 调用打印函数，显示通道样本信息
                printChannelSamples(channel_samples);
                
                // 打印第一个通道的采样数据（用于调试，按照receiver.cpp的方式）
                if (!channel_samples[0].empty()) {  // 如果第一个通道有数据，输出最新的几个样本
                    std::cout << "Channel 0 (last " << SAMPLES_PER_PACKET << " samples): ";  // 输出标题信息
                    size_t start = channel_samples[0].size() > SAMPLES_PER_PACKET ?  // 计算起始索引：显示最后SAMPLES_PER_PACKET个样本
                                  channel_samples[0].size() - SAMPLES_PER_PACKET : 0;
                    // 输出最新的样本数据
                    for (size_t i = start; i < channel_samples[0].size(); ++i) {
                        std::cout << channel_samples[0][i] << " ";  // 输出样本值
                    }
                    std::cout << std::endl;  // 换行
                }
                
                // 将多通道采样数据转换为图表显示，为每个显示通道生成时间对应数据
                // 基于实际接收到的数据包来计算时间，而不是实时时间
                
                // 为每个接收到的样本包更新显示数据
                for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
                    // 基于总接收样本数计算时间戳，假设采样率为1000Hz
                    double sample_time = total_samples_received * (1.0 / 1000 );  // 1kHz采样率
                    
                    // 更新各显示通道的数据
                    for (size_t display_ch = 0; display_ch < DISPLAY_CHANNELS; ++display_ch) {
                        if (display_ch < CHANNEL_COUNT && !channel_samples[display_ch].empty()) {
                            // 获取该通道最新的样本数据
                            size_t sample_idx = channel_samples[display_ch].size() - SAMPLES_PER_PACKET + sample;
                            if (sample_idx < channel_samples[display_ch].size()) {
                                float sample_value = channel_samples[display_ch][sample_idx];
                                
                                // 添加新数据点，保持数组大小限制
                                if (channel_display_data[display_ch].size() >= MAX_DISPLAY_SAMPLES) {
                                    channel_display_data[display_ch].erase(channel_display_data[display_ch].begin());
                                }
                                channel_display_data[display_ch].push_back(sample_value);
                            }
                        }
                    }
                    
                    // 更新时间轴（所有通道共享）
                    if (time_values.size() >= MAX_DISPLAY_SAMPLES) {
                        time_values.erase(time_values.begin());
                    }
                    time_values.push_back(static_cast<float>(sample_time));
                    
                    total_samples_received++;
                }
            }
        }
        
        // 创建全屏图表显示窗口，无边框无标题栏
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);  // 设置窗口位置为屏幕左上角
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);  // 设置窗口大小为整个显示区域
        // 创建窗口，设置各种无边框和无控制标志
        ImGui::Begin("##Charts", nullptr, 
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |    // 无标题栏，不可调整大小
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |      // 不可移动，不可折叠
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);  // 无滚动条，鼠标不滚动
        
        // 计算图表的显示尺寸和位置参数  
        ImVec2 window_size = ImGui::GetWindowSize();  // 获取窗口的实际大小
        float margin = 80.0f;  // 边距用于显示坐标轴  // 定义边距大小，用于绘制坐标轴
        float chart_width = window_size.x - margin * 2;  // 计算图表宽度，扣除左右边距
        float chart_height = (window_size.y - margin * 3);  // 计算图表高度，上下边距
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();  // 获取ImGui的绘制列表，用于自定义绘制
        ImVec2 window_pos = ImGui::GetWindowPos();  // 获取窗口在屏幕上的位置
        
        // === 多通道图表绘制区域 ===
        ImVec2 chart_pos = ImVec2(window_pos.x + margin, window_pos.y + margin);  // 图表的起始位置（左上角坐标）
        ImVec2 chart_end = ImVec2(chart_pos.x + chart_width, chart_pos.y + chart_height);  // 图表的结束位置（右下角坐标）
        
        // 绘制图表的边框矩形
        draw_list->AddRect(chart_pos, chart_end, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);  // 添加矩形边框，灰色，圆角0，线宽2像素
        
        // 动态计算Y轴范围（基于所有显示通道的数据）
        float min_value = 0.0f, max_value = 40.0f;  // 默认范围
        bool has_data = false;
        
        for (size_t ch = 0; ch < DISPLAY_CHANNELS; ++ch) {
            if (!channel_display_data[ch].empty()) {
                if (!has_data) {  // 第一次找到数据
                    min_value = *std::min_element(channel_display_data[ch].begin(), channel_display_data[ch].end());
                    max_value = *std::max_element(channel_display_data[ch].begin(), channel_display_data[ch].end());
                    has_data = true;
                } else {  // 扩展范围
                    min_value = std::min(min_value, *std::min_element(channel_display_data[ch].begin(), channel_display_data[ch].end()));
                    max_value = std::max(max_value, *std::max_element(channel_display_data[ch].begin(), channel_display_data[ch].end()));
                }
            }
        }
        
        // 确保Y轴范围合理
        if (max_value <= min_value) {
            max_value = min_value + 1.0f;
        }
        
        // 绘制图表的Y轴刻度和标签
        for (int i = 0; i <= 4; i++) {  // 绘制5个刻度点
            float scale_value = min_value + (max_value - min_value) * i / 4.0f;  // 计算当前刻度的值
            float y_pos = chart_end.y - (i / 4.0f) * chart_height;  // 计算刻度在图表中的Y坐标位置
            
            // 绘制Y轴刻度线
            draw_list->AddLine(ImVec2(chart_pos.x - 5, y_pos),  // 在刻度位置绘制短横线作为刻度标记
                              ImVec2(chart_pos.x, y_pos), 
                              IM_COL32(150, 150, 150, 255), 1.0f);
            
            // 绘制Y轴刻度的标签
            char scale_label[16];
            snprintf(scale_label, sizeof(scale_label), "%.1f", scale_value);  // 格式化标签文本
            draw_list->AddText(ImVec2(chart_pos.x - 55, y_pos - 8),  // 在刻度线左侧绘制标签
                              IM_COL32(200, 200, 200, 255), scale_label);
        }
        
        // 绘制图表的X轴标签
        if (!time_values.empty()) {
            // 显示实际时间范围
            char time_start[32], time_end[32];
            snprintf(time_start, sizeof(time_start), "%.1fs", time_values.front());
            snprintf(time_end, sizeof(time_end), "%.1fs", time_values.back());
            
            draw_list->AddText(ImVec2(chart_pos.x, chart_end.y + 10),  // 在图表底部左侧绘制起始时间
                              IM_COL32(200, 200, 200, 255), time_start);
            draw_list->AddText(ImVec2(chart_end.x - 50, chart_end.y + 10),  // 在图表底部右侧绘制结束时间
                              IM_COL32(200, 200, 200, 255), time_end);
        }
        
        // 绘制图表的标题
        char title[64];
        snprintf(title, sizeof(title), "Multi-Channel Data (All %zu Channels)", DISPLAY_CHANNELS);
        draw_list->AddText(ImVec2(chart_pos.x + chart_width/2 - 100, chart_pos.y - 25),  // 在图表顶部中央绘制标题
                          IM_COL32(255, 255, 255, 255), title);
        
        // 为128个通道生成颜色（使用HSV色彩空间以获得更好的颜色分布）
        auto generateChannelColor = [](size_t channel_index, size_t total_channels) -> ImU32 {
            float hue = (channel_index * 360.0f) / total_channels;  // 色调分布在0-360度
            float saturation = 0.8f + 0.2f * ((channel_index % 5) / 4.0f);  // 饱和度0.8-1.0
            float value = 0.7f + 0.3f * ((channel_index % 3) / 2.0f);       // 明度0.7-1.0
            
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
            
            return IM_COL32((r + m) * 255, (g + m) * 255, (b + m) * 255, 255);
        };
        
        // 绘制所有通道的曲线 - 使用自定义绘制避免ImGui布局问题
        if (!time_values.empty()) {
            for (size_t ch = 0; ch < DISPLAY_CHANNELS; ++ch) {
                if (!channel_display_data[ch].empty() && channel_display_data[ch].size() == time_values.size()) {
                    ImU32 channel_color = generateChannelColor(ch, DISPLAY_CHANNELS);
                    
                    // 手动绘制曲线
                    if (channel_display_data[ch].size() > 1) {
                        for (size_t i = 1; i < channel_display_data[ch].size(); ++i) {
                            // 计算点的屏幕坐标
                            float x1 = chart_pos.x + ((i - 1) / float(channel_display_data[ch].size() - 1)) * chart_width;
                            float x2 = chart_pos.x + (i / float(channel_display_data[ch].size() - 1)) * chart_width;
                            
                            float y1 = chart_end.y - ((channel_display_data[ch][i-1] - min_value) / (max_value - min_value)) * chart_height;
                            float y2 = chart_end.y - ((channel_display_data[ch][i] - min_value) / (max_value - min_value)) * chart_height;
                            
                            // 绘制线段，根据通道数量调整线条粗细
                            float line_thickness = (DISPLAY_CHANNELS > 64) ? 0.8f : 1.2f;
                            draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), channel_color, line_thickness);
                        }
                    }
                }
            }
        }
        
        // 绘制通道数量信息（在右上角）
        char channel_info[64];
        snprintf(channel_info, sizeof(channel_info), "Displaying %zu channels", DISPLAY_CHANNELS);
        draw_list->AddText(ImVec2(chart_end.x - 180, chart_pos.y + 10), 
                          IM_COL32(200, 200, 200, 255), channel_info);
        
        // 添加一个不可见的项目来确保窗口边界正确
        ImGui::SetCursorPos(ImVec2(margin, margin));
        ImGui::Dummy(ImVec2(chart_width, chart_height));
        
        ImGui::End();  // 结束当前窗口的绘制

        ImGui::Render();  // 渲染ImGui界面
        int display_w, display_h;  // 定义显示区域的宽度和高度变量
        glfwGetFramebufferSize(window, &display_w, &display_h);  // 获取窗口的帧缓冲区大小
        glViewport(0, 0, display_w, display_h);  // 设置OpenGL视口
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // 设置清除颜色（深灰色）
        glClear(GL_COLOR_BUFFER_BIT);  // 清除颜色缓冲区
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());  // 使用OpenGL渲染ImGui的绘制数据
        glfwSwapBuffers(window);  // 交换前后缓冲区，显示绘制结果
    }  // 主循环结束

    // 程序结束时，保存缓存的采样数据到文件（按照receiver.cpp的方式）
    std::ofstream out("cached_samples.bin", std::ios::binary);  // 以二进制模式打开输出文件流
    // 检查文件是否成功打开
    if (out) {
        // 遍历所有通道
        for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
            const auto& samples = channel_samples[channel];  // 获取当前通道的样本数据引用
            // 将通道索引写入文件
            out.write(reinterpret_cast<const char*>(&channel), sizeof(channel));  // 以二进制形式写入通道索引
            // 将样本数量写入文件
            size_t sample_count = samples.size();  // 获取当前通道的样本数量
            out.write(reinterpret_cast<const char*>(&sample_count), sizeof(sample_count));  // 以二进制形式写入样本数量
            // 将采样数据写入文件
            out.write(reinterpret_cast<const char*>(samples.data()), sample_count * sizeof(float));  // 以二进制形式写入所有样本数据
        }
        // 输出保存成功信息
        std::cout << "Saved cached samples to cached_samples.bin" << std::endl;
    }
    
    // 输出数据缓存的摘要信息（按照receiver.cpp的方式）
    std::cout << "Cache summary:" << std::endl;  // 输出缓存摘要标题
    // 遍历所有通道，输出每个通道的样本数量
    for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
        std::cout << "Channel " << channel << ": " << channel_samples[channel].size() << " samples" << std::endl;
    }
    
    // 清理ZeroMQ相关资源
    zmq_close(receiver);  // 关闭ZeroMQ套接字
    zmq_ctx_destroy(context);  // 销毁ZeroMQ上下文
    
    ImGui_ImplOpenGL3_Shutdown();  // 关闭ImGui OpenGL3后端
    ImGui_ImplGlfw_Shutdown();    // 关闭ImGui GLFW后端
    ImGui::DestroyContext();      // 销毁ImGui上下文

    glfwDestroyWindow(window);  // 销毁GLFW窗口
    glfwTerminate();            // 终止GLFW库

    return 0;  // 程序正常结束，返回0
}