cmake_minimum_required(VERSION 3.10)
project(SensorMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Add source files
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

file(GLOB IMPLOT_SOURCES "third_party/implot/*.cpp")
file(GLOB_RECURSE GLAD_SOURCES "third_party/glad/src/*.c")

add_executable(SensorMonitor
    src/main_refactored.cpp
    src/Core/DataManager.cpp
    src/IO/SocketSubscriber.cpp
    src/UI/MainController.cpp
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${GLAD_SOURCES}
)

# Include directories
target_include_directories(SensorMonitor PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include"
)

# Link libraries
target_link_libraries(SensorMonitor PRIVATE
    glfw
    imgui::imgui
    implot::implot
    Threads::Threads
)

if(WIN32)
    target_link_libraries(SensorMonitor PRIVATE ws2_32)
else()
    target_link_libraries(SensorMonitor PRIVATE -lpthread)
endif()