#pragma once
#define WINDOW_LENGTH 1024
#define STEP_LENGTH 512
#define SAMPLING_RATE 22500
#define SYSTEM_STATUS "start"

struct SystemConfig {
    static constexpr int windowLength = 1024;
    static constexpr int stepLength = 512;
    static constexpr double samplingRate = 22500;
    static constexpr const char* status = "start";
};
