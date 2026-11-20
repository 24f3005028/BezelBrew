#define WIDEN_INNER(x) L##x
#define WIDEN(x) WIDEN_INNER(x)
#pragma once
#include "saved_themes.hpp"
#include "toolbar_layout.hpp"
struct Preferences {
    wchar_t displayName[64];
    int sizePercent,textPercent,accentRgb,cornerDip,blurPasses,tintPercent;
    bool glassMode;
    int backgroundRgb,fontRgb,glassTintRgb;
    int hoverWidth,hoverHeight,hoverDelay,closeDelay,animationMs,alignment,offset,defaultTab;
    bool hoverEnabled,waveformEnabled,swipeEnabled,clipboardEnabled,automaticBreak;
    int historyLimit,focusMinutes,breakMinutes;
    bool gmailEnabled,gmailUnreadOnly,androidEnabled,iphoneEnabled;int gmailRefresh,phoneRefresh;
    wchar_t gmailClientId[256],kdeCliPath[MAX_PATH],androidDeviceId[128];
    Workstation workstations[workstationCount];
    SavedTheme themes[savedThemeCount];
    AppTarget apps[appLimit];
    ToolbarLayout toolbar;
    wchar_t monitor[32],note[2048];
};
static Preferences prefs;
static void defaultPreferences(Preferences& p){ZeroMemory(&p,sizeof(p));p.toolbar=ToolbarLayout::defaults();wcscpy_s(p.displayName,L"BezelBrew");p.gmailUnreadOnly=true;p.gmailRefresh=120;p.phoneRefresh=30;p.sizePercent=p.textPercent=100;p.accentRgb=0xa4aeff;p.glassMode=true;p.backgroundRgb=0x18181f;p.fontRgb=0xf5f5f7;p.glassTintRgb=0x454e64;p.cornerDip=18;p.blurPasses=1;p.tintPercent=28;p.hoverWidth=220;p.hoverHeight=7;p.hoverDelay=60;p.closeDelay=180;p.animationMs=300;p.alignment=1;p.hoverEnabled=p.waveformEnabled=p.swipeEnabled=p.clipboardEnabled=true;p.historyLimit=12;p.focusMinutes=25;p.breakMinutes=5;const wchar_t* names[]={L"File Explorer",L"Windows Settings",L"Calculator",L"Notepad"};const wchar_t* targets[]={L"explorer.exe",L"ms-settings:",L"calc.exe",L"notepad.exe"};for(int i=0;i<4;i++){wcscpy_s(p.apps[i].name,names[i]);wcscpy_s(p.apps[i].path,targets[i]);}}
static void workstationSection(int index,wchar_t (&section)[32]){
    swprintf_s(section,L"Workstation%d",index+1);
}
static void readPreferences(const wchar_t* file,Preferences& p){
    SavedThemes::loadLibrary(file,p.themes);
    p.toolbar.load(file);
    // Migrate existing presets once, until the user saves an explicit appearance.
    int legacy=GetPrivateProfileInt(L"Appearance",L"Theme",3,file);
    p.glassMode=legacy<0||legacy>=3;
    if(legacy==0)p.backgroundRgb=0x0b0b0f;
    if(legacy==1){p.backgroundRgb=0xf2f2f7;p.fontRgb=0x1c1c1e;}
    if(legacy==2)p.backgroundRgb=0x241721;
#define READ(field,section) p.field=GetPrivateProfileInt(section,WIDEN(#field),p.field,file)
    READ(glassMode,L"Appearance");
    int background=p.backgroundRgb,font=p.fontRgb,tint=p.glassTintRgb;
    READ(backgroundRgb,L"Appearance");READ(fontRgb,L"Appearance");READ(glassTintRgb,L"Appearance");
    if((unsigned)p.backgroundRgb>0xffffff)p.backgroundRgb=background;
    if((unsigned)p.fontRgb>0xffffff)p.fontRgb=font;
    if((unsigned)p.glassTintRgb>0xffffff)p.glassTintRgb=tint;
    READ(gmailEnabled,L"Connectors");READ(gmailUnreadOnly,L"Connectors");READ(androidEnabled,L"Connectors");READ(iphoneEnabled,L"Connectors");READ(gmailRefresh,L"Connectors");READ(phoneRefresh,L"Connectors");
    READ(sizePercent,L"Layout");READ(textPercent,L"Layout");READ(accentRgb,L"Appearance");READ(cornerDip,L"Appearance");READ(blurPasses,L"Appearance");READ(tintPercent,L"Appearance");READ(hoverWidth,L"Behavior");READ(hoverHeight,L"Behavior");READ(hoverDelay,L"Behavior");READ(closeDelay,L"Behavior");READ(animationMs,L"Behavior");READ(alignment,L"Layout");READ(offset,L"Layout");READ(defaultTab,L"Behavior");READ(hoverEnabled,L"Behavior");READ(waveformEnabled,L"Behavior");READ(swipeEnabled,L"Behavior");READ(clipboardEnabled,L"Tools");READ(automaticBreak,L"Tools");READ(historyLimit,L"Tools");READ(focusMinutes,L"Tools");READ(breakMinutes,L"Tools");
#undef READ
    GetPrivateProfileString(L"Appearance",L"DisplayName",L"BezelBrew",p.displayName,64,file);if(!p.displayName[0])wcscpy_s(p.displayName,L"BezelBrew");
    GetPrivateProfileString(L"Connectors",L"GmailClientId",L"",p.gmailClientId,256,file);GetPrivateProfileString(L"Connectors",L"KdeCliPath",L"",p.kdeCliPath,MAX_PATH,file);GetPrivateProfileString(L"Connectors",L"AndroidDeviceId",L"",p.androidDeviceId,128,file);
    GetPrivateProfileString(L"Layout",L"Monitor",L"",p.monitor,32,file);
    int usedApps=0;
    for(int i=0;i<appLimit;i++){
        wchar_t key[24];
        auto& app=p.apps[i];
        AppTarget loaded={};
        swprintf_s(key,L"Name%d",i);
        GetPrivateProfileString(L"Launchers",key,app.name,loaded.name,ARRAYSIZE(loaded.name),file);
        swprintf_s(key,L"Target%d",i);
        GetPrivateProfileString(L"Launchers",key,app.path,loaded.path,ARRAYSIZE(loaded.path),file);
        if(loaded.path[0])p.apps[usedApps++]=loaded;
    }
    for(int i=usedApps;i<appLimit;i++)p.apps[i]={};
    for(int i=0;i<workstationCount;i++){
        wchar_t section[32];workstationSection(i,section);
        auto& station=p.workstations[i];
        GetPrivateProfileString(section,L"Name",L"",station.name,ARRAYSIZE(station.name),file);
        if(!station.name[0])continue;
        wchar_t spotify[80];
        GetPrivateProfileString(section,L"SpotifyTrack",L"",spotify,ARRAYSIZE(spotify),file);
        station.spotify[0]=0;
        LaunchTargets::spotifyItem(spotify,station.spotify,true);
        int apps=0,tabs=0;
        for(int j=0;j<workstationApps;j++){
            wchar_t key[24];
            swprintf_s(key,L"App%dName",j+1);
            AppTarget target={};
            GetPrivateProfileString(section,key,L"",target.name,ARRAYSIZE(target.name),file);
            swprintf_s(key,L"App%dTarget",j+1);
            GetPrivateProfileString(section,key,L"",target.path,ARRAYSIZE(target.path),file);
            if(target.path[0])station.apps[apps++]=target;
        }
        for(int j=0;j<workstationTabs;j++){
            wchar_t key[24];swprintf_s(key,L"Tab%d",j+1);
            wchar_t tab[webAddressSize];
            GetPrivateProfileString(section,key,L"",tab,ARRAYSIZE(tab),file);
            if(tab[0])wcscpy_s(station.tabs[tabs++],tab);
        }
        for(int j=apps;j<workstationApps;j++)station.apps[j]={};
        for(int j=tabs;j<workstationTabs;j++)station.tabs[j][0]=0;
    }
    wchar_t encoded[4096];GetPrivateProfileString(L"Notes",L"Text",L"",encoded,4096,file);int at=0;for(int i=0;encoded[i]&&at<2047;i++){if(encoded[i]==L'\\'&&encoded[i+1]){i++;if(encoded[i]==L'n'){p.note[at++]=L'\r';if(at<2047)p.note[at++]=L'\n';}else p.note[at++]=encoded[i];}else p.note[at++]=encoded[i];}p.note[at]=0;
#define LIMIT(field,lo,hi) if(p.field<lo||p.field>hi)p.field=lo
    LIMIT(gmailRefresh,60,900);LIMIT(phoneRefresh,15,300);
    LIMIT(sizePercent,80,160);LIMIT(textPercent,85,130);LIMIT(cornerDip,8,40);LIMIT(blurPasses,0,4);LIMIT(tintPercent,0,70);LIMIT(hoverWidth,80,500);LIMIT(hoverHeight,4,24);LIMIT(hoverDelay,20,1000);LIMIT(closeDelay,100,2000);LIMIT(animationMs,100,600);LIMIT(alignment,0,2);LIMIT(offset,-600,600);LIMIT(defaultTab,0,8);LIMIT(historyLimit,1,12);LIMIT(focusMinutes,5,120);LIMIT(breakMinutes,1,60);
#undef LIMIT
    if(p.defaultTab==4)p.defaultTab=3; // Former Quick Controls now opens Widgets.
}
static bool unicodeConfig(const wchar_t* path){
    HANDLE input=CreateFile(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);DWORD bytes=0;char* raw=0;
    if(input!=INVALID_HANDLE_VALUE){bytes=GetFileSize(input,0);if(bytes>1024*1024){CloseHandle(input);return false;}raw=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes+2);DWORD read=0;if(!raw||!ReadFile(input,raw,bytes,&read,0)){CloseHandle(input);if(raw)HeapFree(GetProcessHeap(),0,raw);return false;}CloseHandle(input);if(bytes>=2&&(BYTE)raw[0]==255&&(BYTE)raw[1]==254){HeapFree(GetProcessHeap(),0,raw);return true;}}
    int skip=bytes>=3&&(BYTE)raw[0]==239&&(BYTE)raw[1]==187&&(BYTE)raw[2]==191?3:0;UINT codepage=CP_UTF8;int count=bytes?MultiByteToWideChar(codepage,MB_ERR_INVALID_CHARS,raw+skip,bytes-skip,0,0):0;if(bytes&&!count){codepage=CP_ACP;count=MultiByteToWideChar(codepage,0,raw,bytes,0,0);skip=0;}
    wchar_t* text=(wchar_t*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(wchar_t));if(!text){if(raw)HeapFree(GetProcessHeap(),0,raw);return false;}text[0]=0xfeff;if(count)MultiByteToWideChar(codepage,0,raw+skip,bytes-skip,text+1,count);if(raw)HeapFree(GetProcessHeap(),0,raw);
    HANDLE output=CreateFile(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);bool ok=false;if(output!=INVALID_HANDLE_VALUE){DWORD written=0;ok=WriteFile(output,text,(count+1)*sizeof(wchar_t),&written,0)&&written==(count+1)*sizeof(wchar_t);CloseHandle(output);}HeapFree(GetProcessHeap(),0,text);return ok;
}
static bool writePreferences(const wchar_t* file,const Preferences& p){bool ok=true;
    ok=p.toolbar.save(file)&&ok;
#define WRITE(field,section) {wchar_t number[32];swprintf_s(number,L"%d",(int)p.field);ok=WritePrivateProfileString(section,WIDEN(#field),number,file)&&ok;}
    WRITE(glassMode,L"Appearance");
    WRITE(backgroundRgb,L"Appearance");WRITE(fontRgb,L"Appearance");WRITE(glassTintRgb,L"Appearance");
    WRITE(gmailEnabled,L"Connectors");WRITE(gmailUnreadOnly,L"Connectors");WRITE(androidEnabled,L"Connectors");WRITE(iphoneEnabled,L"Connectors");WRITE(gmailRefresh,L"Connectors");WRITE(phoneRefresh,L"Connectors");
    WRITE(sizePercent,L"Layout");WRITE(textPercent,L"Layout");WRITE(accentRgb,L"Appearance");WRITE(cornerDip,L"Appearance");WRITE(blurPasses,L"Appearance");WRITE(tintPercent,L"Appearance");WRITE(hoverWidth,L"Behavior");WRITE(hoverHeight,L"Behavior");WRITE(hoverDelay,L"Behavior");WRITE(closeDelay,L"Behavior");WRITE(animationMs,L"Behavior");WRITE(alignment,L"Layout");WRITE(offset,L"Layout");WRITE(defaultTab,L"Behavior");WRITE(hoverEnabled,L"Behavior");WRITE(waveformEnabled,L"Behavior");WRITE(swipeEnabled,L"Behavior");WRITE(clipboardEnabled,L"Tools");WRITE(automaticBreak,L"Tools");WRITE(historyLimit,L"Tools");WRITE(focusMinutes,L"Tools");WRITE(breakMinutes,L"Tools");
#undef WRITE
    ok=WritePrivateProfileString(L"Appearance",L"DisplayName",p.displayName,file)&&ok;
    ok=WritePrivateProfileString(L"Connectors",L"GmailClientId",p.gmailClientId,file)&&ok;ok=WritePrivateProfileString(L"Connectors",L"KdeCliPath",p.kdeCliPath,file)&&ok;ok=WritePrivateProfileString(L"Connectors",L"AndroidDeviceId",p.androidDeviceId,file)&&ok;
    ok=WritePrivateProfileString(L"Layout",L"Monitor",p.monitor,file)&&ok;
    wchar_t apps[appLimit*(64+MAX_PATH+32)+1]={};
    size_t used=0;
    for(int i=0;i<appLimit;i++){
        const auto& app=p.apps[i];
        used+=swprintf_s(apps+used,ARRAYSIZE(apps)-used,L"Name%d=%s",i,app.name)+1;
        used+=swprintf_s(apps+used,ARRAYSIZE(apps)-used,L"Target%d=%s",i,app.path)+1;
    }
    ok=WritePrivateProfileSection(L"Launchers",apps,file)&&ok;
    for(int i=0;i<workstationCount;i++){
        wchar_t section[32];workstationSection(i,section);
        const auto& station=p.workstations[i];
        if(!station.name[0]){ok=WritePrivateProfileString(section,nullptr,nullptr,file)&&ok;continue;}
        ok=WritePrivateProfileString(section,L"Name",station.name,file)&&ok;
        ok=WritePrivateProfileString(section,L"SpotifyTrack",station.spotify,file)&&ok;
        for(int j=0;j<workstationApps;j++){
            wchar_t key[24];
            swprintf_s(key,L"App%dName",j+1);
            ok=WritePrivateProfileString(section,key,station.apps[j].name,file)&&ok;
            swprintf_s(key,L"App%dTarget",j+1);
            ok=WritePrivateProfileString(section,key,station.apps[j].path,file)&&ok;
        }
        for(int j=0;j<workstationTabs;j++){
            wchar_t key[24];swprintf_s(key,L"Tab%d",j+1);
            ok=WritePrivateProfileString(section,key,station.tabs[j],file)&&ok;
        }
    }
    wchar_t encoded[4096];int at=0;for(int i=0;p.note[i]&&at<4093;i++){wchar_t c=p.note[i];if(c==L'\r')continue;if(c==L'\n'||c==L'\\'){encoded[at++]=L'\\';encoded[at++]=c==L'\n'?L'n':c;}else encoded[at++]=c;}encoded[at]=0;ok=WritePrivateProfileString(L"Notes",L"Text",encoded,file)&&ok;return ok;
}

#undef WIDEN
#undef WIDEN_INNER
