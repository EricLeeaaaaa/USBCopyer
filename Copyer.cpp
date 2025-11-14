#include "Copyer.h"
#include <iostream>
#include <string>
#include <filesystem>
#include "Config.h"
#include "Utils.h"
using namespace std;

void CopyRecursively(const filesystem::path& currentDir, const filesystem::path& targetDir, int depth, bool* exitSign)
{
const int maxDepth = GetSearchMaxDepth();
const auto& fileExts = GetFileExts();
const unsigned long long fileSizeLimit = GetFileSizeLimit();

if (*exitSign)
return;

auto iter = filesystem::directory_iterator(currentDir);
for (auto& it : iter)
{
try
{
if (*exitSign)
return;
auto fromPath = it.path();
auto targetPath = targetDir / fromPath.filename();
if (filesystem::is_directory(fromPath))
{
if (maxDepth > 0 && depth >= maxDepth)
continue;

if (*exitSign)
return;
if (!filesystem::create_directories(targetPath))
{
cout << "Fail to create copy target dir " << targetPath.string();
continue;
}
if (*exitSign)
return;

printf("Go into dir %s\n", targetPath.string().c_str());
CopyRecursively(fromPath, targetPath, depth + 1, exitSign);
if (*exitSign)
return;
}
else
{
bool extMatched = false;
if (fileExts.empty())
extMatched = true;
else
{
for (auto& ext : fileExts)
if (EndsWith(fromPath.string(), ext))
{
extMatched = true;
break;
}
}
bool fileSizeOk = (fileSizeLimit == 0 ? true : filesystem::file_size(fromPath) <= fileSizeLimit);

if (*exitSign)
return;
if (extMatched && fileSizeOk)
{
if (GetSkipDuplicateFile() && filesystem::exists(targetPath))
{
try
{
auto from_size = filesystem::file_size(fromPath);
auto target_size = filesystem::file_size(targetPath);
auto from_time = filesystem::last_write_time(fromPath);
auto target_time = filesystem::last_write_time(targetPath);

if (from_size == target_size && from_time == target_time)
{
continue;
}
}
catch(const filesystem::filesystem_error& e)
{
printf("[WARN] Could not compare file attributes for %s: %s\n", fromPath.string().c_str(), e.what());
}
}

printf("Copy %s -> %s\n", fromPath.string().c_str(), targetPath.string().c_str());
try
{
auto from_time = filesystem::last_write_time(fromPath);
filesystem::copy(fromPath, targetPath, filesystem::copy_options::overwrite_existing);
if (*exitSign) return;
filesystem::last_write_time(targetPath, from_time);
}
catch (const filesystem::filesystem_error& e)
{
printf("[ERROR] Failed to copy or set time for %s: %s\n", fromPath.string().c_str(), e.what());
}

if (*exitSign)
return;
}
}
}
catch (const std::exception& e)
{
printf("[ERROR] Exception during copy operation near %s: %s\n", it.path().string().c_str(), e.what());
if (*exitSign)
return;
}
}
}

unsigned int StartCopy(void* info)
{
int delay = GetDelayStart();
if (delay > 0)
{
printf("Sleep %ds before start...\n", delay);
Sleep(delay * 1000);
}

CopyInfoStruct* copyInfo = (CopyInfoStruct*)info;
string fromDir = copyInfo->fromDir;
string targetDir = copyInfo->targetDir;
bool* exitSignal = copyInfo->exitSign;
delete copyInfo;

char driveLetter = fromDir[0];
string label = GetDeviceLabel(driveLetter);
if (label.empty())
label = string("NoLabel_") + driveLetter;

string serial = GetVolumeSerialNumber(driveLetter);
if (serial.empty())
serial = "UnknownSerial";

ReplaceStr(targetDir, "<drivelabel>", label);
ReplaceStr(targetDir, "<volumeserial>", serial);
ReplaceStr(targetDir, "<date>", GetDateString());
ReplaceStr(targetDir, "<time>", GetTimeString());

std::error_code ec;
if (!filesystem::exists(targetDir, ec))
{
filesystem::create_directories(targetDir, ec);
if (ec)
{
printf("[ERROR] Failed to create target directory: %s\n", targetDir.c_str());
return 1;
}
}

printf("[INFO] Start syncing from %s -> %s\n", fromDir.c_str(), targetDir.c_str());
CopyRecursively(fromDir, targetDir, 1, exitSignal);
printf("[INFO] Sync process finished.\n");
return 0;
}