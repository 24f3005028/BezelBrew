#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "../src/saved_themes.hpp"

int wmain(int argc,wchar_t** argv){
    assert(argc==2);
    SavedTheme original={L"Coding \u2013 blue",{1,0x121318,0xf5f5f7,0x70aeff,0x3157db,2,38,100,110,18,1,0}};
    assert(SavedThemes::exportFile(argv[1],original));
    SavedTheme imported={};
    assert(SavedThemes::importFile(argv[1],imported));
    assert(!wcscmp(imported.name,original.name));
    wchar_t before[160],after[160];
    SavedThemes::encode(original.look,before);
    SavedThemes::encode(imported.look,after);
    assert(!wcscmp(before,after));
    const char malformed[]="BezelBrew Theme 1\nInvalid\n1 16777216 0 0 0 1 30 100 100 18 1 0\n";
    HANDLE file=CreateFile(argv[1],GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    assert(file!=INVALID_HANDLE_VALUE);
    DWORD written;
    assert(WriteFile(file,malformed,sizeof(malformed)-1,&written,nullptr));
    CloseHandle(file);
    assert(!SavedThemes::importFile(argv[1],imported));
    assert(DeleteFile(argv[1]));
    puts("Theme file round trip and rejection of invalid fields passed.");
}
