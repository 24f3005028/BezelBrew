#pragma once
#include <tlhelp32.h>

namespace BackgroundApps {
enum class App { None, WhatsApp, Discord, Telegram, Slack, CurseForge, Chrome, Edge };
constexpr const wchar_t* names[]={L"",L"WhatsApp",L"Discord",L"Telegram",L"Slack",L"CurseForge",
                                 L"Google Chrome",L"Microsoft Edge"};
struct Candidate{HANDLE process;DWORD pid;App app;};
struct InstallRoots{wchar_t local[MAX_PATH],roaming[MAX_PATH],programs[MAX_PATH],programs32[MAX_PATH];};
static bool browser(App app){return app==App::Chrome||app==App::Edge;}

static bool sameUser(HANDLE process,PSID user,bool* elevated=nullptr){
    HANDLE token=nullptr;
    if(!OpenProcessToken(process,TOKEN_QUERY,&token))return false;
    alignas(TOKEN_USER) BYTE storage[256];
    TOKEN_ELEVATION elevation={};
    DWORD used=0;
    bool same=GetTokenInformation(token,TokenUser,storage,sizeof(storage),&used)&&
        EqualSid(user,((TOKEN_USER*)storage)->User.Sid)&&
        GetTokenInformation(token,TokenElevation,&elevation,sizeof(elevation),&used);
    CloseHandle(token);
    if(elevated)*elevated=elevation.TokenIsElevated!=0;
    return same&&(elevated||!elevation.TokenIsElevated);
}

enum class WindowApp { Explorer, TaskManager, Codex, Count };
constexpr const wchar_t* windowNames[]={L"File Explorer windows",L"Task Manager",L"Codex"};
static_assert(ARRAYSIZE(windowNames)==(int)WindowApp::Count);
struct WindowReview{
    wchar_t explorer[MAX_PATH],taskManager[MAX_PATH];
    alignas(TOKEN_USER) BYTE user[256];
    DWORD session;
    bool available[ARRAYSIZE(windowNames)],blockedTaskManager;
    const bool* approved;
    int requested;
};

static BOOL CALLBACK reviewWindow(HWND window,LPARAM data){
    auto& review=*(WindowReview*)data;
    if(!IsWindowVisible(window)||GetWindow(window,GW_OWNER))return TRUE;
    wchar_t cls[72];
    if(!GetClassName(window,cls,ARRAYSIZE(cls)))return TRUE;
    WindowApp app;
    if(!_wcsicmp(cls,L"CabinetWClass")||!_wcsicmp(cls,L"ExploreWClass"))app=WindowApp::Explorer;
    else if(!_wcsicmp(cls,L"TaskManagerWindow"))app=WindowApp::TaskManager;
    else if(!_wcsicmp(cls,L"Chrome_WidgetWin_1")&&GetWindowTextLength(window))app=WindowApp::Codex;
    else return TRUE;
    int index=(int)app;
    if(review.approved&&!review.approved[index])return TRUE;

    DWORD pid=0,session=0;
    GetWindowThreadProcessId(window,&pid);
    if(!ProcessIdToSessionId(pid,&session)||session!=review.session)return TRUE;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return TRUE;
    wchar_t path[MAX_PATH];
    DWORD length=ARRAYSIZE(path);
    bool genuine=QueryFullProcessImageName(process,0,path,&length)!=FALSE;
    if(genuine&&app==WindowApp::Codex){
        wchar_t family[128];
        UINT32 size=ARRAYSIZE(family);
        const wchar_t* exe=wcsrchr(path,L'\\');
        genuine=exe&&(!_wcsicmp(exe+1,L"ChatGPT.exe")||!_wcsicmp(exe+1,L"Codex.exe"))&&
            GetPackageFamilyName(process,&size,family)==ERROR_SUCCESS&&!_wcsicmp(family,L"OpenAI.Codex_2p2nqsd0c76g0");
    }else if(genuine){
        genuine=!_wcsicmp(path,app==WindowApp::Explorer?review.explorer:review.taskManager);
    }
    bool elevated=false;
    bool accessible=genuine&&sameUser(process,((TOKEN_USER*)review.user)->User.Sid,&elevated);
    CloseHandle(process);
    if(genuine&&app==WindowApp::TaskManager&&(!accessible||elevated))review.blockedTaskManager=true;
    if(!accessible||elevated)return TRUE;
    if(!review.approved)review.available[index]=true;
    else if(PostMessage(window,WM_CLOSE,0,0))review.requested++;
    return TRUE;
}

static bool reviewWindows(WindowReview& review){
    if(!ProcessIdToSessionId(GetCurrentProcessId(),&review.session)||!review.session)return false;
    HANDLE token=nullptr;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
    DWORD used=0;
    bool ready=GetTokenInformation(token,TokenUser,review.user,sizeof(review.user),&used)!=FALSE;
    CloseHandle(token);
    if(!ready)return false;
    if(GetWindowsDirectory(review.explorer,ARRAYSIZE(review.explorer))<ARRAYSIZE(review.explorer))
        wcscat_s(review.explorer,L"\\explorer.exe");
    if(GetSystemDirectory(review.taskManager,ARRAYSIZE(review.taskManager))<ARRAYSIZE(review.taskManager))
        wcscat_s(review.taskManager,L"\\Taskmgr.exe");
    EnumWindows(reviewWindow,(LPARAM)&review);
    return true;
}
static bool installedAs(const wchar_t* path,const wchar_t* root,const wchar_t* subpath,
                        const wchar_t* executable,bool versioned){
    size_t prefix=wcslen(root);
    if(!prefix||_wcsnicmp(path,root,prefix)||path[prefix]!=L'\\')return false;
    path+=prefix+1;
    size_t folder=wcslen(subpath);
    if(_wcsnicmp(path,subpath,folder)||path[folder]!=L'\\')return false;
    path+=folder+1;
    if(!versioned)return !_wcsicmp(path,executable);
    if(_wcsnicmp(path,L"app-",4))return false;
    const wchar_t* end=wcschr(path,L'\\');
    return end&&!_wcsicmp(end+1,executable);
}
static App identify(HANDLE process,const InstallRoots& roots){
    wchar_t path[MAX_PATH];
    DWORD length=ARRAYSIZE(path);
    if(!QueryFullProcessImageName(process,0,path,&length))return App::None;
    const wchar_t* name=wcsrchr(path,L'\\');
    if(!name)return App::None;
    ++name;
    if(!_wcsicmp(name,L"WhatsApp.exe")||!_wcsicmp(name,L"WhatsApp.Root.exe")){
        wchar_t family[128];
        UINT32 size=ARRAYSIZE(family);
        if(GetPackageFamilyName(process,&size,family)==ERROR_SUCCESS&&
           !_wcsicmp(family,L"5319275A.WhatsAppDesktop_cv1g1gvanyjgm"))return App::WhatsApp;
    }
    if(installedAs(path,roots.local,L"Discord",L"Discord.exe",true))return App::Discord;
    if(installedAs(path,roots.roaming,L"Telegram Desktop",L"Telegram.exe",false))return App::Telegram;
    if(installedAs(path,roots.local,L"slack",L"slack.exe",true))return App::Slack;
    if(installedAs(path,roots.local,L"Programs\\CurseForge Windows",L"CurseForge.exe",false))return App::CurseForge;
    const wchar_t* browserRoots[]={roots.local,roots.programs,roots.programs32};
    for(const auto* root:browserRoots){
        if(installedAs(path,root,L"Google\\Chrome\\Application",L"chrome.exe",false))return App::Chrome;
        if(installedAs(path,root,L"Microsoft\\Edge\\Application",L"msedge.exe",false))return App::Edge;
    }
    return App::None;
}
struct WindowCheck {DWORD pid;bool protectMinimized,found=false;};
static BOOL CALLBACK childForProcess(HWND window,LPARAM data){
    auto& check=*(WindowCheck*)data;
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    if(pid==check.pid&&IsWindowVisible(window)){check.found=true;return FALSE;}
    return TRUE;
}
static BOOL CALLBACK visibleProcess(HWND window,LPARAM data){
    auto& check=*(WindowCheck*)data;
    if(!IsWindowVisible(window)||(!check.protectMinimized&&IsIconic(window)))return TRUE;
    DWORD cloaked=0;
    if(!check.protectMinimized&&SUCCEEDED(DwmGetWindowAttribute(window,DWMWA_CLOAKED,&cloaked,sizeof(cloaked)))){
        if(cloaked==DWM_CLOAKED_APP)return TRUE;
    }
    if(!childForProcess(window,data))return FALSE;
    EnumChildWindows(window,childForProcess,data);
    return !check.found;
}
static bool hasOpenWindow(const Candidate& app){
    DWORD foreground=0;
    GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    if(app.pid==foreground)return true;
    WindowCheck check={app.pid,browser(app.app)};
    EnumWindows(visibleProcess,(LPARAM)&check);
    return check.found;
}
static void release(Candidate* apps,int count){
    for(int i=0;i<count;i++)CloseHandle(apps[i].process);
    if(apps)HeapFree(GetProcessHeap(),0,apps);
}
static int collect(Candidate*& apps){
    InstallRoots roots={};
    SHGetFolderPath(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,roots.local);
    SHGetFolderPath(nullptr,CSIDL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,roots.roaming);
    SHGetFolderPath(nullptr,CSIDL_PROGRAM_FILES,nullptr,SHGFP_TYPE_CURRENT,roots.programs);
    SHGetFolderPath(nullptr,CSIDL_PROGRAM_FILESX86,nullptr,SHGFP_TYPE_CURRENT,roots.programs32);
    DWORD current=GetCurrentProcessId(),session=0;
    if(!ProcessIdToSessionId(current,&session)||!session)return 0;
    HANDLE token=nullptr;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return 0;
    alignas(TOKEN_USER) BYTE storage[256];
    DWORD used=0;
    bool ready=GetTokenInformation(token,TokenUser,storage,sizeof(storage),&used)!=FALSE;
    CloseHandle(token);
    if(!ready)return 0;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE)return 0;
    int count=0,capacity=0;
    PROCESSENTRY32 entry={sizeof(entry)};
    for(BOOL next=Process32First(snapshot,&entry);next;next=Process32Next(snapshot,&entry)){
        DWORD pid=entry.th32ProcessID,otherSession=0;
        if(pid==current||!ProcessIdToSessionId(pid,&otherSession)||otherSession!=session)continue;
        const wchar_t* exe=entry.szExeFile;
        if(_wcsicmp(exe,L"WhatsApp.exe")&&_wcsicmp(exe,L"WhatsApp.Root.exe")&&
           _wcsicmp(exe,L"Discord.exe")&&_wcsicmp(exe,L"Telegram.exe")&&
           _wcsicmp(exe,L"slack.exe")&&_wcsicmp(exe,L"CurseForge.exe")&&
           _wcsicmp(exe,L"chrome.exe")&&_wcsicmp(exe,L"msedge.exe"))continue;
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_TERMINATE|SYNCHRONIZE,FALSE,pid);
        if(!process)continue;
        App app=identify(process,roots);
        if(app==App::None||!sameUser(process,((TOKEN_USER*)storage)->User.Sid)){CloseHandle(process);continue;}
        if(count==capacity){
            capacity+=64;
            HANDLE heap=GetProcessHeap();
            auto* buffer=(Candidate*)(apps?HeapReAlloc(heap,0,apps,capacity*sizeof(Candidate)):
                                          HeapAlloc(heap,0,capacity*sizeof(Candidate)));
            if(!buffer){
                CloseHandle(process);
                CloseHandle(snapshot);
                release(apps,count);
                apps=nullptr;
                return 0;
            }
            apps=buffer;
        }
        apps[count++]={process,pid,app};
    }
    CloseHandle(snapshot);
    return count;
}
static void findOpenApps(const Candidate* apps,int count,bool (&open)[ARRAYSIZE(names)]){
    for(int i=0;i<count;i++){
        auto& app=apps[i];
        int name=(int)app.app;
        if(!open[name]&&WaitForSingleObject(app.process,0)==WAIT_TIMEOUT&&hasOpenWindow(app))
            open[name]=true;
    }
}
static BOOL CALLBACK closeBrowserWindow(HWND window,LPARAM data){
    if(!IsWindowVisible(window)||GetWindow(window,GW_OWNER)||!GetWindowTextLength(window))return TRUE;
    DWORD pid=0;
    GetWindowThreadProcessId(window,&pid);
    if(pid==((const Candidate*)data)->pid)PostMessage(window,WM_CLOSE,0,0);
    return TRUE;
}
static void closeBrowserWindows(const Candidate* apps,int count,const bool (&listed)[ARRAYSIZE(names)]){
    bool pending=false;
    bool requested[ARRAYSIZE(names)]={};
    for(int i=0;i<count;i++){
        const auto& app=apps[i];
        int name=(int)app.app;
        if(browser(app.app)&&listed[name]&&hasOpenWindow(app)){
            EnumWindows(closeBrowserWindow,(LPARAM)&app);
            pending=requested[name]=true;
        }
    }
    if(!pending)return;
    ULONGLONG deadline=GetTickCount64()+1500;
    do {
        Sleep(75);
        pending=false;
        for(int i=0;i<count;i++){
            const auto& app=apps[i];
            if(requested[(int)app.app]&&WaitForSingleObject(app.process,0)==WAIT_TIMEOUT){
                pending=true;
                break;
            }
        }
    }while(pending&&GetTickCount64()<deadline);
}
static void clear(HWND owner,wchar_t (&result)[150]){
    WindowReview windows={};
    reviewWindows(windows);
    constexpr int codex=(int)WindowApp::Codex;
    Candidate* apps=nullptr;
    int count=collect(apps);
    bool open[ARRAYSIZE(names)]={};
    findOpenApps(apps,count,open);
    bool listed[ARRAYSIZE(names)]={};
    wchar_t details[768]=L"Close these apps? Notifications and calls may stop.\n\n";
    bool eligible=false,closingBrowserWindows=false;
    for(int i=0;i<count;i++){
        int name=(int)apps[i].app;
        if((!open[name]||browser(apps[i].app))&&!listed[name]){
            listed[name]=true;
            eligible=true;
            closingBrowserWindows=closingBrowserWindows||(browser(apps[i].app)&&open[name]);
            wcscat_s(details,names[name]);
            wcscat_s(details,L"\n");
        }
    }
    for(int i=0;i<(int)ARRAYSIZE(windowNames);i++)if(i!=codex&&windows.available[i]){
        eligible=true;
        wcscat_s(details,windowNames[i]);
        wcscat_s(details,L"\n");
    }
    constexpr const wchar_t* taskManagerNote=L"Task Manager is running as administrator. Windows requires it to be closed from its own window.";
    if(!eligible&&!windows.available[codex]){
        release(apps,count);
        if(!count&&!windows.blockedTaskManager){
            wcscpy_s(result,L"No recognized background apps are running.");
            return;
        }
        wchar_t openNames[512]={};
        if(count)wcscpy_s(openNames,L"Minimize these apps to include them in Clear memory.\n\n");
        for(int name=1;name<(int)ARRAYSIZE(names);name++)if(open[name]){
            wcscat_s(openNames,names[name]);
            wcscat_s(openNames,L"\n");
        }
        if(windows.blockedTaskManager){
            if(count)wcscat_s(openNames,L"\n");
            wcscat_s(openNames,taskManagerNote);
        }
        MessageBox(owner,openNames,L"Clear memory",MB_OK|MB_ICONINFORMATION);
        return;
    }
    if(closingBrowserWindows)wcscat_s(details,L"\nOpen browser windows and their tabs will close.\n");
    bool first=true;
    for(int name=1;name<(int)ARRAYSIZE(names);name++)if(open[name]&&!listed[name]){
        if(first)wcscat_s(details,L"\nKeeping open:\n");
        first=false;
        wcscat_s(details,names[name]);
        wcscat_s(details,L"\n");
    }
    if(windows.blockedTaskManager){
        wcscat_s(details,L"\n");
        wcscat_s(details,taskManagerNote);
    }
    BOOL includeCodex=FALSE;
    int choice=IDCANCEL;
    if(windows.available[codex]){
        TASKDIALOGCONFIG dialog={sizeof(dialog)};
        dialog.hwndParent=owner;
        dialog.dwFlags=TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW;
        dialog.dwCommonButtons=TDCBF_OK_BUTTON|TDCBF_CANCEL_BUTTON;
        dialog.pszWindowTitle=L"Close apps?";
        dialog.pszContent=details;
        dialog.pszVerificationText=L"Include Codex (this can interrupt running work)";
        dialog.nDefaultButton=IDCANCEL;
        TaskDialogIndirect(&dialog,&choice,nullptr,&includeCodex);
    }else choice=MessageBox(owner,details,L"Close apps?",MB_OKCANCEL|MB_ICONQUESTION|MB_DEFBUTTON2);
    bool confirmed=choice==IDOK;
    if(!confirmed){release(apps,count);return;}
    closeBrowserWindows(apps,count,listed);
    open[(int)App::Chrome]=open[(int)App::Edge]=false;
    findOpenApps(apps,count,open);
    bool closed[ARRAYSIZE(names)]={};
    for(int i=0;i<count;i++){
        const auto& app=apps[i];
        int name=(int)app.app;
        if(!listed[name]||open[name])continue;
        DWORD state=WaitForSingleObject(app.process,0);
        if((browser(app.app)&&state==WAIT_OBJECT_0)||
           (state==WAIT_TIMEOUT&&!hasOpenWindow(app)&&TerminateProcess(app.process,0)))closed[name]=true;
    }
    release(apps,count);
    windows.available[codex]=includeCodex!=FALSE;
    windows.approved=windows.available;
    EnumWindows(reviewWindow,(LPARAM)&windows);
    int total=0;
    for(bool didClose:closed)if(didClose)total++;
    if(open[(int)App::Chrome]||open[(int)App::Edge])wcscpy_s(result,L"Some browser windows are still open. Check for a prompt.");
    else if(total&&windows.requested)swprintf_s(result,L"Closed %d apps. Sent %d window close requests.",total,windows.requested);
    else if(total)swprintf_s(result,L"Closed %d app%s.",total,total==1?L"":L"s");
    else if(windows.requested)swprintf_s(result,L"Sent close requests to %d window%s.",windows.requested,windows.requested==1?L"":L"s");
    else wcscpy_s(result,L"No apps were closed.");
}
}
