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
    int hwm = 1000;  // 设置高水位线值为1000
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
    
    // 定义图表显示用的数据容器
    std::vector<float> temperature_data;  // 温度图表数据容器
    std::vector<float> humidity_data;    // 湿度图表数据容器
    std::vector<float> time_values;      // 时间轴数据容器
    
    // 初始化图表数据，填充初始值
    // 填充100个时间点和对应的温度、湿度初始值
    for (int i = 0; i < 100; i++) {
        time_values.push_back(i * 0.1f);  // 时间从0到9.9秒，间隔0.1秒
        temperature_data.push_back(0.0f);   // 温度初始值为0
        humidity_data.push_back(0.0f);    // 湿度初始值为0
    }
    
    float current_temperature = 20.0f;  // 定义当前温度变量，初始值20.0
    float current_humidity = 50.0f;     // 定义当前湿度变量，初始值50.0

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
                
                // 将多通道采样数据转换为图表显示的温度和湿度数据
                if (!channel_samples[0].empty() && !channel_samples[1].empty()) {  // 如果通道0和通道1都有数据
                    // 使用通道0的数据作为温度基础，通道1作为湿度基础
                    float raw_temp = channel_samples[0].back();  // 获取通道0的最新样本值
                    float raw_humid = channel_samples[1].back();  // 获取通道1的最新样本值
                    
                    // 将原始采样值映射到实际的传感器范围
                    current_temperature = 22.0f + raw_temp * 8.0f;  // 温度范围：22±8度
                    current_humidity = 50.0f + raw_humid * 25.0f;    // 湿度范围：50±25%
                    
                    // 限制温度和湿度在有效范围内
                    current_temperature = std::max(0.0f, std::min(40.0f, current_temperature));  // 温度限制在0-40度之间
                    current_humidity = std::max(0.0f, std::min(100.0f, current_humidity));      // 湿度限制在0-100%之间
                    
                    // 更新图表显示的数据
                    temperature_data.erase(temperature_data.begin());  // 移除最旧的数据点，保持数据长度不变
                    humidity_data.erase(humidity_data.begin());
                    time_values.erase(time_values.begin());
                    
                    float new_time = time_values.back() + 0.1f;  // 添加新的数据点：时间递增0.1秒
                    time_values.push_back(new_time);
                    temperature_data.push_back(current_temperature);  // 温度和湿度更新为最新值
                    humidity_data.push_back(current_humidity);
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
        float chart_height = (window_size.y - margin * 3) / 2.0f;  // 三个边距：上、中、下  // 计算图表高度，上下边距，两个图表平分剩余空间
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();  // 获取ImGui的绘制列表，用于自定义绘制
        ImVec2 window_pos = ImGui::GetWindowPos();  // 获取窗口在屏幕上的位置
        
        // === 温度图表绘制区域（上半部分） ===
        ImVec2 temp_chart_pos = ImVec2(window_pos.x + margin, window_pos.y + margin);  // 温度图表的起始位置（左上角坐标）
        ImVec2 temp_chart_end = ImVec2(temp_chart_pos.x + chart_width, temp_chart_pos.y + chart_height);  // 温度图表的结束位置（右下角坐标）
        
        // 绘制温度图表的边框矩形
        draw_list->AddRect(temp_chart_pos, temp_chart_end, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);  // 添加矩形边框，灰色，圆角0，线宽2像素
        
        // 绘制温度图表的Y轴刻度和标签
        for (int i = 0; i <= 4; i++) {  // 绘制5个刻度点（0,10,20,30,40度）
            float temp_value = i * 10.0f;  // 0, 10, 20, 30, 40  // 计算当前刻度的温度值
            float y_pos = temp_chart_end.y - (i / 4.0f) * chart_height;  // 计算刻度在图表中的Y坐标位置
            
            // 绘制Y轴刻度线
            draw_list->AddLine(ImVec2(temp_chart_pos.x - 5, y_pos),  // 在刻度位置绘制短横线作为刻度标记
                              ImVec2(temp_chart_pos.x, y_pos), 
                              IM_COL32(150, 150, 150, 255), 1.0f);
            
            // 绘制Y轴刻度的温度标签
            char temp_label[16];
            snprintf(temp_label, sizeof(temp_label), "%.0f°C", temp_value);  // 格式化温度标签文本
            draw_list->AddText(ImVec2(temp_chart_pos.x - 45, y_pos - 8),  // 在刻度线左侧绘制温度标签
                              IM_COL32(200, 200, 200, 255), temp_label);
        }
        
        // 绘制温度图表的X轴标签
        draw_list->AddText(ImVec2(temp_chart_pos.x, temp_chart_end.y + 10),  // 在图表底部左侧绘制"Time"标签
                          IM_COL32(200, 200, 200, 255), "Time");
        draw_list->AddText(ImVec2(temp_chart_end.x - 30, temp_chart_end.y + 10),  // 在图表底部右侧绘制"Now"标签
                          IM_COL32(200, 200, 200, 255), "Now");
        
        // 绘制温度图表的标题
        draw_list->AddText(ImVec2(temp_chart_pos.x + chart_width/2 - 40, temp_chart_pos.y - 25),  // 在图表顶部中央绘制"Temperature"标题
                          IM_COL32(255, 255, 255, 255), "Temperature");
        
        // 使用ImGui内置函数绘制温度图表曲线
        ImGui::SetCursorPos(ImVec2(margin, margin));  // 设置光标位置，准备绘制图表
        ImGui::PlotLines("##Temperature", temperature_data.data(), temperature_data.size(),  // 绘制温度曲线图表，数据范围0-40度
                        0, nullptr, 0.0f, 40.0f, ImVec2(chart_width, chart_height));
        
        // === 湿度图表绘制区域（下半部分） ===
        float humidity_y_start = margin * 2 + chart_height;  // 计算湿度图表的起始Y位置（在温度图表下方）
        ImVec2 humid_chart_pos = ImVec2(window_pos.x + margin, window_pos.y + humidity_y_start);  // 湿度图表的起始位置（左上角坐标）
        ImVec2 humid_chart_end = ImVec2(humid_chart_pos.x + chart_width, humid_chart_pos.y + chart_height);  // 湿度图表的结束位置（右下角坐标）
        
        // 绘制湿度图表的边框矩形
        draw_list->AddRect(humid_chart_pos, humid_chart_end, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);  // 添加矩形边框，灰色，圆角0，线宽2像素
        
        // 绘制湿度图表的Y轴刻度和标签
        for (int i = 0; i <= 5; i++) {  // 绘制6个刻度点（0,20,40,60,80,100%）
            float humid_value = i * 20.0f;  // 0, 20, 40, 60, 80, 100  // 计算当前刻度的湿度值
            float y_pos = humid_chart_end.y - (i / 5.0f) * chart_height;  // 计算刻度在图表中的Y坐标位置
            
            // 绘制Y轴刻度线
            draw_list->AddLine(ImVec2(humid_chart_pos.x - 5, y_pos),  // 在刻度位置绘制短横线作为刻度标记
                              ImVec2(humid_chart_pos.x, y_pos), 
                              IM_COL32(150, 150, 150, 255), 1.0f);
            
            // 绘制Y轴刻度的湿度标签
            char humid_label[16];
            snprintf(humid_label, sizeof(humid_label), "%.0f%%", humid_value);  // 格式化湿度标签文本
            draw_list->AddText(ImVec2(humid_chart_pos.x - 45, y_pos - 8),  // 在刻度线左侧绘制湿度标签
                              IM_COL32(200, 200, 200, 255), humid_label);
        }
        
        // 绘制湿度图表的X轴标签
        draw_list->AddText(ImVec2(humid_chart_pos.x, humid_chart_end.y + 10),  // 在图表底部左侧绘制"Time"标签
                          IM_COL32(200, 200, 200, 255), "Time");
        draw_list->AddText(ImVec2(humid_chart_end.x - 30, humid_chart_end.y + 10),  // 在图表底部右侧绘制"Now"标签
                          IM_COL32(200, 200, 200, 255), "Now");
        
        // 绘制湿度图表的标题
        draw_list->AddText(ImVec2(humid_chart_pos.x + chart_width/2 - 30, humid_chart_pos.y - 25),  // 在图表顶部中央绘制"Humidity"标题
                          IM_COL32(255, 255, 255, 255), "Humidity");
        
        // 使用ImGui内置函数绘制湿度图表曲线
        ImGui::SetCursorPos(ImVec2(margin, humidity_y_start));  // 设置光标位置，准备绘制湿度图表
        ImGui::PlotLines("##Humidity", humidity_data.data(), humidity_data.size(),  // 绘制湿度曲线图表，数据范围0-100%
                        0, nullptr, 0.0f, 100.0f, ImVec2(chart_width, chart_height));
        
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