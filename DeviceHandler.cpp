#include "DeviceHandler.h"
#include "Config.h"
#include "Copyer.h"
#include "Utils.h"
#include <Windows.h>
#include <Dbt.h>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

// 线程安全的设备管理
namespace {
    std::mutex g_deviceMutex;
    std::unordered_map<char, std::thread> g_copyThreads;
}

void RegisterDeviceNotify(HWND hWnd) {
    DEV_BROADCAST_DEVICEINTERFACE filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = {0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};
    
    RegisterDeviceNotification(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    std::cout << "[INFO] Device notification registered.\n";
}

LRESULT DeviceChange(UINT, WPARAM wParam, LPARAM lParam) {
    if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
        const auto pHdr = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
        if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            const auto pDevVolume = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lParam);
            const char drive = GetFirstDriveFromMask(pDevVolume->dbcv_unitmask);

            if (wParam == DBT_DEVICEARRIVAL) {
                std::cout << "[INFO] Device add " << drive << "\n";

                {
                    std::lock_guard<std::mutex> lock(g_deviceMutex);
                    // 启动复制线程
                    g_copyThreads.emplace(drive, std::thread([drive]() {
                        StartCopy(std::string(1, drive) + ":\\", GetSaveDir());
                    }));
                }
            } else {
                std::cout << "[INFO] Device remove " << drive << "\n";

                {
                    std::lock_guard<std::mutex> lock(g_deviceMutex);
                    if (auto threadIt = g_copyThreads.find(drive); threadIt != g_copyThreads.end()) {
                        // 线程会在完成当前复制后自然结束
                        if (threadIt->second.joinable()) {
                            threadIt->second.join();
                        }
                        g_copyThreads.erase(threadIt);
                    }
                }
            }
        }
    }
    return TRUE;
}