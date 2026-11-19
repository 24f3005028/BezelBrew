#pragma once

constexpr int workstationCount=6, workstationApps=4, workstationTabs=4, webAddressSize=512;
struct AppTarget { wchar_t name[64],path[MAX_PATH]; };
struct Workstation {
    wchar_t name[64];
    AppTarget apps[workstationApps];
    wchar_t tabs[workstationTabs][webAddressSize];
    wchar_t track[23];
};

namespace LaunchTargets {
static bool webAddress(const wchar_t* input,wchar_t (&out)[webAddressSize]){
    while(*input==L' '||*input==L'\t')++input;
    size_t length=wcslen(input);
    while(length&&(input[length-1]==L' '||input[length-1]==L'\t'))--length;
    if(!length||length>=ARRAYSIZE(out))return false;
    for(size_t i=0;i<length;i++){
        wchar_t c=input[i];
        if(c<=L' '||c==L'"'||c==L'\\'||c==127)return false;
    }
    bool hasScheme=wcsstr(input,L"://")!=nullptr;
    if(swprintf_s(out,L"%s%.*s",hasScheme?L"":L"https://",(int)length,input)<0)return false;
    URL_COMPONENTS parts={sizeof(parts)};
    parts.dwHostNameLength=parts.dwUserNameLength=parts.dwPasswordLength=(DWORD)-1;
    return WinHttpCrackUrl(out,0,0,&parts)&&parts.dwHostNameLength&&
           !parts.dwUserNameLength&&!parts.dwPasswordLength&&
           (parts.nScheme==INTERNET_SCHEME_HTTP||parts.nScheme==INTERNET_SCHEME_HTTPS);
}
static bool trackId(const wchar_t* input,wchar_t (&id)[23]){
    const wchar_t* url=L"https://open.spotify.com/track/";
    const wchar_t* uri=L"spotify:track:";
    const wchar_t* value=input;
    if(!_wcsnicmp(input,url,wcslen(url)))value+=wcslen(url);
    else if(!_wcsnicmp(input,uri,wcslen(uri)))value+=wcslen(uri);
    size_t length=wcscspn(value,L"?#");
    if(length!=22)return false;
    for(size_t i=0;i<length;i++){
        wchar_t c=value[i];
        if(!(c>=L'a'&&c<=L'z')&&!(c>=L'A'&&c<=L'Z')&&!(c>=L'0'&&c<=L'9'))return false;
    }
    wcsncpy_s(id,value,length);
    return true;
}
static bool open(HWND owner,const wchar_t* target,const wchar_t* args=nullptr){
    SHELLEXECUTEINFO exec={sizeof(exec)};
    exec.fMask=SEE_MASK_FLAG_NO_UI;
    exec.hwnd=owner;
    exec.lpVerb=L"open";
    exec.lpFile=target;
    exec.lpParameters=args;
    exec.nShow=SW_SHOWNORMAL;
    if(wcsncmp(target,L"shell:AppsFolder\\",17)==0){
        PIDLIST_ABSOLUTE id=nullptr;
        if(SUCCEEDED(SHParseDisplayName(target,nullptr,&id,0,nullptr))){
            SHELLEXECUTEINFO byId=exec;
            byId.fMask|=SEE_MASK_INVOKEIDLIST;
            byId.lpIDList=id;
            byId.lpFile=nullptr;
            BOOL ok=ShellExecuteEx(&byId);
            CoTaskMemFree(id);
            if(ok)return true;
        }
    }
    return ShellExecuteEx(&exec)!=FALSE;
}
static bool chromePath(wchar_t (&path)[MAX_PATH]){
    const HKEY roots[]={HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE};
    for(HKEY root:roots){
        DWORD length=sizeof(path);
        if(RegGetValue(root,L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
                       nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,path,&length)==ERROR_SUCCESS&&
           GetFileAttributes(path)!=INVALID_FILE_ATTRIBUTES)return true;
    }
    const wchar_t* directories[]={L"LOCALAPPDATA",L"ProgramFiles",L"ProgramFiles(x86)"};
    for(const auto* name:directories){
        wchar_t dir[MAX_PATH];
        DWORD length=GetEnvironmentVariable(name,dir,ARRAYSIZE(dir));
        if(length&&length<ARRAYSIZE(dir)&&swprintf_s(path,L"%s\\Google\\Chrome\\Application\\chrome.exe",dir)>0&&
           GetFileAttributes(path)!=INVALID_FILE_ATTRIBUTES)return true;
    }
    return false;
}
struct TabsResult {int failed=0;bool fallback=false;};
static TabsResult openTabs(HWND owner,const wchar_t (&tabs)[workstationTabs][webAddressSize],bool newWindow){
    TabsResult result;
    wchar_t valid[workstationTabs][webAddressSize]={};
    wchar_t args[workstationTabs*(webAddressSize+3)+16]={};
    if(newWindow)wcscpy_s(args,L"--new-window");
    int count=0;
    for(const auto& tab:tabs){
        if(!tab[0])continue;
        if(!webAddress(tab,valid[count])){result.failed++;continue;}
        wcscat_s(args,L" \"");
        wcscat_s(args,valid[count++]);
        wcscat_s(args,L"\"");
    }
    if(!count)return result;
    wchar_t chrome[MAX_PATH];
    if(chromePath(chrome)&&open(owner,chrome,args))return result;
    result.fallback=true;
    for(int i=0;i<count;i++)if(!open(owner,valid[i]))result.failed++;
    return result;
}
static bool shortcut(HWND owner,const wchar_t* target){
    if(!_wcsnicmp(target,L"https://",8)||!_wcsnicmp(target,L"http://",7)){
        wchar_t tabs[workstationTabs][webAddressSize]={};
        if(wcslen(target)>=webAddressSize)return false;
        wcscpy_s(tabs[0],target);
        return openTabs(owner,tabs,false).failed==0;
    }
    return open(owner,target);
}
static bool openTrack(HWND owner,const wchar_t* track){
    wchar_t uri[48],url[80];
    swprintf_s(uri,L"spotify:track:%s",track);
    swprintf_s(url,L"https://open.spotify.com/track/%s",track);
    return open(owner,uri)||open(owner,url);
}
}
