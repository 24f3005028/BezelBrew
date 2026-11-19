#pragma once

namespace DiscordVoice {
static bool openApp(HWND owner){
    return (INT_PTR)ShellExecute(owner,L"open",L"discord://",nullptr,nullptr,SW_SHOWNORMAL)>32;
}
static BOOL CALLBACK findWindow(HWND window,LPARAM result){
    if(!IsWindowVisible(window)||GetWindow(window,GW_OWNER)||!GetWindowTextLength(window))return TRUE;
    wchar_t cls[32];
    if(!GetClassName(window,cls,ARRAYSIZE(cls))||wcscmp(cls,L"Chrome_WidgetWin_1"))return TRUE;
    DWORD pid=0;
    GetWindowThreadProcessId(window,&pid);
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return TRUE;
    wchar_t path[MAX_PATH];
    DWORD length=ARRAYSIZE(path);
    bool found=QueryFullProcessImageName(process,0,path,&length)!=FALSE;
    CloseHandle(process);
    if(!found)return TRUE;
    const wchar_t* name=wcsrchr(path,L'\\');
    if(!name)return TRUE;
    ++name;
    if(_wcsicmp(name,L"Discord.exe")&&_wcsicmp(name,L"DiscordPTB.exe")&&
       _wcsicmp(name,L"DiscordCanary.exe"))return TRUE;
    *(HWND*)result=window;
    return FALSE;
}
static const wchar_t* toggleMute(){
    HWND discord=nullptr;
    EnumWindows(findWindow,(LPARAM)&discord);
    if(!discord)return L"Open the Discord desktop window first.";
    const int modifiers[]={VK_CONTROL,VK_SHIFT,VK_MENU,VK_LWIN,VK_RWIN,'M'};
    for(int key:modifiers){
        if(GetAsyncKeyState(key)<0)return L"Release the keys and try again.";
    }
    if(IsIconic(discord))ShowWindow(discord,SW_RESTORE);
    SetForegroundWindow(discord);
    if(GetForegroundWindow()!=discord)return L"Could not switch to the Discord window.";

    // Discord handles its own shortcut and remains the source of truth for mute state.
    const WORD keys[]={VK_CONTROL,VK_SHIFT,'M','M',VK_SHIFT,VK_CONTROL};
    INPUT inputs[ARRAYSIZE(keys)]={};
    for(int i=0;i<(int)ARRAYSIZE(keys);i++){
        inputs[i].type=INPUT_KEYBOARD;
        inputs[i].ki.wVk=keys[i];
        if(i>=3)inputs[i].ki.dwFlags=KEYEVENTF_KEYUP;
    }
    UINT sent=SendInput(ARRAYSIZE(inputs),inputs,sizeof(INPUT));
    if(sent==ARRAYSIZE(inputs))return L"Mute shortcut sent. Check Discord for your mute state.";
    if(sent>0)SendInput(ARRAYSIZE(inputs)-3,inputs+3,sizeof(INPUT));
    return L"Windows could not send the Discord mute shortcut.";
}
}
