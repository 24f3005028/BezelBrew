#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "../src/connectors.hpp"
int main(){wchar_t root[MAX_PATH],result[MAX_PATH],path[MAX_PATH];GetEnvironmentVariable(L"ProgramFiles",root,MAX_PATH);swprintf_s(path,L"%s\\KDE Connect\\bin\\kdeconnect-cli.exe",root);wprintf(L"root=%ls path=%ls attr=%lu length=%llu fixtureExists=%d detect=%d\n",root,path,GetFileAttributes(path),(unsigned long long)wcslen(path),Connectors::Setup::executable(path),Connectors::Setup::detect(result));return 0;}
