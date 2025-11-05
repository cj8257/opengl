#pragma once
#include <string>

void ShowSpectrogramWindow(bool* p_open = nullptr, const std::string& host = "127.0.0.1", int port = 5556, bool showData = true);
void CleanupSpectrogramResources();  // 清理资源函数