#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>

char GetFirstDriveFromMask(ULONG unitmask)
{
    char i;
    for (i = 0; i < 26; ++i)
    {
        if (unitmask & 1)
            break;
        unitmask >>= 1;
    }
    return (i + 'A');
}

std::vector<std::string> SplitStrWithPattern(const std::string& str, const std::string& pattern)
{
    std::vector<std::string> resVec;

    if (str.empty())
        return resVec;

    std::string strs = str + pattern;

    size_t pos = strs.find(pattern);
    size_t size = strs.size();

    while (pos != std::string::npos) {
        std::string x = strs.substr(0, pos);
        resVec.push_back(x);
        strs = strs.substr(pos + 1, size);
        pos = strs.find(pattern);
    }

    return resVec;
}

bool StartsWith(const std::string& str, const std::string& start) {
    size_t srcLen = str.size();
    size_t startLen = start.size();
    if (srcLen >= startLen) {
        std::string temp = str.substr(0, startLen);
        if (temp == start)
            return true;
    }

    return false;
}

bool EndsWith(const std::string& str, const std::string& end) {
    size_t srcLen = str.size();
    size_t endLen = end.size();
    if (srcLen >= endLen) {
        std::string temp = str.substr(srcLen - endLen, endLen);
        if (temp == end)
            return true;
    }

    return false;
}

std::string& ReplaceStr(std::string& str, const std::string& old_value, const std::string& new_value) {
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = str.find(old_value, pos)) != std::string::npos)
            str.replace(pos, old_value.length(), new_value);
        else
            break;
    }
    return str;
}

std::optional<std::string> GetDeviceLabel(char drive)
{
    DWORD dwVolumeSerialNumber;
    DWORD dwMaximumComponentLength;
    DWORD dwFileSystemFlags;
    CHAR szFileSystemNameBuffer[1024];
    CHAR szDriveName[MAX_PATH];
    std::string drivePath = std::string(1, drive) + ":\\";

    if (!GetVolumeInformationA(drivePath.c_str(), szDriveName, MAX_PATH, &dwVolumeSerialNumber,
        &dwMaximumComponentLength, &dwFileSystemFlags, szFileSystemNameBuffer, sizeof(szFileSystemNameBuffer)))
    {
        return std::nullopt;
    }
    return std::string(szDriveName);
}

std::optional<std::string> GetVolumeSerialNumber(char drive)
{
    DWORD dwVolumeSerialNumber;
    std::string drivePath = std::string(1, drive) + ":\\";

    if (!GetVolumeInformationA(drivePath.c_str(), NULL, 0, &dwVolumeSerialNumber,
        NULL, NULL, NULL, 0))
    {
        return std::nullopt;
    }

    std::stringstream ss;
    ss << std::hex << dwVolumeSerialNumber;
    return ss.str();
}

std::string GetDateString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_s(&tm, &time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string GetTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_s(&tm, &time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H-%M-%S");
    return oss.str();
}

std::optional<std::uintmax_t> ParseFileSize(const std::string& sizeStr)
{
    if (sizeStr.empty()) return 0;

    std::string realNumStr;
    int ratio = 0;
    
    if (EndsWith(sizeStr, "GB") || EndsWith(sizeStr, "gb"))
    {
        realNumStr = sizeStr.substr(0, sizeStr.size() - 2);
        ratio = 30; // 2^30 = 1GB
    }
    else if (EndsWith(sizeStr, "MB") || EndsWith(sizeStr, "mb"))
    {
        realNumStr = sizeStr.substr(0, sizeStr.size() - 2);
        ratio = 20; // 2^20 = 1MB
    }
    else if (EndsWith(sizeStr, "KB") || EndsWith(sizeStr, "kb"))
    {
        realNumStr = sizeStr.substr(0, sizeStr.size() - 2);
        ratio = 10; // 2^10 = 1KB
    }
    else if (EndsWith(sizeStr, "B") || EndsWith(sizeStr, "b"))
    {
        realNumStr = sizeStr.substr(0, sizeStr.size() - 1);
        ratio = 0; // bytes
    }
    else
    {
        realNumStr = sizeStr;
        ratio = 0;
    }

    try 
    {
        // Remove whitespace
        realNumStr.erase(std::remove_if(realNumStr.begin(), realNumStr.end(), ::isspace), realNumStr.end());
        std::uintmax_t num = std::stoull(realNumStr);
        return num << ratio;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Invalid FileSizeLimit value: " << sizeStr << ". Using 0 (unlimited).\n";
        return std::nullopt;
    }
}