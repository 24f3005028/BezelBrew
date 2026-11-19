#pragma once
// Included inside Connectors after the bounded CLI runner is defined.
namespace Setup {
using PhoneChoice=Connectors::PhoneChoice;
static SRWLOCK lock=SRWLOCK_INIT;
static Config requested;
static LONG pending=0,generation=0;

static bool executable(const wchar_t* path){
    DWORD attributes=GetFileAttributes(path);size_t n=wcslen(path);
    return n>4&&n<MAX_PATH&&attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY)&&!_wcsicmp(path+n-4,L".exe")&&!wcschr(path,L'"');
}
static bool candidate(const wchar_t* root,const wchar_t* suffix,wchar_t* result){
    if(!root[0]||wcslen(root)+wcslen(suffix)>=MAX_PATH)return false;
    wchar_t path[MAX_PATH];wcscpy_s(path,root);wcscat_s(path,suffix);
    if(!executable(path))return false;wcscpy_s(result,MAX_PATH,path);return true;
}
static bool folder(const wchar_t* root,wchar_t* result){return candidate(root,L"\\kdeconnect-cli.exe",result)||candidate(root,L"\\bin\\kdeconnect-cli.exe",result);}
using Environment=DWORD(WINAPI*)(LPCWSTR,LPWSTR,DWORD);
static bool detect(wchar_t* result,const wchar_t* preferred=L"",Environment environment=GetEnvironmentVariableW){
    result[0]=0;if(executable(preferred)){wcscpy_s(result,MAX_PATH,preferred);return true;}
    const wchar_t* variables[]={L"ProgramFiles",L"ProgramFiles(x86)",L"LOCALAPPDATA"};
    for(auto variable:variables){wchar_t root[MAX_PATH];DWORD n=environment(variable,root,MAX_PATH);if(!n||n>=MAX_PATH)continue;const wchar_t* suffixes[]={L"\\KDE Connect",L"\\KDEConnect",L"\\Programs\\KDE Connect"};
        for(auto suffix:suffixes){if(wcslen(root)+wcslen(suffix)>=MAX_PATH)continue;wchar_t install[MAX_PATH];wcscpy_s(install,root);wcscat_s(install,suffix);if(folder(install,result))return true;}}
    HKEY roots[]={HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE};
    for(auto root:roots){HKEY key;if(RegOpenKeyEx(root,L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\kdeconnect-cli.exe",0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){wchar_t path[MAX_PATH];DWORD bytes=sizeof(path),type=0;bool ok=RegQueryValueEx(key,0,0,&type,(BYTE*)path,&bytes)==ERROR_SUCCESS&&type==REG_SZ&&bytes<sizeof(path);RegCloseKey(key);if(ok){path[bytes/sizeof(wchar_t)]=0;if(executable(path)){wcscpy_s(result,MAX_PATH,path);return true;}}}}
    // Running packaged apps expose their actual installed path without scanning
    // the protected WindowsApps folder or asking the user to find an executable.
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot!=INVALID_HANDLE_VALUE){PROCESSENTRY32 entry={sizeof(entry)};bool found=false;if(Process32First(snapshot,&entry))do{
        if(_wcsicmp(entry.szExeFile,L"kdeconnect-app.exe")&&_wcsicmp(entry.szExeFile,L"kdeconnect-indicator.exe")&&_wcsicmp(entry.szExeFile,L"kdeconnectd.exe"))continue;
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!process)continue;wchar_t path[MAX_PATH];DWORD size=MAX_PATH;bool ok=QueryFullProcessImageName(process,0,path,&size)!=FALSE;CloseHandle(process);if(ok){wchar_t* end=wcsrchr(path,L'\\');if(end){*end=0;if(folder(path,result)){found=true;break;}}}
    }while(Process32Next(snapshot,&entry));CloseHandle(snapshot);if(found)return true;}
    // Both standard and per-user offline installers can record InstallLocation.
    REGSAM registryViews[]={KEY_WOW64_64KEY,KEY_WOW64_32KEY};
    for(auto root:roots)for(REGSAM view:registryViews){
        HKEY uninstall;if(RegOpenKeyEx(root,L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",0,KEY_ENUMERATE_SUB_KEYS|view,&uninstall)!=ERROR_SUCCESS)continue;
        for(DWORD i=0;i<2048;i++){wchar_t name[256];DWORD count=256;if(RegEnumKeyEx(uninstall,i,name,&count,0,0,0,0)!=ERROR_SUCCESS)break;HKEY key;if(RegOpenKeyEx(uninstall,name,0,KEY_QUERY_VALUE|view,&key)!=ERROR_SUCCESS)continue;
            wchar_t title[256]={},location[MAX_PATH]={};DWORD type=0,bytes=sizeof(title)-2;bool match=RegQueryValueEx(key,L"DisplayName",0,&type,(BYTE*)title,&bytes)==ERROR_SUCCESS&&type==REG_SZ&&!_wcsnicmp(title,L"KDE Connect",11);
            bytes=sizeof(location)-2;bool has=match&&RegQueryValueEx(key,L"InstallLocation",0,&type,(BYTE*)location,&bytes)==ERROR_SUCCESS&&type==REG_SZ;RegCloseKey(key);
            if(has&&folder(location,result)){RegCloseKey(uninstall);return true;}}
        RegCloseKey(uninstall);
    }
    return false;
}
static bool companion(const wchar_t* cli,wchar_t* result){
    result[0]=0;wchar_t directory[MAX_PATH];wcsncpy_s(directory,MAX_PATH,cli,_TRUNCATE);wchar_t* end=wcsrchr(directory,L'\\');if(!end)return false;*end=0;
    return candidate(directory,L"\\kdeconnect-app.exe",result)||candidate(directory,L"\\kdeconnect-indicator.exe",result);
}
static int choices(const char* output,PhoneChoice* phones,int capacity){
    int count=0;for(const char* line=output;*line&&count<capacity;){const char* end=strchr(line,'\n');if(!end)end=line+strlen(line);const char* space=(const char*)memchr(line,' ',end-line);
        if(space&&space-line>=8&&space-line<128){char id[128],name[640];int length=(int)(space-line);memcpy(id,line,length);id[length]=0;ConnectorNet::wide(id,phones[count].id,128);
            while(space<end&&*space==' ')space++;while(end>space&&(end[-1]=='\r'||end[-1]==' '))end--;int n=(int)(end-space);if(n>639)n=639;memcpy(name,space,n);name[n]=0;ConnectorNet::wide(name,phones[count].name,160);
            bool duplicate=false;for(int i=0;i<count;i++)if(!wcscmp(phones[i].id,phones[count].id))duplicate=true;
            if(deviceId(phones[count].id)&&phones[count].name[0]&&!duplicate)count++;
        }
        const char* next=strchr(line,'\n');line=next?next+1:line+strlen(line);
    }return count;
}
static void scan(const Config& value){
    AcquireSRWLockExclusive(&lock);requested=value;InterlockedIncrement(&generation);InterlockedExchange(&pending,1);ReleaseSRWLockExclusive(&lock);
    AcquireSRWLockExclusive(&viewLock);view.setupBusy=true;wcscpy_s(view.setupStatus,L"Looking for your paired phone…");ReleaseSRWLockExclusive(&viewLock);publish();if(wake)SetEvent(wake);
}
static void run(){
    if(!InterlockedExchange(&pending,0))return;AcquireSRWLockShared(&lock);Config value=requested;LONG revision=generation;ReleaseSRWLockShared(&lock);
    wchar_t cli[MAX_PATH]={},message[320];PhoneChoice phones[16]={};int count=0;char output[32768]={};
    if(!detect(cli,value.kdePath))wcscpy_s(message,L"Install KDE Connect on this PC, open it, then choose Find my phone.");
    else if(!runCli(cli,L"--list-available --id-name-only",output,sizeof(output)))wcscpy_s(message,L"Open KDE Connect on this PC and your phone. If it is already open, update it and try Find my phone again.");
    else{count=choices(output,phones,16);if(count)swprintf_s(message,L"%d paired device(s) found. Choose your phone, then select Connect this phone.",count);else wcscpy_s(message,L"No paired phone found. Use the same Wi-Fi, open KDE Connect on both devices, and confirm pairing. Then try again.");}
    AcquireSRWLockShared(&lock);bool current=revision==generation;
    if(current){AcquireSRWLockExclusive(&viewLock);wcscpy_s(view.detectedCli,cli);wcscpy_s(view.setupStatus,message);view.deviceCount=count;memcpy(view.devices,phones,sizeof(phones));view.setupBusy=false;view.deviceRevision++;ReleaseSRWLockExclusive(&viewLock);}ReleaseSRWLockShared(&lock);
    SecureZeroMemory(output,sizeof(output));if(current)publish();
}
}
