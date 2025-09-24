#include <glad/glad.h>          // 加载OpenGL函数指针的库
#include <GLFW/glfw3.h>         // 跨平台窗口管理库
#include "imgui.h"              // 即时模式GUI库主头文件
#include "imgui_impl_glfw.h"    // ImGui的GLFW后端实现
#include "imgui_impl_opengl3.h" // ImGui的OpenGL3渲染后端
#include "implot.h"             // ImGui的绘图扩展库
#include <iostream>             // 标准输入输出流
#include "UI/MainController.h"  // 主控制器类
#include "UI/FrameController.h" // 帧数据控制器类
#include "UI/impoltHeartMap.h"  // 心率图谱显示组件
#include "UI/SystemControl.h"   // 系统控制组件

// GLFW错误回调函数 - 当GLFW发生错误时会调用此函数
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl; // 输出错误信息到标准错误流
}
// 初始化 ImGui 和字体
static void InitImGui(GLFWwindow* window) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // 初始化ImPlot（重构后使用ImPlot绘图库）
    ImPlot::CreateContext(); // 创建ImPlot绘图上下文

    ImGuiIO& io = ImGui::GetIO();
    
    // 加载中文字体（例如思源黑体）
    ImFont* font = io.Fonts->AddFontFromFileTTF("../utils/NotoSansSC-Black.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(font != nullptr); // 确保字体加载成功
    
    // 设置现代深色主题风格，匹配页面设计
    ImGui::StyleColorsDark();

    // 自定义颜色方案以匹配设计图
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 主要背景色 - 深黑色
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

    // 边框和分割线
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);

    // 按钮样式 - 蓝色主题
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.50f, 0.90f, 1.00f);        // 蓝色按钮
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.60f, 1.00f, 1.00f); // 悬停时更亮
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.40f, 0.80f, 1.00f);  // 按下时更暗

    // 框架和滑块
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.38f, 1.00f);

    // 滑块把手
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.60f, 1.00f, 1.00f);

    // 复选框
    colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.60f, 1.00f, 1.00f);

    // 标题栏
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

    // 文本颜色
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    // 设置圆角和间距
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 20.0f;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core"); // 初始化ImGui的OpenGL3后端 着色器（Shader）的目标版本是 GLSL 330
}

// 使用重构后的MainController架构的主函数
int main() {
    // 设置GLFW错误回调函数 - 注册错误处理回调
    glfwSetErrorCallback(glfw_error_callback);

    // 初始化GLFW库 - 必须在使用任何GLFW功能前调用
    if (!glfwInit())
        return -1; // 如果初始化失败，返回错误码
        // 2. 获取主监视器
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            std::cerr << "Failed to get the primary monitor" << std::endl;
            glfwTerminate();
            return -1;
        }
    
        // 3. 获取主监视器的当前视频模式
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            std::cerr << "Failed to get the video mode of the monitor" << std::endl;
            glfwTerminate();
            return -1;
        }
    // 设置OpenGL版本参数，使用3.3核心模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 设置OpenGL主版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // 设置OpenGL次版本号为3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 使用核心模式，去除过时功能

    // 创建1280x720分辨率的窗口
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "信号处理系统", NULL, NULL); // 
    if (!window) { // 如果窗口创建失败
        glfwTerminate(); // 清理GLFW资源
        return -1; // 返回错误码
    }
    glfwMakeContextCurrent(window); // 设置当前窗口的OpenGL上下文为活跃状态
    glfwSwapInterval(1); // 开启垂直同步 - 限制帧率与显示器刷新率同步

    // 初始化GLAD库 - 加载OpenGL函数指针
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize glad" << std::endl; // 输出错误信息
        return -1; // 返回错误码
    }

    InitImGui(window);
    // 创建主控制器实例（使用重构后的架构）
    // 连接到本地5555端口的socket服务器
    // MainController mainController("127.0.0.1", 5555);

    // 创建帧数据控制器实例
    FrameController frameController1("127.0.0.1", 5555);  // 主控制器，处理网络连接
    FrameController frameController2(frameController1.getDataManager());  // 共享数据的控制器

    // 创建系统控制组件实例
    SystemControl systemControl;
    
    // 用于控制频谱图窗口显示的布尔标志
    bool show_spectrogram = true;
    
    // 输出程序启动信息到控制台
    std::cout << "SensorMonitorApp started with refactored architecture" << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "- 128 channels @ 22.5kHz sampling rate" << std::endl; // 128通道22.5kHz采样率
    std::cout << "- Binary data format support" << std::endl; // 支持二进制数据格式
    std::cout << "- ImPlot-based professional charts" << std::endl; // 基于ImPlot的专业图表
    std::cout << "- Modular MVC architecture" << std::endl; // 模块化MVC架构
    std::cout << "- Play/Pause functionality" << std::endl; // 播放/暂停功能
    std::cout << "- Performance optimizations" << std::endl; // 性能优化

    // 主程序循环 - 持续运行直到用户关闭窗口
    while (!glfwWindowShouldClose(window)) {
        // 处理GLFW事件 - 处理键盘、鼠标等输入事件
        glfwPollEvents();

        // 开始新的ImGui帧
        ImGui_ImplOpenGL3_NewFrame(); // 开始OpenGL渲染后端的新帧
        ImGui_ImplGlfw_NewFrame();    // 开始GLFW输入后端的新帧
        ImGui::NewFrame();            // 开始ImGui的新帧
        
        // 使用MainController绘制UI（模块化架构）
        // mainController.drawUI(); // 调用主控制器的UI绘制方法

        // 使用FrameController绘制帧数据快照UI
        frameController1.drawUI(900, 110, 1000, 460,"谱图1"); // 调用帧控制器的UI绘制方法

         // 使用FrameController绘制帧数据快照UI
         frameController2.drawUI(900, 590, 1000, 460,"谱图2"); // 调用帧控制器的UI绘制方法

        // 绘制系统控制按钮（右上角的三个按钮）
        systemControl.drawControlButtons();

        // 绘制系统参数配置窗口（如果需要显示）
        systemControl.drawSystemConfigWindow();
        
        // 直接显示频谱图窗口
        if (show_spectrogram) { // 如果频谱图窗口开关为true
            ShowSpectrogramWindow(&show_spectrogram); // 显示频谱图窗口
        }

        // 渲染
        ImGui::Render(); // 结束ImGui帧并准备渲染数据
        int display_w, display_h; // 声明显示区域宽高变量
        glfwGetFramebufferSize(window, &display_w, &display_h); // 获取窗口帧缓冲区大小
        glViewport(0, 0, display_w, display_h); // 设置OpenGL视口大小
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 设置清屏颜色为深灰色
        glClear(GL_COLOR_BUFFER_BIT); // 清除颜色缓冲区
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // 渲染ImGui绘制数据
        glfwSwapBuffers(window); // 交换前后缓冲区，显示渲染结果
    }

    // 清理资源
    ImPlot::DestroyContext();        // 销毁ImPlot上下文
    ImGui_ImplOpenGL3_Shutdown();    // 关闭ImGui OpenGL后端
    ImGui_ImplGlfw_Shutdown();       // 关闭ImGui GLFW后端
    ImGui::DestroyContext();         // 销毁ImGui上下文

    glfwDestroyWindow(window); // 销毁GLFW窗口
    glfwTerminate();          // 清理GLFW库资源

    std::cout << "SensorMonitorApp shutdown completed" << std::endl; // 输出程序关闭完成信息
    return 0; // 程序正常退出
}