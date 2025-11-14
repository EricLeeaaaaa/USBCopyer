#include "DeviceHandler.h"
#include <iostream>
#include <windows.h>
#include <Dbt.h>
#include "Utils.h"
#include "Copyer.h"
#include "Config.h"
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <mutex>
using namespace std;

static const GUID GUID_DEVINTERFACE_USB_DEVICE = { 0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED} };

std::mutex g_deviceMapMutex;
unordered_map<char, HANDLE> copyerThreadMap;
unordered_map<char, bool*> exitSignalMap;

void RegisterDeviceNotify(HWND hWnd)
{
    HDEVNOTIFY hDevNotify;
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
    hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    printf("[INFO] Device notification registered.\n");
}

void AddDevice(char drive)
{
    std::lock_guard<std::mutex> lock(g_deviceMapMutex);
    
    bool* exitSign = new bool(false);
    exitSignalMap[drive] = exitSign;

    CopyInfoStruct* copyInfo = new CopyInfoStruct;
    copyInfo->fromDir = string(1, drive) + ":\\";
    copyInfo->targetDir = GetSaveDir();
    copyInfo->exitSign = exitSign;

    unsigned int threadId;
    HANDLE copyerThread = (HANDLE)_beginthreadex(NULL, 0, StartCopy, (void*)copyInfo, 0, &threadId);
    if (copyerThread != NULL)
    {
        copyerThreadMap[drive] = copyerThread;
    }
    else
        printf("[ERROR] Fail to start copy thread on %c:\\\\ \n", drive);
}

void RemoveDevice(char drive)
{
    std::lock_guard<std::mutex> lock(g_deviceMapMutex);

    auto iter = copyerThreadMap.find(drive);
    if (iter != copyerThreadMap.end())
    {
        bool* exitSignal = exitSignalMap[drive];
        *exitSignal = true;

        DWORD exitCode;
        HANDLE hThread = iter->second;
        GetExitCodeThread(hThread, &exitCode);
        if (exitCode == STILL_ACTIVE)
        {
            printf("[WARN] Copy thread is still working, waiting for it to finish gracefully...\n");
            DWORD waitRes = WaitForSingleObject(hThread, 5000);
            if (waitRes == WAIT_TIMEOUT)
            {
                printf("[ERROR] Thread did not exit gracefully after 5 seconds. It may exit later on its own.\n");
                CloseHandle(hThread);
            }
            else
            {
                printf("finished\n");
                CloseHandle(hThread);
            }
        }
        else
        {
             CloseHandle(hThread);
        }

        delete exitSignal;
        copyerThreadMap.erase(drive);
        exitSignalMap.erase(drive);
    }
}

LRESULT DeviceChange(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEQUERYREMOVE || wParam == DBT_DEVICEREMOVECOMPLETE)
    {
        PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
        if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
        {
            PDEV_BROADCAST_VOLUME pDevVolume = (PDEV_BROADCAST_VOLUME)lParam;
            char drive = GetFirstDriveFromMask(pDevVolume->dbcv_unitmask);

            if (wParam == DBT_DEVICEARRIVAL)
            {
                printf("[INFO] Device add %c\n", drive);
                AddDevice(drive);
            }
            else
            {
                printf("[INFO] Device remove %c\n", drive);
                RemoveDevice(drive);
            }
        }
    }
    return 0;
}