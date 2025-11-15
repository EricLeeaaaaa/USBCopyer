#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <ctime>

using namespace std::chrono_literals;

// 获取驱动器盘符
char GetFirstDriveFromMask(ULONG unitmask);

// 字符串工具
std::vector<std::string> SplitStrWithPattern(const std::string& str, const std::string& pattern);
bool StartsWith(const std::string& str, const std::string& start);
bool EndsWith(const std::string& str, const std::string& end);
std::string& ReplaceStr(std::string& str, const std::string& old_value, const std::string& new_value);

// 设备信息
std::optional<std::string> GetDeviceLabel(char drive);
std::optional<std::string> GetVolumeSerialNumber(char drive);

// 时间工具
std::string GetDateString();
std::string GetTimeString();

// 文件大小解析
std::optional<std::uintmax_t> ParseFileSize(const std::string& sizeStr);