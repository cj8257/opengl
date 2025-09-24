# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Main Application
```bash
# Configure and build using CMake
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .

# Run the application
./SensorMonitor
```

### Test Sender Programs
```bash
# Build socket-based test sender
./build_sender_socket.sh

# Run socket sender
./sender_socket
```

## Architecture Overview

### Core Components

- **MVC Architecture**: The application follows a Model-View-Controller pattern
  - `MainController`: Manages UI rendering and coordinates between components
  - `DataManager`: Thread-safe data processing and buffering system
  - UI components: Real-time visualization using ImGui/ImPlot

### Data Flow Architecture

- **Multi-threaded Design**:
  - Network thread: Receives data via socket connection on port 5555
  - Processing thread: Handles data parsing and buffering (128 channels @ 22.5kHz)
  - UI thread: Renders real-time charts and controls

- **Data Format**:
  - 128 channels, 8 samples per packet
  - 4096 bytes per packet (4 * 128 * 8)
  - float32 data type

### Key Classes

- `DataManager` (`src/Core/`): Thread-safe data processing with double-buffering for display
- `SocketSubscriber` (`src/IO/`): Network communication handler
- `MainController` (`src/UI/`): Main UI controller and application coordinator
- `impoltHeartMap` (`src/UI/`): Spectrogram and heart rate visualization components

### Dependencies

The project uses vcpkg for dependency management:
- **GLFW3**: Window management
- **GLAD**: OpenGL function loading
- **ImGui**: Immediate mode GUI
- **ImPlot**: Plotting library for ImGui
- **ZeroMQ**: High-performance messaging (legacy, now uses sockets)

### Project Structure

```
src/
├── Core/           # Data management and processing
├── IO/             # Network communication
├── UI/             # User interface components
└── main.cpp        # Application entry point

include/            # Header files matching src/ structure
third_party/        # Third-party libraries (ImGui, ImPlot, GLAD)
vcpkg/              # Package manager for dependencies
```

### Development Notes

- **C++17** standard required
- **Thread Safety**: DataManager uses atomic operations and mutex locks for thread-safe data access
- **Real-time Performance**: Optimized for high-frequency data display with configurable frame rates (30 FPS default)
- **Memory Management**: Pre-allocated buffers and circular queues to minimize allocations during runtime
- **Chinese Font Support**: Uses NotoSansSC-Black.ttf for Chinese characters in UI

### Network Configuration

- Default connection: `127.0.0.1:5555`
- Protocol: TCP socket communication
- Data rate: 22.5kHz sampling rate across 128 channels