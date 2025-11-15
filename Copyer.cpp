#include "Copyer.h"
#include "Config.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

// 文件元数据比较
struct FileMetadata {
    std::uintmax_t size;
    fs::file_time_type lastWrite;
};

std::optional<FileMetadata> GetFileMetadata(const fs::path& filePath) {
    try {
        return FileMetadata{
            fs::file_size(filePath),
            fs::last_write_time(filePath)
        };
    } catch (const fs::filesystem_error&) {
        return std::nullopt;
    }
}

bool ShouldCopyFile(const fs::path& src, const fs::path& dest) {
    if (!GetSkipDuplicateFile() || !fs::exists(dest)) {
        return true;
    }

    const auto srcMeta = GetFileMetadata(src);
    const auto destMeta = GetFileMetadata(dest);

    return !srcMeta || !destMeta ||
           (srcMeta->size != destMeta->size ||
            srcMeta->lastWrite != destMeta->lastWrite);
}

void CopyRecursively(const fs::path& srcDir, const fs::path& destDir, int depth, const std::atomic<bool>& exitFlag) {
    const int maxDepth = GetSearchMaxDepth();
    const auto& allowedExts = GetFileExts();
    const std::uintmax_t sizeLimit = GetFileSizeLimit();

    try {
        for (const auto& entry : fs::directory_iterator(srcDir)) {
            if (exitFlag.load()) return;

            const auto srcPath = entry.path();
            const auto destPath = destDir / srcPath.filename();

            if (entry.is_directory()) {
                if (maxDepth > 0 && depth >= maxDepth) continue;

                fs::create_directories(destPath);
                CopyRecursively(srcPath, destPath, depth + 1, exitFlag);
            }
            else if (entry.is_regular_file()) {
                // 文件扩展名检查
                const bool extMatched = allowedExts.empty() ||
                    std::any_of(allowedExts.begin(), allowedExts.end(),
                        [&](const auto& ext) { return EndsWith(srcPath.string(), ext); });

                // 文件大小检查
                const bool sizeValid = (sizeLimit == 0) ||
                    (entry.file_size() <= sizeLimit);

                if (extMatched && sizeValid && ShouldCopyFile(srcPath, destPath)) {
                    try {
                        fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing);

                        // 保持原始时间戳
                        if (const auto srcTime = fs::last_write_time(srcPath);
                            !exitFlag.load()) {
                            fs::last_write_time(destPath, srcTime);
                        }

                        std::cout << "Copy " << srcPath.string() << " -> " << destPath.string() << "\n";
                    } catch (const fs::filesystem_error& e) {
                        std::cerr << "[ERROR] Copy failed: " << e.what() << "\n";
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ERROR] Directory traversal failed: " << e.what() << "\n";
    }
}

void StartCopy(const std::string& srcDir, std::string destDir) {
    // 创建一个退出标志来控制此复制操作
    std::atomic<bool> exitFlag{false};

    // 延迟启动
    const int delay = GetDelayStart();
    if (delay > 0) {
        std::cout << "Sleep " << delay << "s before start...\n";
        std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    // 替换路径占位符
    const char drive = srcDir[0];
    if (const auto label = GetDeviceLabel(drive)) {
        ReplaceStr(destDir, "<drivelabel>", *label);
    }
    if (const auto serial = GetVolumeSerialNumber(drive)) {
        ReplaceStr(destDir, "<volumeserial>", *serial);
    }
    ReplaceStr(destDir, "<date>", GetDateString());
    ReplaceStr(destDir, "<time>", GetTimeString());

    // 创建目标目录
    std::error_code ec;
    fs::create_directories(destDir, ec);
    if (ec) {
        std::cerr << "[ERROR] Failed to create directory: " << ec.message() << "\n";
        return;
    }

    std::cout << "[INFO] Start syncing from " << srcDir << " -> " << destDir << "\n";
    CopyRecursively(srcDir, destDir, 1, exitFlag);
    std::cout << "[INFO] Sync process finished.\n";
}