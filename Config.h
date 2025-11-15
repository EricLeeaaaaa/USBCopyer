#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <cstdint>

bool InitConfig();

// 线程安全的配置访问器
int GetSearchMaxDepth();
int GetDelayStart();
const std::vector<std::string>& GetFileExts();
std::uintmax_t GetFileSizeLimit();
std::string GetSaveDir();
bool GetSkipDuplicateFile();