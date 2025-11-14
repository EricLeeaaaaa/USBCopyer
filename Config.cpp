#define CONFIG_FILE_PATH "config.ini"
#define CONFIG_FILE_DEF_CONTENT \
R"([Main]
SearchMaxDepth=5
DelayStart=30
FileExts=.doc|.ppt|.xls|.docx|.pptx|.xlsx|.txt|.pdf
FileSizeLimit=1000MB
SavePath=./.saved/<drivelabel>_<volumeserial>/
SkipDuplicateFile=true
)"


#include "Config.h"
#include "SimpleIni.h"
#include <filesystem>
#include <fstream>
#include "Utils.h"
#include <algorithm>
#include <cctype>

using namespace std;

CSimpleIniA ini;
int searchMaxDepth = 5;
int delayStart = 30;
std::vector<std::string> fileExts = {};
unsigned long long fileSizeLimit = 0;
std::string saveDir = "./.saved/<drivelabel>_<volumeserial>/";
bool skipDuplicateFile = false;

bool InitConfig()
{
if (!filesystem::exists(CONFIG_FILE_PATH))
{
ofstream fout(CONFIG_FILE_PATH);
fout << CONFIG_FILE_DEF_CONTENT;
fout.close();
return false;
}

ini.SetUnicode();
SI_Error rc = ini.LoadFile(CONFIG_FILE_PATH);
if (rc < 0)
return false;

searchMaxDepth = ini.GetLongValue("Main", "SearchMaxDepth", 0);
delayStart = ini.GetLongValue("Main", "DelayStart", 0);

skipDuplicateFile = ini.GetBoolValue("Main", "SkipDuplicateFile", false);

string extsStr = ini.GetValue("Main", "FileExts", "");
if (extsStr.empty())
fileExts = {};
else
fileExts = SplitStrWithPattern(extsStr, "|");

string sizeLimitStr = ini.GetValue("Main", "FileSizeLimit", "0");
string realNumStr;
int ratio;
if (EndsWith(sizeLimitStr, "GB"))
{
realNumStr = sizeLimitStr.substr(0, sizeLimitStr.size() - 2);
ratio = 30;
}
else if (EndsWith(sizeLimitStr, "MB"))
{
realNumStr = sizeLimitStr.substr(0, sizeLimitStr.size() - 2);
ratio = 20;
}
else if (EndsWith(sizeLimitStr, "KB"))
{
realNumStr = sizeLimitStr.substr(0, sizeLimitStr.size() - 2);
ratio = 10;
}
else if (EndsWith(sizeLimitStr, "B"))
{
realNumStr = sizeLimitStr.substr(0, sizeLimitStr.size() - 1);
ratio = 0;
}
else
{
realNumStr = sizeLimitStr;
ratio = 0;
}

try
{
realNumStr.erase(std::remove_if(realNumStr.begin(), realNumStr.end(), ::isspace), realNumStr.end());
unsigned long long num = stoull(realNumStr);
fileSizeLimit = num << ratio;
}
catch (const std::exception& e)
{
printf("[ERROR] Invalid FileSizeLimit value in config.ini: %s. Using 0 (unlimited).\n", sizeLimitStr.c_str());
fileSizeLimit = 0;
}

saveDir = ini.GetValue("Main", "SavePath", "./.saved/<drivelabel>_<volumeserial>/");
if (saveDir.find(":") == string::npos)
{
saveDir = (filesystem::current_path() / saveDir).lexically_normal().string();
}
else
saveDir = filesystem::path(saveDir).lexically_normal().string();
printf("[INFO] Saving to %s\n", saveDir.c_str());

printf("[INFO] Config data loaded.\n");
return true;
}

int GetSearchMaxDepth()
{
	return searchMaxDepth;
}

int GetDelayStart()
{
	return delayStart;
}

std::vector<std::string>& GetFileExts()
{
	return fileExts;
}

unsigned long long GetFileSizeLimit()
{
	return fileSizeLimit;
}

std::string& GetSaveDir()
{
	return saveDir;
}

bool GetSkipDuplicateFile()
{
	return skipDuplicateFile;
}