#pragma once

struct ThemeLook {
    int glass,background,text,accent,tint,blur,strength,size,textSize,corner,animations,docked;
};
struct SavedTheme {
    wchar_t name[64];
    ThemeLook look;
};
constexpr int savedThemeCount=12;

namespace SavedThemes {
static bool validName(const wchar_t* name){
    size_t length=wcslen(name);
    if(!length||length>=ARRAYSIZE(SavedTheme{}.name)||name[0]<=L' '||name[length-1]<=L' ')return false;
    for(const wchar_t* c=name;*c;c++)if(*c<32||*c==127)return false;
    return true;
}
static bool decode(const wchar_t* text,ThemeLook& look){
    constexpr int low[]={0,0,0,0,0,0,0,80,85,8,0,0};
    constexpr int high[]={1,0xffffff,0xffffff,0xffffff,0xffffff,4,70,160,130,40,1,1};
    int values[ARRAYSIZE(low)];
    for(int i=0;i<(int)ARRAYSIZE(values);i++){
        while(*text==L' ')text++;
        if(*text<L'0'||*text>L'9')return false;
        int value=0;
        while(*text>=L'0'&&*text<=L'9'){
            value=value*10+(*text++-L'0');
            if(value>high[i])return false;
        }
        if(value<low[i]||(*text&&*text!=L' '))return false;
        values[i]=value;
    }
    while(*text==L' ')text++;
    if(*text)return false;
    look={values[0],values[1],values[2],values[3],values[4],values[5],
          values[6],values[7],values[8],values[9],values[10],values[11]};
    return true;
}
static void encode(const ThemeLook& look,wchar_t (&out)[160]){
    swprintf_s(out,L"%d %d %d %d %d %d %d %d %d %d %d %d",
              look.glass,look.background,look.text,look.accent,look.tint,look.blur,
              look.strength,look.size,look.textSize,look.corner,look.animations,look.docked);
}
static void loadLibrary(const wchar_t* path,SavedTheme (&themes)[savedThemeCount]){
    for(int i=0;i<savedThemeCount;i++){
        wchar_t key[24],name[66],look[160];
        swprintf_s(key,L"Name%d",i+1);
        GetPrivateProfileString(L"SavedThemes",key,L"",name,ARRAYSIZE(name),path);
        swprintf_s(key,L"Look%d",i+1);
        GetPrivateProfileString(L"SavedThemes",key,L"",look,ARRAYSIZE(look),path);
        if(name[0]==L'@'&&validName(name+1)&&decode(look,themes[i].look))wcscpy_s(themes[i].name,name+1);
    }
}
static bool writeLibrary(const wchar_t* path,const SavedTheme (&themes)[savedThemeCount]){
    wchar_t section[savedThemeCount*260+2]={};
    size_t used=0;
    for(int i=0;i<savedThemeCount;i++){
        const auto& theme=themes[i];
        if(!theme.name[0])continue;
        wchar_t look[160];encode(theme.look,look);
        int length=swprintf_s(section+used,ARRAYSIZE(section)-used,L"Name%d=@%s",i+1,theme.name);
        if(length<0)return false;
        used+=length+1;
        length=swprintf_s(section+used,ARRAYSIZE(section)-used,L"Look%d=%s",i+1,look);
        if(length<0)return false;
        used+=length+1;
    }
    return WritePrivateProfileSection(L"SavedThemes",section,path)!=FALSE;
}
static bool exportFile(const wchar_t* path,const SavedTheme& theme){
    if(!validName(theme.name))return false;
    wchar_t look[160],contents[300];
    encode(theme.look,look);
    swprintf_s(contents,L"BezelBrew Theme 1\r\n%s\r\n%s\r\n",theme.name,look);
    char bytes[1200];
    int size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,contents,-1,bytes,ARRAYSIZE(bytes),nullptr,nullptr);
    if(size<=1)return false;
    HANDLE file=CreateFile(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    bool ok=WriteFile(file,bytes,size-1,&written,nullptr)&&written==(DWORD)(size-1);
    CloseHandle(file);
    return ok;
}
static bool importFile(const wchar_t* path,SavedTheme& theme){
    HANDLE file=CreateFile(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    char bytes[1200];
    DWORD read=0;
    bool ok=ReadFile(file,bytes,sizeof(bytes),&read,nullptr)!=FALSE;
    CloseHandle(file);
    if(!ok||!read||read>=sizeof(bytes)||memchr(bytes,0,read))return false;
    int start=read>=3&&(BYTE)bytes[0]==0xef&&(BYTE)bytes[1]==0xbb&&(BYTE)bytes[2]==0xbf?3:0;
    wchar_t contents[300];
    int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes+start,read-start,contents,ARRAYSIZE(contents)-1);
    if(!size)return false;
    contents[size]=0;
    wchar_t* name=wcschr(contents,L'\n');
    if(!name)return false;
    *name++=0;
    if(name-contents>1&&name[-2]==L'\r')name[-2]=0;
    if(wcscmp(contents,L"BezelBrew Theme 1"))return false;
    wchar_t* look=wcschr(name,L'\n');
    if(!look)return false;
    *look++=0;
    if(look-name>1&&look[-2]==L'\r')look[-2]=0;
    size_t length=wcslen(look);
    while(length&&(look[length-1]==L'\n'||look[length-1]==L'\r'))look[--length]=0;
    if(!validName(name)||!decode(look,theme.look))return false;
    wcscpy_s(theme.name,name);
    return true;
}
}
