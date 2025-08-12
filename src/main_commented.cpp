// 包含glad库的头文件，用于加载OpenGL函数指针
#include <glad/glad.h>
// 包含GLFW库的头文件，用于创建窗口和处理输入
#include <GLFW/glfw3.h>
// 包含ImGui库的头文件，用于创建图形用户界面
#include "imgui.h"
// 包含ImGui的GLFW后端实现，用于将ImGui与GLFW集成
#include "imgui_impl_glfw.h"
// 包含ImGui的OpenGL3后端实现，用于将ImGui与OpenGL3渲染管线集成
#include "imgui_impl_opengl3.h"
// 包含ImPlot库的头文件，用于绘制图表
#include "implot.h"
// 包含标准输入输出流库的头文件
#include <iostream>
// 包含文件流库的头文件，用于文件操作
#include <fstream>
// 包含主控制器类的头文件
#include "UI/MainController.h"

// 定义GLFW错误回调函数
static void glfw_error_callback(int error, const char* description) {
    // 在标准错误流中输出GLFW错误代码和描述信息
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// 主函数，程序入口点
int main() {
    // 设置自定义的GLFW错误回调函数
    glfwSetErrorCallback(glfw_error_callback);

    // 初始化GLFW库
    if (!glfwInit())
        // 如果初始化失败，返回-1表示错误
        return -1;

    // 设置GLFW窗口提示，指定OpenGL主版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // 设置GLFW窗口提示，指定OpenGL次版本号为3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // 设置GLFW窗口提示，指定使用核心模式(Core Profile)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建一个1280x720大小的窗口，标题为"SensorMonitorApp - Refactored"
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SensorMonitorApp - Refactored", NULL, NULL);
    // 检查窗口是否创建成功
    if (!window) {
        // 如果窗口创建失败，终止GLFW
        glfwTerminate();
        // 返回-1表示错误
        return -1;
    }
    // 将当前窗口的上下文设置为当前线程的主上下文
    glfwMakeContextCurrent(window);
    // 设置交换间隔为1，即开启垂直同步
    glfwSwapInterval(1); 

    // 初始化GLAD库，加载OpenGL函数指针
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        // 如果GLAD初始化失败，向标准错误流输出错误信息
        std::cerr << "Failed to initialize glad" << std::endl;
        // 返回-1表示错误
        return -1;
    }

    // 检查ImGui版本是否与头文件匹配
    IMGUI_CHECKVERSION();
    // 创建ImGui上下文
    ImGui::CreateContext();
    
    // 创建ImPlot上下文
    ImPlot::CreateContext();
    
    // 获取对ImGui IO对象的引用
    ImGuiIO& io = ImGui::GetIO();
    
    // 定义一个包含多种中文字体路径的数组
    const char* font_paths[] = {
        "C:/Windows/Fonts/msyh.ttc",  // 微软雅黑 (Windows)
        "C:/Windows/Fonts/simhei.ttf", // 黑体 (Windows)
        "/System/Library/Fonts/Helvetica.ttc", // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" // Linux
    };
    
    // 标志位，用于记录是否成功加载了字体
    bool font_loaded = false;
    // 遍历字体路径数组
    for (const char* font_path : font_paths) {
        // 检查字体文件是否存在且可读
        if (std::ifstream(font_path).good()) {
            // 从文件加载字体，并指定字体大小和字形范围（包含完整中文字符）
            io.Fonts->AddFontFromFileTTF(font_path, 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
            // 设置字体加载成功标志位为true
            font_loaded = true;
            // 成功加载一个字体后即退出循环
            break;
        }
    }
    
    // 如果没有成功加载任何中文字体
    if (!font_loaded) {
        // 在控制台输出提示信息，告知用户将使用默认英文字体
        std::cout << "No Chinese font found, using default font (English only)" << std::endl;
    }
    
    // 设置ImGui的颜色风格为深色模式
    ImGui::StyleColorsDark();
    // 为OpenGL初始化ImGui的GLFW后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // 为OpenGL 3.3核心模式初始化ImGui的OpenGL3后端
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // 定义ZeroMQ服务器的地址
    const std::string ADDRESS = "tcp://*:5555";
    // 创建主控制器对象，并传入地址
    MainController mainController(ADDRESS);
    
    // 在控制台输出启动信息
    std::cout << "SensorMonitorApp started with refactored architecture" << std::endl;
    // 在控制台输出功能特性列表
    std::cout << "Features:" << std::endl;
    std::cout << "- 128 channels @ 22.5kHz sampling rate" << std::endl;
    std::cout << "- Binary data format support" << std::endl;
    std::cout << "- ImPlot-based professional charts" << std::endl;
    std::cout << "- Modular MVC architecture" << std::endl;
    std::cout << "- Play/Pause functionality" << std::endl;
    std::cout << "- Performance optimizations" << std::endl;

    // 主循环，只要窗口没有被要求关闭就一直执行
    while (!glfwWindowShouldClose(window)) {
        // 处理所有待处理的GLFW事件
        glfwPollEvents();

        // 开始一个新的ImGui OpenGL3帧
        ImGui_ImplOpenGL3_NewFrame();
        // 开始一个新的ImGui GLFW帧
        ImGui_ImplGlfw_NewFrame();
        // 开始一个新的ImGui帧
        ImGui::NewFrame();

        // 调用主控制器的方法来绘制UI界面
        mainController.drawUI();

        // 渲染ImGui绘制的数据
        ImGui::Render();
        // 获取帧缓冲区的大小
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        // 设置OpenGL视口
        glViewport(0, 0, display_w, display_h);
        // 设置清屏颜色
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // 清除颜色缓冲区
        glClear(GL_COLOR_BUFFER_BIT);
        // 渲染ImGui的绘制数据
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // 交换前后缓冲区，将渲染结果显示在屏幕上
        glfwSwapBuffers(window);
    }

    // 销毁ImPlot上下文
    ImPlot::DestroyContext();
    // 关闭ImGui的OpenGL3后端
    ImGui_ImplOpenGL3_Shutdown();
    // 关闭ImGui的GLFW后端
    ImGui_ImplGlfw_Shutdown();
    // 销毁ImGui上下文
    ImGui::DestroyContext();

    // 销毁窗口对象
    glfwDestroyWindow(window);
    // 终止GLFW库
    glfwTerminate();

    // 在控制台输出关闭完成信息
    std::cout << "SensorMonitorApp shutdown completed" << std::endl;
    // 程序正常退出，返回0
    return 0;
}