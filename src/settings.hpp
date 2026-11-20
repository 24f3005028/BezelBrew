#pragma once
// A native, keyboard-accessible settings window. Drafts are applied explicitly.
constexpr int workstationSettingsPage=9;
constexpr int toolbarSettingsPage=10;
constexpr int appCardBase=8700;
constexpr int settingsNavigationOrder[]={0,toolbarSettingsPage,1,2,3,workstationSettingsPage,4,5,6,7,8};
constexpr int settingsPageCount=ARRAYSIZE(settingsNavigationOrder);
static int settingsNavigationRow(int page){
    for(int i=0;i<settingsPageCount;i++)if(settingsNavigationOrder[i]==page)return i;
    return 0;
}
static HWND settingsWindow=0,settingsTabs=0,settingsMessage=0,settingsNavigation[settingsPageCount]={};static HFONT settingsFont=0,settingsTitleFont=0;
static HWND settingsControls[80];static int settingsControlCount=0,settingsPage=0;static UINT settingsDpi=96;
static char pendingClientSecret[4096]={};static bool connectorAdvanced[2]={};
static bool iphoneSetup=false;
static Connectors::PhoneChoice settingsPhones[16]={};static int settingsPhoneCount=0;static LONG settingsDeviceRevision=-1;
static Preferences draft;static bool draftAnimations=true,draftDocked=false,draftStartup=false;
struct MonitorChoice {wchar_t device[32],label[100];};static MonitorChoice monitors[16];static int monitorCount=0;
static int D(int x){return MulDiv(x,settingsDpi,96);}
struct AppearanceColor {
    int edit,button;
    const wchar_t* label;
    int Preferences::*value;
};
static constexpr AppearanceColor appearanceColors[]={
    {114,182,L"Solid background",&Preferences::backgroundRgb},
    {115,183,L"Text color",&Preferences::fontRgb},
    {116,184,L"Glass tint color",&Preferences::glassTintRgb},
    {102,181,L"Accent color",&Preferences::accentRgb}
};
#include "settings_ui.hpp"
#include "color_picker.hpp"
#include "launcher_dialog.hpp"
static void executablePath(wchar_t* path){GetModuleFileName(0,path,MAX_PATH);}
static bool ownedExecutable(const wchar_t* target){wchar_t exe[MAX_PATH];executablePath(exe);if(!_wcsicmp(target,exe))return true;wchar_t* end=wcsrchr(exe,L'\\');if(!end)return false;wcscpy_s(end+1,MAX_PATH-(end-exe)-1,L"BezelBrew.exe");if(!_wcsicmp(target,exe))return true;wcscpy_s(end+1,MAX_PATH-(end-exe)-1,L"WinPixel.exe");return !_wcsicmp(target,exe);}
static bool ownedStartupShortcut(wchar_t* shortcut){
    wchar_t folder[MAX_PATH];if(FAILED(SHGetFolderPath(0,CSIDL_STARTUP,0,SHGFP_TYPE_CURRENT,folder)))return false;
    const wchar_t* names[]={L"BezelBrew.lnk",L"WinPixel.lnk"};
    for(auto name:names){swprintf_s(shortcut,MAX_PATH,L"%s\\%s",folder,name);IShellLink* link=0;IPersistFile* file=0;bool owned=false;
        if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(void**)&link))){if(SUCCEEDED(link->QueryInterface(IID_IPersistFile,(void**)&file))){wchar_t target[MAX_PATH];if(SUCCEEDED(file->Load(shortcut,STGM_READ))&&SUCCEEDED(link->GetPath(target,MAX_PATH,0,SLGP_RAWPATH)))owned=ownedExecutable(target);file->Release();}link->Release();}if(owned)return true;
    }return false;
}
static bool ownedStartupValue(HKEY key,const wchar_t* name){wchar_t value[MAX_PATH+4];DWORD bytes=sizeof(value),type=0;if(RegQueryValueEx(key,name,0,&type,(BYTE*)value,&bytes)!=ERROR_SUCCESS||type!=REG_SZ||bytes>=sizeof(value))return false;value[bytes/sizeof(wchar_t)]=0;int length=(int)wcslen(value);if(length>2&&value[0]==L'"'&&value[length-1]==L'"'){value[length-1]=0;return ownedExecutable(value+1);}return ownedExecutable(value);}
static bool startupEnabled(){wchar_t shortcut[MAX_PATH];if(ownedStartupShortcut(shortcut))return true;HKEY key;bool enabled=false;if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){enabled=ownedStartupValue(key,L"BezelBrew")||ownedStartupValue(key,L"WinPixel");RegCloseKey(key);}return enabled;}
static bool setStartup(bool enabled){
    if(startupEnabled()==enabled)return true;
    if(!enabled){wchar_t shortcut[MAX_PATH];for(int i=0;i<2&&ownedStartupShortcut(shortcut);i++)if(!DeleteFile(shortcut))return false;}
    HKEY key;if(RegCreateKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,0,0,KEY_SET_VALUE|KEY_QUERY_VALUE,0,&key,0)!=ERROR_SUCCESS)return false;
    bool ok=true;const wchar_t* names[]={L"BezelBrew",L"WinPixel"};
    if(enabled){DWORD bytes=0;LSTATUS existing=RegQueryValueEx(key,L"BezelBrew",0,0,0,&bytes);if(existing==ERROR_SUCCESS&&!ownedStartupValue(key,L"BezelBrew"))ok=false;else{wchar_t exe[MAX_PATH],value[MAX_PATH+4];executablePath(exe);swprintf_s(value,L"\"%s\"",exe);ok=RegSetValueEx(key,L"BezelBrew",0,REG_SZ,(BYTE*)value,(DWORD)((wcslen(value)+1)*sizeof(wchar_t)))==ERROR_SUCCESS;}}
    else for(auto name:names)if(ownedStartupValue(key,name))ok=(RegDeleteValue(key,name)==ERROR_SUCCESS)&&ok;
    RegCloseKey(key);return ok;
}
static BOOL CALLBACK collectMonitors(HMONITOR h,HDC,LPRECT,LPARAM){if(monitorCount>=16)return FALSE;MONITORINFOEX info={};info.cbSize=sizeof(info);GetMonitorInfo(h,&info);auto& item=monitors[monitorCount];wcscpy_s(item.device,info.szDevice);swprintf_s(item.label,L"Display %d  (%ld × %ld)%s",monitorCount+1,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top,info.dwFlags&MONITORINFOF_PRIMARY?L" - primary":L"");monitorCount++;return TRUE;}
static HWND control(const wchar_t* cls,const wchar_t* text,int id,int x,int y,int w,int h,DWORD style=0){
    x+=190;
    if(settingsPage==6&&id!=8410&&id!=8411){
        y+=24;
        if(settingsControlCount==2)style|=WS_GROUP;
    }
    RECT bounds={x,y,x+w,y+h};bool edit=!_wcsicmp(cls,WC_EDIT),comboBox=!_wcsicmp(cls,WC_COMBOBOX),button=!_wcsicmp(cls,WC_BUTTON),track=!_wcsicmp(cls,TRACKBAR_CLASS);
    if(comboBox)style|=CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
    if(!edit&&!comboBox&&!button&&!track)style|=SS_NOPREFIX;
    HWND child=CreateWindowEx(0,cls,text,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,D(x+(edit?9:0)),D(y+(edit?5:0)),D(w-(edit?18:0)),D(h-(edit?10:0)),settingsWindow,(HMENU)(INT_PTR)id,GetModuleHandle(0),0);
    SettingsSkin::Kind kind=edit?SettingsSkin::Edit:comboBox?SettingsSkin::Combo:track?SettingsSkin::Slider:button?(style&BS_TYPEMASK)==BS_AUTOCHECKBOX?SettingsSkin::Switch:id>=8200&&id<8202?SettingsSkin::MaterialCard:SettingsSkin::Button:SettingsSkin::Text;
    if(comboBox)bounds.bottom=bounds.top+32;SettingsSkin::prepare(child,kind,bounds);
    if(settingsControlCount<80)settingsControls[settingsControlCount++]=child;return child;
}
static bool selectFile(HWND owner,const COMDLG_FILTERSPEC* filters,UINT count,wchar_t (&path)[MAX_PATH],
                       FILEOPENDIALOGOPTIONS extraOptions=0){
    IFileOpenDialog* dialog=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))
        return false;
    FILEOPENDIALOGOPTIONS options=0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_NOCHANGEDIR|extraOptions|
                       ((extraOptions&FOS_PICKFOLDERS)?0:FOS_FILEMUSTEXIST));
    if(count)dialog->SetFileTypes(count,filters);
    bool selected=false;
    IShellItem* file=nullptr;
    if(SUCCEEDED(dialog->Show(owner))&&SUCCEEDED(dialog->GetResult(&file))){
        PWSTR name=nullptr;
        if(SUCCEEDED(file->GetDisplayName(SIGDN_FILESYSPATH,&name))){
            if(wcslen(name)<ARRAYSIZE(path)){wcscpy_s(path,name);selected=true;}
            else SetWindowText(settingsMessage,L"Choose a file with a shorter path.");
            CoTaskMemFree(name);
        }
        file->Release();
    }
    dialog->Release();
    return selected;
}
static void caption(const wchar_t* text,int y){control(WC_STATIC,text,0,38,y+5,177,25);}
static void checkbox(int id,const wchar_t* text,int y,bool checked){HWND item=control(WC_BUTTON,text,id,218,y,470,32,BS_AUTOCHECKBOX|WS_TABSTOP);SendMessage(item,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);}
static void editNumber(int id,const wchar_t* name,int y,int value){caption(name,y);wchar_t text[30];swprintf_s(text,L"%d",value);HWND edit=control(WC_EDIT,text,id,218,y,120,32,ES_AUTOHSCROLL|WS_TABSTOP);SendMessage(edit,EM_SETLIMITTEXT,7,0);}
static HWND combo(int id,const wchar_t* name,int y){caption(name,y);return control(WC_COMBOBOX,L"",id,218,y,470,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL);}
static void choice(HWND item,const wchar_t* value){SendMessage(item,CB_ADDSTRING,0,(LPARAM)value);}
static void slider(int id,const wchar_t* name,int y,int value,int lo,int hi,const wchar_t* suffix){caption(name,y);HWND item=control(TRACKBAR_CLASS,L"",id,210,y,330,32,TBS_NOTICKS|WS_TABSTOP);SendMessage(item,TBM_SETRANGE,TRUE,MAKELPARAM(lo,hi));SendMessage(item,TBM_SETPOS,TRUE,value);wchar_t label[40];swprintf_s(label,L"%d%s",value,suffix);control(WC_STATIC,label,id+1000,555,y+5,128,24);}
static bool checked(int id){return SendDlgItemMessage(settingsWindow,id,BM_GETCHECK,0,0)==BST_CHECKED;}
static void settingsError(const wchar_t* text){SetWindowText(settingsMessage,text);}
static bool number(int id,int& destination,int low,int high){wchar_t text[32];GetDlgItemText(settingsWindow,id,text,32);wchar_t* end=0;long value=wcstol(text,&end,10);if(!text[0]||!end||*end||value<low||value>high){wchar_t message[130];swprintf_s(message,L"Enter a value between %d and %d.",low,high);settingsError(message);SetFocus(GetDlgItem(settingsWindow,id));return false;}destination=(int)value;return true;}
#include "workstation_settings.hpp"
#include "apps_settings.hpp"
#include "theme_settings.hpp"
static bool readDraft(){
    if(settingsPage==0){
        GetDlgItemText(settingsWindow,themeNameId,themeNameDraft,ARRAYSIZE(themeNameDraft));
        wchar_t name[64];
        GetDlgItemText(settingsWindow,113,name,ARRAYSIZE(name));
        int length=(int)wcslen(name), first=0;
        while(length&&name[length-1]<=L' ')name[--length]=0;
        while(name[first]&&name[first]<=L' ')first++;
        if(!name[first]){settingsError(L"Enter a display name.");return false;}
        for(int i=first;name[i];i++)if(name[i]<32){settingsError(L"Display name must be a single line.");return false;}
        for(const auto& color:appearanceColors){
            if(!GetDlgItem(settingsWindow,color.edit))continue;
            wchar_t hex[16];unsigned value;
            GetDlgItemText(settingsWindow,color.edit,hex,ARRAYSIZE(hex));
            if(!SettingsSkin::parseHex(hex,value)){
                wchar_t message[100];
                swprintf_s(message,L"%s needs six hex digits, such as FFB6D3.",color.label);
                settingsError(message);
                SetFocus(GetDlgItem(settingsWindow,color.edit));
                return false;
            }
            draft.*color.value=(int)value;
        }
        wcscpy_s(draft.displayName,name+first);
        draft.sizePercent=(int)SendDlgItemMessage(settingsWindow,103,TBM_GETPOS,0,0);
        draft.textPercent=(int)SendDlgItemMessage(settingsWindow,104,TBM_GETPOS,0,0);
        draft.cornerDip=(int)SendDlgItemMessage(settingsWindow,105,TBM_GETPOS,0,0);
        if(draft.glassMode){
            draft.blurPasses=(int)SendDlgItemMessage(settingsWindow,106,CB_GETCURSEL,0,0);
            draft.tintPercent=(int)SendDlgItemMessage(settingsWindow,107,TBM_GETPOS,0,0);
        }
        draftAnimations=checked(108);
        draftDocked=checked(109);
    }
    if(settingsPage==1){int index=(int)SendDlgItemMessage(settingsWindow,201,CB_GETCURSEL,0,0);draft.monitor[0]=0;if(index>0&&index<=monitorCount)wcscpy_s(draft.monitor,monitors[index-1].device);draft.alignment=(int)SendDlgItemMessage(settingsWindow,202,CB_GETCURSEL,0,0);if(!number(203,draft.offset,-600,600)||!number(205,draft.hoverWidth,80,500)||!number(206,draft.hoverHeight,4,24)||!number(207,draft.hoverDelay,20,1000)||!number(208,draft.closeDelay,100,2000))return false;draft.hoverEnabled=checked(204);int indexSpeed=(int)SendDlgItemMessage(settingsWindow,209,CB_GETCURSEL,0,0);int speeds[]={180,300,450};draft.animationMs=speeds[indexSpeed<0?1:indexSpeed];}
    if(settingsPage==2){int index=(int)SendDlgItemMessage(settingsWindow,301,CB_GETCURSEL,0,0);if(index>=0&&index<primaryTabCount){int page=primaryPage(index);draft.defaultTab=page==workstationPage?8:page;}draft.waveformEnabled=checked(302);draft.swipeEnabled=checked(303);draft.clipboardEnabled=checked(304);draft.automaticBreak=checked(308);draftStartup=checked(309);if(!number(305,draft.historyLimit,1,12)||!number(306,draft.focusMinutes,5,120)||!number(307,draft.breakMinutes,1,60))return false;}
    if(settingsPage==3&&!readAppDraft())return false;
    if(settingsPage==workstationSettingsPage&&!readWorkstationDraft())return false;
    if(settingsPage==4)GetDlgItemText(settingsWindow,501,draft.note,2048);
    if(settingsPage==5){if(GetDlgItem(settingsWindow,601))draft.gmailEnabled=checked(601);if(GetDlgItem(settingsWindow,602))draft.gmailUnreadOnly=checked(602);if(GetDlgItem(settingsWindow,605)&&!number(605,draft.gmailRefresh,60,900))return false;}
    if(settingsPage==6&&!iphoneSetup){if(GetDlgItem(settingsWindow,610))draft.androidEnabled=checked(610);if(GetDlgItem(settingsWindow,611))GetDlgItemText(settingsWindow,611,draft.kdeCliPath,MAX_PATH);if(GetDlgItem(settingsWindow,613))GetDlgItemText(settingsWindow,613,draft.androidDeviceId,128);if(draft.androidDeviceId[0]&&!Connectors::deviceId(draft.androidDeviceId)){settingsError(L"Device ID may only contain letters, numbers, periods, underscores, and dashes.");return false;}if(GetDlgItem(settingsWindow,619)&&!number(619,draft.phoneRefresh,15,300))return false;}
    if(settingsPage==6&&iphoneSetup)draft.iphoneEnabled=checked(615);
    return true;
}
static bool googleReady(){const wchar_t* id=draft.gmailClientId[0]?draft.gmailClientId:PublisherGoogle::clientId;return PublisherGoogle::configured()||(id[0]&&(pendingClientSecret[0]||Connectors::clientReady(id)));}
static void updatePhonePicker(){
    HWND picker=GetDlgItem(settingsWindow,640);if(!picker||SendMessage(picker,CB_GETDROPPEDSTATE,0,0))return;
    if(settingsDeviceRevision==connectorView.deviceRevision&&SendMessage(picker,CB_GETCOUNT,0,0)>0)return;
    settingsDeviceRevision=connectorView.deviceRevision;settingsPhoneCount=connectorView.deviceCount;memcpy(settingsPhones,connectorView.devices,sizeof(settingsPhones));
    SendMessage(picker,CB_RESETCONTENT,0,0);choice(picker,settingsPhoneCount?L"Choose your phone":L"No paired phone found yet");int selectedPhone=0;
    for(int i=0;i<settingsPhoneCount;i++){choice(picker,settingsPhones[i].name);if(!wcscmp(settingsPhones[i].id,draft.androidDeviceId))selectedPhone=i+1;}
    if(!selectedPhone&&settingsPhoneCount==1){selectedPhone=1;wcscpy_s(draft.androidDeviceId,settingsPhones[0].id);}SendMessage(picker,CB_SETCURSEL,selectedPhone,0);
}
static void updateConnectorStatus(){
    if(!settingsWindow)return;
    if(settingsPage==5){
        bool ready=googleReady();const wchar_t* status=connectorView.gmailStatus[0]?connectorView.gmailStatus:L"Sign in to show your latest mail.";
        if(!ready&&!connectorAdvanced[0])status=L"Google sign-in is not available in this build yet. This needs a one-time setup by the app publisher; you do not need a developer account.";
        SetDlgItemText(settingsWindow,609,status);EnableWindow(GetDlgItem(settingsWindow,606),ready&&!InterlockedCompareExchange(&Connectors::authenticate,0,0));
        if(!connectorAdvanced[0])EnableWindow(GetDlgItem(settingsWindow,601),ready);
        EnableWindow(GetDlgItem(settingsWindow,607),Connectors::hasAccount()||prefs.gmailEnabled||InterlockedCompareExchange(&Connectors::authenticate,0,0));
    }
    if(settingsPage==6&&!iphoneSetup){
        if(connectorView.detectedCli[0]&&!Connectors::Setup::executable(draft.kdeCliPath))wcscpy_s(draft.kdeCliPath,connectorView.detectedCli);updatePhonePicker();
        const wchar_t* status=connectorView.setupBusy?L"Looking for your paired phone…":prefs.androidEnabled?connectorView.androidStatus:connectorView.setupStatus[0]?connectorView.setupStatus:L"Install the companion app, pair your phone, then choose Find my phone.";
        SetDlgItemText(settingsWindow,614,status);
        bool selected=GetDlgItem(settingsWindow,640)&&SendDlgItemMessage(settingsWindow,640,CB_GETCURSEL,0,0)>0;
        if(!connectorAdvanced[1]){EnableWindow(GetDlgItem(settingsWindow,616),selected&&Connectors::Setup::executable(draft.kdeCliPath)&&!connectorView.setupBusy);EnableWindow(GetDlgItem(settingsWindow,635),prefs.androidEnabled);EnableWindow(GetDlgItem(settingsWindow,631),!connectorView.setupBusy);}
    }
    if(settingsPage==6&&iphoneSetup)SetDlgItemText(settingsWindow,618,connectorView.iphoneStatus[0]?connectorView.iphoneStatus:L"Follow the three steps, then check your connection.");
}
constexpr int toolbarToggleBase=9200,toolbarEarlierBase=9220,toolbarLaterBase=9240,toolbarReset=9260;
static void toolbarSettings(){
    wchar_t shown[40];
    swprintf_s(shown,L"%d of %d tiles shown",draft.toolbar.visibleCount(),ToolbarLayout::count);
    control(WC_STATIC,shown,9261,38,115,420,28);
    control(WC_BUTTON,L"Restore defaults",toolbarReset,536,110,152,34,WS_TABSTOP);
    for(int row=0;row<ToolbarLayout::count;row++){
        const int tile=draft.toolbar.order[row];
        const int x=38+(row%2)*331,y=158+(row/2)*43;
        HWND toggle=control(WC_BUTTON,toolbarControls[tile].name,toolbarToggleBase+row,
                            x,y,180,34,BS_AUTOCHECKBOX|WS_TABSTOP);
        SendMessage(toggle,BM_SETCHECK,draft.toolbar.visible(tile)?BST_CHECKED:BST_UNCHECKED,0);
        EnableWindow(control(WC_BUTTON,L"Earlier",toolbarEarlierBase+row,x+184,y,66,34,WS_TABSTOP),row>0);
        EnableWindow(control(WC_BUTTON,L"Later",toolbarLaterBase+row,x+253,y,66,34,WS_TABSTOP),
                     row<ToolbarLayout::count-1);
    }
}
static bool toolbarSettingsCommand(int id){
    if(id>=toolbarToggleBase&&id<toolbarToggleBase+ToolbarLayout::count){
        unsigned bit=1u<<draft.toolbar.order[id-toolbarToggleBase];
        if(checked(id))draft.toolbar.shown|=bit;
        else draft.toolbar.shown&=~bit;
        wchar_t shown[40];
        swprintf_s(shown,L"%d of %d tiles shown",draft.toolbar.visibleCount(),ToolbarLayout::count);
        SetDlgItemText(settingsWindow,9261,shown);
        return true;
    }
    int focusId=toolbarReset;
    if(id==toolbarReset){
        draft.toolbar=ToolbarLayout::defaults();
    }else{
        const bool earlier=id>=toolbarEarlierBase&&id<toolbarEarlierBase+ToolbarLayout::count;
        const bool later=id>=toolbarLaterBase&&id<toolbarLaterBase+ToolbarLayout::count;
        if(!earlier&&!later)return false;
        const int row=id-(earlier?toolbarEarlierBase:toolbarLaterBase),target=row+(earlier?-1:1);
        if(target<0||target>=ToolbarLayout::count)return true;
        const int tile=draft.toolbar.order[row];
        draft.toolbar.order[row]=draft.toolbar.order[target];
        draft.toolbar.order[target]=tile;
        focusId=(target==0||target==ToolbarLayout::count-1)?toolbarToggleBase+target:
                (earlier?toolbarEarlierBase:toolbarLaterBase)+target;
    }
    int scroll=SettingsSkin::scroll;
    populateSettings();
    SettingsSkin::scroll=scroll;
    SettingsSkin::layout();
    HWND button=GetDlgItem(settingsWindow,focusId);
    if(IsWindowVisible(button))SetFocus(button);
    return true;
}
static void populateSettings(){
    if(settingsPage!=3)editingApp=-1;
    if(settingsPage!=workstationSettingsPage)workstationAddOpen=false;
    SettingsSkin::scroll=0;
    for(int i=0;i<settingsControlCount;i++)DestroyWindow(settingsControls[i]);settingsControlCount=0;settingsError(L"");
    ShowWindow(GetDlgItem(settingsWindow,9001),settingsPage==0?SW_SHOW:SW_HIDE);
    if(settingsPage==6){
        HWND android=control(WC_BUTTON,L"Android",8410,38,108,113,32,BS_AUTORADIOBUTTON|WS_GROUP|(iphoneSetup?0:WS_TABSTOP));
        HWND iphone=control(WC_BUTTON,L"iPhone",8411,159,108,113,32,BS_AUTORADIOBUTTON|(iphoneSetup?WS_TABSTOP:0));
        SendMessage(iphoneSetup?iphone:android,BM_SETCHECK,BST_CHECKED,0);
    }
    if(settingsPage==0){
        themeControls();
        for(int i=0;i<2;i++)control(WC_BUTTON,appearanceModes[i],8200+i,38+i*331,273,319,56,WS_TABSTOP);
        caption(L"Display name",336);
        HWND name=control(WC_EDIT,draft.displayName,113,218,336,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
        SendMessage(name,EM_SETLIMITTEXT,63,0);
        const AppearanceColor* visible[]={&appearanceColors[draft.glassMode?2:0],&appearanceColors[1],&appearanceColors[3]};
        for(int i=0;i<(int)ARRAYSIZE(visible);i++){
            const auto& color=*visible[i];
            int x=38+(i%2)*331,y=375+(i/2)*36;
            control(WC_STATIC,color.label,0,x,y+4,137,24);
            wchar_t hex[7],button[50];
            swprintf_s(hex,L"%06X",draft.*color.value);
            swprintf_s(button,L"Choose %s",color.label);
            HWND edit=control(WC_EDIT,hex,color.edit,x+139,y,116,32,ES_AUTOHSCROLL|WS_TABSTOP);
            SendMessage(edit,EM_SETLIMITTEXT,6,0);
            control(WC_BUTTON,button,color.button,x+261,y,58,32,WS_TABSTOP);
        }
        if(draft.glassMode){
            control(WC_STATIC,L"Glass blur",0,369,415,137,24);
            HWND blur=control(WC_COMBOBOX,L"",106,508,411,180,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL);
            const wchar_t* blurNames[]={L"Clear",L"Subtle",L"Soft",L"Frosted",L"Strong frost"};
            for(auto blurName:blurNames)choice(blur,blurName);
            SendMessage(blur,CB_SETCURSEL,draft.blurPasses,0);
        }
        slider(103,L"Island size",447,draft.sizePercent,80,160,L"%");
        slider(104,L"Text size",483,draft.textPercent,85,130,L"%");
        slider(105,L"Corner radius",519,draft.cornerDip,8,40,L" px");
        if(draft.glassMode)slider(107,L"Glass tint strength",555,draft.tintPercent,0,70,L"%");
        int toggleY=draft.glassMode?592:556;
        HWND animation=control(WC_BUTTON,L"Smooth animations",108,38,toggleY,319,32,BS_AUTOCHECKBOX|WS_TABSTOP);
        HWND dock=control(WC_BUTTON,L"Flat top edge",109,369,toggleY,319,32,BS_AUTOCHECKBOX|WS_TABSTOP);
        SendMessage(animation,BM_SETCHECK,draftAnimations?BST_CHECKED:BST_UNCHECKED,0);
        SendMessage(dock,BM_SETCHECK,draftDocked?BST_CHECKED:BST_UNCHECKED,0);
    }

    if(settingsPage==1){HWND displays=combo(201,L"Display",142);choice(displays,L"Automatic - current display");monitorCount=0;EnumDisplayMonitors(0,0,collectMonitors,0);int selectedMonitor=0;for(int i=0;i<monitorCount;i++){choice(displays,monitors[i].label);if(!_wcsicmp(draft.monitor,monitors[i].device))selectedMonitor=i+1;}SendMessage(displays,CB_SETCURSEL,selectedMonitor,0);HWND align=combo(202,L"Position",183);const wchar_t* alignNames[]={L"Top left",L"Top center",L"Top right"};for(auto name:alignNames)choice(align,name);SendMessage(align,CB_SETCURSEL,draft.alignment,0);editNumber(203,L"Horizontal offset (px)",224,draft.offset);checkbox(204,L"Open when the pointer rests on the top edge",265,draft.hoverEnabled);editNumber(205,L"Hover target width (px)",305,draft.hoverWidth);editNumber(206,L"Hover target height (px)",345,draft.hoverHeight);editNumber(207,L"Hover delay (ms)",385,draft.hoverDelay);editNumber(208,L"Close delay (ms)",425,draft.closeDelay);HWND speed=combo(209,L"Animation speed",465);choice(speed,L"Fast (180 ms)");choice(speed,L"Balanced (300 ms)");choice(speed,L"Relaxed (450 ms)");SendMessage(speed,CB_SETCURSEL,draft.animationMs<240?0:draft.animationMs>375?2:1,0);}
    if(settingsPage==2){HWND section=combo(301,L"Starting section",140);for(auto name:tabNames)choice(section,name);SendMessage(section,CB_SETCURSEL,primaryIndex(startupPage(draft.defaultTab)),0);checkbox(302,L"Animate the song waveform",181,draft.waveformEnabled);checkbox(303,L"Swipe and scroll horizontally between sections",216,draft.swipeEnabled);checkbox(304,L"Capture copied text (turning off clears history)",251,draft.clipboardEnabled);editNumber(305,L"Clipboard entries",291,draft.historyLimit);editNumber(306,L"Focus duration (minutes)",331,draft.focusMinutes);editNumber(307,L"Break duration (minutes)",371,draft.breakMinutes);checkbox(308,L"Start a break automatically after focus ends",411,draft.automaticBreak);checkbox(309,L"Start automatically when I sign in to Windows",451,draftStartup);}
    if(settingsPage==toolbarSettingsPage)toolbarSettings();
    if(settingsPage==3)appsSettings();
    if(settingsPage==workstationSettingsPage)workstationSettings();
    if(settingsPage==4){control(WC_STATIC,L"A private scratchpad, saved locally with your settings. Up to 2,047 characters.",0,38,142,660,30);HWND note=control(WC_EDIT,draft.note,501,38,182,650,310,ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|WS_TABSTOP);SendMessage(note,EM_SETLIMITTEXT,2047,0);}
    if(settingsPage==8){control(WC_STATIC,L"Ctrl + Alt + N\r\nOpen or hide the island, even after Quit.\r\n\r\nWin + Alt + N\r\nPause for 30 minutes, or resume immediately.\r\n\r\nSwipe left/right or use Shift + mouse wheel to change sections.\r\nThe startup and launch shortcut can be installed with install-startup.ps1.\r\nGlass refreshes its background sample each time the island opens.",0,38,146,660,340);}
    if(settingsPage==5&&connectorAdvanced[0]){
        control(WC_STATIC,L"Gmail · latest Inbox headers",0,38,141,650,30);
        checkbox(601,L"Enable Gmail background refresh",181,draft.gmailEnabled);
        checkbox(602,L"Show unread messages only",217,draft.gmailUnreadOnly);
        caption(L"Google Desktop client",258);
        control(WC_EDIT,draft.gmailClientId[0]?draft.gmailClientId:PublisherGoogle::configured()?L"Provided by the app publisher":L"Import a Desktop OAuth client JSON",603,218,258,355,32,ES_READONLY|ES_AUTOHSCROLL|WS_TABSTOP);
        control(WC_BUTTON,L"Import JSON…",604,581,258,107,28,WS_TABSTOP);
        editNumber(605,L"Refresh interval (seconds)",300,draft.gmailRefresh);
        control(WC_BUTTON,L"Connect Google…",606,218,343,155,32,WS_TABSTOP);
        control(WC_BUTTON,L"Disconnect",607,385,343,115,32,WS_TABSTOP);
        control(WC_BUTTON,L"Refresh",608,512,343,100,32,WS_TABSTOP);
        control(WC_STATIC,L"",609,38,392,650,55);
        control(WC_STATIC,L"Read-only headers; no sending, deleting, or message bodies. Tokens stay in Windows Credential Manager. Google sign-in opens in your browser.",0,38,451,650,46);
        control(WC_BUTTON,L"Setup guide",620,38,497,120,27,WS_TABSTOP);updateConnectorStatus();
    }
    if(settingsPage==6&&!iphoneSetup&&connectorAdvanced[1]){
        control(WC_STATIC,L"Android · KDE Connect notifications",0,38,141,650,30);
        checkbox(610,L"Enable Android notification sync",181,draft.androidEnabled);
        caption(L"KDE Connect CLI",225);control(WC_EDIT,draft.kdeCliPath,611,218,225,355,32,ES_AUTOHSCROLL|WS_TABSTOP);control(WC_BUTTON,L"Browse…",612,581,225,107,28,WS_TABSTOP);
        caption(L"Device ID (optional)",269);control(WC_EDIT,draft.androidDeviceId,613,218,269,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
        control(WC_STATIC,L"Leave blank to use the first reachable paired KDE Connect device.",0,218,301,470,37);
        editNumber(619,L"Refresh interval (seconds)",342,draft.phoneRefresh);
        control(WC_BUTTON,L"Apply & refresh",616,218,385,155,32,WS_TABSTOP);control(WC_BUTTON,L"Get KDE Connect",621,385,385,155,32,WS_TABSTOP);
        control(WC_STATIC,L"",614,38,435,650,56);
        control(WC_STATIC,L"Pair in KDE Connect, then enable Notification sync on your phone. Notifications are kept in memory; no BezelBrew cloud or account.",0,38,492,650,38);
        SendDlgItemMessage(settingsWindow,611,EM_SETLIMITTEXT,MAX_PATH-1,0);SendDlgItemMessage(settingsWindow,613,EM_SETLIMITTEXT,127,0);updateConnectorStatus();
    }
    if(settingsPage==5&&!connectorAdvanced[0]){
        control(WC_STATIC,L"1. Sign in with Google",660,38,142,650,28);
        control(WC_STATIC,L"Choose your Google account in your browser. Your password is never entered in BezelBrew.",0,38,179,650,42);
        control(WC_STATIC,L"2. Allow Inbox previews",661,38,238,650,28);
        control(WC_STATIC,L"Only sender names, subject lines, and unread counts. Your messages stay in Gmail.",0,38,274,650,42);
        control(WC_BUTTON,L"Sign in with Google",606,38,337,235,38,WS_TABSTOP);
        control(WC_BUTTON,L"Open Gmail",637,289,337,150,38,WS_TABSTOP);
        control(WC_BUTTON,L"Disconnect",607,455,337,145,38,WS_TABSTOP);
        control(WC_STATIC,L"",609,38,394,650,60);
        checkbox(601,L"Keep my Inbox previews updated",472,draft.gmailEnabled);checkbox(602,L"Show unread messages only",516,draft.gmailUnreadOnly);
    }
    if(settingsPage==6&&!iphoneSetup&&!connectorAdvanced[1]){
        wchar_t detected[MAX_PATH];if(Connectors::Setup::detect(detected,draft.kdeCliPath))wcscpy_s(draft.kdeCliPath,detected);
        control(WC_STATIC,L"1. Get the free companion app",660,38,142,425,27);
        control(WC_STATIC,L"Install KDE Connect on this PC and your Android phone. BezelBrew finds it automatically.",0,38,177,425,44);
        control(WC_BUTTON,detected[0]?L"PC app found":L"Install on this PC",621,510,142,178,32,WS_TABSTOP);if(detected[0])EnableWindow(GetDlgItem(settingsWindow,621),FALSE);
        control(WC_BUTTON,L"Get the phone app",633,510,182,178,32,WS_TABSTOP);
        control(WC_STATIC,L"2. Pair your phone",661,38,244,425,27);
        control(WC_STATIC,L"Use the same Wi-Fi. Pair both devices in KDE Connect, then turn on Notification sync and allow notifications on your phone.",0,38,279,425,57);
        control(WC_BUTTON,L"Open KDE Connect",632,510,244,178,32,WS_TABSTOP);
        control(WC_STATIC,L"3. Choose your phone",662,38,352,650,27);
        control(WC_COMBOBOX,L"",640,38,389,450,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL);
        control(WC_BUTTON,L"Find my phone",631,510,389,178,32,WS_TABSTOP);
        control(WC_STATIC,L"",614,38,443,650,51);
        control(WC_BUTTON,L"Connect this phone",616,38,513,235,36,WS_TABSTOP);control(WC_BUTTON,L"Disconnect",635,290,513,150,36,WS_TABSTOP);control(WC_BUTTON,L"Notifications missing?",634,250,566,220,32,WS_TABSTOP);
        updateConnectorStatus();
    }
    if(settingsPage==6&&iphoneSetup){
        control(WC_STATIC,L"1. Pair your iPhone",660,38,142,425,27);
        control(WC_STATIC,L"Turn on Bluetooth on both devices, then pair your iPhone with this PC in Windows settings.",0,38,178,425,44);
        control(WC_BUTTON,L"Bluetooth settings",617,510,142,178,32,WS_TABSTOP);
        control(WC_STATIC,L"2. Allow notifications",661,38,242,650,27);
        control(WC_STATIC,L"On iPhone: Settings > Bluetooth > this PC. Turn on Share System Notifications if that option is available.",0,38,278,650,44);
        control(WC_STATIC,L"3. Check the connection",662,38,342,650,27);
        control(WC_BUTTON,L"Check connection",622,38,384,235,36,WS_TABSTOP);control(WC_BUTTON,L"Use Phone Link instead",623,290,384,220,36,WS_TABSTOP);
        control(WC_STATIC,L"",618,38,442,650,52);checkbox(615,L"Sync iPhone notifications in BezelBrew (beta)",513,draft.iphoneEnabled);
        control(WC_STATIC,L"iPhone sync is a beta and depends on your Bluetooth connection. Phone Link is a separate Windows app; its notifications are not imported into BezelBrew.",0,38,551,650,48);updateConnectorStatus();
    }
    if(settingsPage==5||(settingsPage==6&&!iphoneSetup)){
        bool advanced=connectorAdvanced[settingsPage-5];
        control(WC_BUTTON,advanced?L"Back to easy setup":L"Advanced settings",630,38,settingsPage==6?566:580,190,32,WS_TABSTOP);
        updateConnectorStatus();
    }
    if(settingsPage==7){
        control(WC_STATIC,L"Discord desktop",660,38,142,650,30);
        control(WC_STATIC,L"Toggle your microphone using Discord’s desktop shortcut. Discord comes to the foreground when you use it.",0,38,182,650,46);
        control(WC_BUTTON,L"Toggle Discord mute",8412,38,240,200,36,WS_TABSTOP);
        control(WC_BUTTON,L"Open Discord",8413,253,240,144,36,WS_TABSTOP);
        control(WC_STATIC,L"Check Discord for your current voice channel and mute state.",0,38,295,650,46);
    }

    for(int i=0;i<settingsPageCount;i++)InvalidateRect(settingsNavigation[i],0,FALSE);
    SettingsSkin::layout();
    RedrawWindow(settingsWindow,0,0,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
static bool applySettings(){
    if(!readDraft())return false;
    if(saveEnabled&&!setStartup(draftStartup)){settingsError(L"Windows could not update startup. Your settings were not applied.");return false;}
    if(wcscmp(prefs.gmailClientId,draft.gmailClientId)){Connectors::disconnect();}
    if(pendingClientSecret[0]){if(!Connectors::storeClientSecret(pendingClientSecret)){settingsError(L"Windows could not save the Google client credential. Nothing was applied.");return false;}SecureZeroMemory(pendingClientSecret,sizeof(pendingClientSecret));}
    bool wasCapture=prefs.clipboardEnabled;prefs=draft;
    selected=visiblePage(selected);
    if(!prefs.toolbar.visible(pinTile))pinned=false;
    scrollApps(0);
    SetWindowText(win,appName());wchar_t captionText[96];swprintf_s(captionText,L"%s Settings",appName());SetWindowText(settingsWindow,captionText);swprintf_s(tray.szTip,paused?L"%s paused - Win+Alt+N to resume":L"%s - right-click for menu",appName());tray.uFlags=NIF_TIP;Shell_NotifyIcon(NIM_MODIFY,&tray);excludeOverlayFromShell(win);InvalidateRect(settingsWindow,0,FALSE);animations=draftAnimations;docked=draftDocked;
    uiScale=.55*prefs.sizePercent/100.0;smallWidth=prefs.hoverWidth/uiScale;smallHeight=prefs.hoverHeight/uiScale;
    if(!prefs.clipboardEnabled&&wasCapture){SecureZeroMemory(clips,sizeof(clips));clipCount=0;}if(clipCount>prefs.historyLimit){for(int i=prefs.historyLimit;i<12;i++)SecureZeroMemory(clips[i],sizeof(clips[i]));clipCount=prefs.historyLimit;}scrollOffset=0;
    moving=tabMoving=hoverMoving=false;openness=fromOpen=toOpen=expanded?1:0;tabReveal=hoverReveal=1;swiping=swipeCandidate=false;
    OpticalGlass::release();releaseBuffer();createFonts();setupTooltips();place();animationTimer();repaint();saveSettings();configureConnectors();if(!settingsSaveSucceeded){settingsError(L"Applied for now, but the settings file could not be saved. Check folder permissions.");return false;}if(settingsPage==3)closeAppEditor();else if(settingsPage==workstationSettingsPage)populateSettings();settingsError(L"Changes saved.");return true;
}
static void resizeSettings(HWND window){
    RECT r;GetClientRect(window,&r);if(settingsTabs)MoveWindow(settingsTabs,0,0,0,0,TRUE);
    if(settingsMessage)MoveWindow(settingsMessage,D(228),r.bottom-D(76),r.right-D(258),D(25),TRUE);
    MoveWindow(GetDlgItem(window,9001),D(228),r.bottom-D(49),D(150),D(36),TRUE);
    MoveWindow(GetDlgItem(window,IDCANCEL),r.right-D(253),r.bottom-D(49),D(74),D(36),TRUE);
    MoveWindow(GetDlgItem(window,9002),r.right-D(169),r.bottom-D(49),D(139),D(36),TRUE);
    if(settingsControlCount)SettingsSkin::layout();
}
static LRESULT CALLBACK settingsProc(HWND h,UINT msg,WPARAM wp,LPARAM lp){switch(msg){
case WM_CREATE:{
    settingsWindow=h;settingsDpi=GetDpiForWindow(h);SettingsSkin::fonts();SettingsSkin::chrome(h);
    // The hidden tab retains native programmatic selection and validation. The
    // visible sidebar uses real, keyboard-accessible native buttons.
    settingsTabs=CreateWindow(WC_TABCONTROL,L"",WS_CHILD,0,0,0,0,h,(HMENU)8001,GetModuleHandle(0),0);
    for(int i=0;i<settingsPageCount;i++){
        TCITEM item={};item.mask=TCIF_TEXT;item.pszText=(wchar_t*)SettingsSkin::sections[i];
        TabCtrl_InsertItem(settingsTabs,i,&item);
    }
    for(int page:settingsNavigationOrder){
        int y=81+settingsNavigationRow(page)*43;
        settingsNavigation[page]=CreateWindow(WC_BUTTON,SettingsSkin::sections[page],
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,D(17),D(y),D(164),D(39),h,
            (HMENU)(INT_PTR)(8100+page),GetModuleHandle(0),0);
        SettingsSkin::prepare(settingsNavigation[page],SettingsSkin::Navigation,RECT{17,y,181,y+39});
    }
    TabCtrl_SetCurSel(settingsTabs,settingsPage);
    settingsMessage=CreateWindow(WC_STATIC,L"",WS_CHILD|WS_VISIBLE,0,0,0,0,h,(HMENU)8002,GetModuleHandle(0),0);SettingsSkin::prepare(settingsMessage,SettingsSkin::Text,RECT{});
    struct Button {int id;const wchar_t* text;};Button buttons[]={{9001,L"Reset appearance"},{IDCANCEL,L"Close"},{9002,L"Save changes"}};
    for(auto& button:buttons){HWND child=CreateWindow(WC_BUTTON,button.text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|(button.id==9002?BS_DEFPUSHBUTTON:0),0,0,0,0,h,(HMENU)(INT_PTR)button.id,GetModuleHandle(0),0);SettingsSkin::prepare(child,SettingsSkin::Button,RECT{});}
    populateSettings();resizeSettings(h);return 0;
}
case DM_GETDEFID:return MAKELRESULT(9002,DC_HASDEFID);
case WM_NOTIFY:if(((NMHDR*)lp)->hwndFrom==settingsTabs){if(((NMHDR*)lp)->code==TCN_SELCHANGING)return readDraft()?FALSE:TRUE;if(((NMHDR*)lp)->code==TCN_SELCHANGE){settingsPage=TabCtrl_GetCurSel(settingsTabs);populateSettings();return 0;}}break;
case WM_HSCROLL:{
    HWND slider=(HWND)lp;
    if(!slider)return 0;
    int id=GetDlgCtrlID(slider),value=(int)SendMessage(slider,TBM_GETPOS,0,0);
    wchar_t text[40];
    swprintf_s(text,L"%d%s",value,id==105?L" px":L"%");
    SetDlgItemText(h,id+1000,text);
    if(id==107){
        draft.tintPercent=value;
        InvalidateRect(GetDlgItem(h,8200),nullptr,FALSE);
    }
    if(id==105||id==107){
        RECT preview={D(227),D(109),D(880),D(181)};
        InvalidateRect(h,&preview,FALSE);
    }
    return 0;
}
case WM_COMMAND:{int id=LOWORD(wp);
if(id>=8100&&id<8100+settingsPageCount){if(readDraft()){settingsPage=id-8100;TabCtrl_SetCurSel(settingsTabs,settingsPage);populateSettings();}return 0;}
if(settingsPage==toolbarSettingsPage&&HIWORD(wp)==BN_CLICKED&&toolbarSettingsCommand(id))return 0;
if(settingsPage==3&&appsCommand(h,id,HIWORD(wp)))return 0;
if(settingsPage==workstationSettingsPage&&workstationCommand(h,id,HIWORD(wp)))return 0;
if(id==8410||id==8411){
    bool iphone=id==8411;
    if(settingsPage==6&&iphone!=iphoneSetup){
        if(readDraft()){
            iphoneSetup=iphone;
            populateSettings();
            SetFocus(GetDlgItem(h,id));
        }else{
            SendDlgItemMessage(h,8410,BM_SETCHECK,!iphoneSetup,0);
            SendDlgItemMessage(h,8411,BM_SETCHECK,iphoneSetup,0);
        }
    }
    return 0;
}
if(id==8412){settingsError(DiscordVoice::toggleMute());return 0;}
if(id==8413){
    if(!DiscordVoice::openApp(h))settingsError(L"Install the Discord desktop app to use this shortcut.");
    return 0;
}
if(settingsPage==0&&themeCommand(h,id,HIWORD(wp)))return 0;
if(settingsPage==0&&(id==8200||id==8201)){
    bool glass=id==8200;
    if(glass!=draft.glassMode&&readDraft()){
        draft.glassMode=glass;
        populateSettings();
        SetFocus(GetDlgItem(h,id));
    }
    return 0;
}
if(settingsPage==0){
    for(const auto& color:appearanceColors){
        if(id==color.edit&&HIWORD(wp)==EN_CHANGE&&GetDlgItem(h,color.edit)){
            wchar_t hex[16];unsigned value;
            GetDlgItemText(h,id,hex,ARRAYSIZE(hex));
            if(SettingsSkin::parseHex(hex,value))draft.*color.value=(int)value;
            InvalidateRect(GetDlgItem(h,color.button),nullptr,FALSE);
            InvalidateRect(GetDlgItem(h,8200),nullptr,FALSE);
            InvalidateRect(GetDlgItem(h,8201),nullptr,FALSE);
            InvalidateRect(h,nullptr,FALSE);
            return 0;
        }
        if(id==color.button&&GetDlgItem(h,color.edit)){
            unsigned result;
            if(AccentPicker::choose(h,color.label,draft.*color.value,result)){
                wchar_t hex[7];
                swprintf_s(hex,L"%06X",result);
                SetDlgItemText(h,color.edit,hex);
            }
            return 0;
        }
    }
}
if((id==113&&HIWORD(wp)==EN_CHANGE)||(id==109&&HIWORD(wp)==BN_CLICKED)){
    InvalidateRect(h,nullptr,FALSE);
    return 0;
}

if(id==604||id==612){
    wchar_t file[MAX_PATH]=L"";
    COMDLG_FILTERSPEC filter=id==604?COMDLG_FILTERSPEC{L"Google Desktop OAuth client",L"*.json"}:
                                      COMDLG_FILTERSPEC{L"KDE Connect CLI",L"*.exe"};
    if(selectFile(h,&filter,1,file)){if(id==612)SetDlgItemText(h,611,file);else{char secret[4096]={};wchar_t client[256]={};if(Connectors::importClient(file,client,256,secret,sizeof(secret))){wcscpy_s(draft.gmailClientId,client);strcpy_s(pendingClientSecret,secret);SetDlgItemText(h,603,client);settingsError(L"Client imported. Select Connect Google to save and sign in.");}else settingsError(L"Choose a Google OAuth client JSON created for a Desktop app.");SecureZeroMemory(secret,sizeof(secret));}}return 0;}
if(id==606){if(!googleReady()){settingsError(L"Google sign-in needs to be configured by the app publisher before it is available in this build.");return 0;}if(!draft.gmailClientId[0]||!pendingClientSecret[0]&&!Connectors::clientReady(draft.gmailClientId)&&PublisherGoogle::configured())wcscpy_s(draft.gmailClientId,PublisherGoogle::clientId);SendDlgItemMessage(h,601,BM_SETCHECK,BST_CHECKED,0);if(applySettings()){Connectors::beginAuth();settingsError(L"Complete Google sign-in and consent in your browser.");}return 0;}
if(id==607){SendDlgItemMessage(h,601,BM_SETCHECK,BST_UNCHECKED,0);if(applySettings()){Connectors::disconnect();settingsError(L"Local access removed. Revoke BezelBrew in your Google account to remove Google's grant.");}return 0;}
if(id==608){Connectors::refresh();return 0;}
if(id==616){
    if(!connectorAdvanced[1]){int index=(int)SendDlgItemMessage(h,640,CB_GETCURSEL,0,0)-1;if(index<0||index>=settingsPhoneCount){settingsError(L"Choose your phone first. Use Find my phone if it is missing.");return 0;}wcscpy_s(draft.androidDeviceId,settingsPhones[index].id);draft.androidEnabled=true;}
    if(applySettings())Connectors::refresh();return 0;
}
if(id==622){SendDlgItemMessage(h,615,BM_SETCHECK,BST_CHECKED,0);if(applySettings())Connectors::refresh();return 0;}
if(id==630&&(settingsPage==5||(settingsPage==6&&!iphoneSetup))){if(readDraft()){connectorAdvanced[settingsPage-5]=!connectorAdvanced[settingsPage-5];populateSettings();}return 0;}
if(id==631){Connectors::Config config=connectorConfig();wcscpy_s(config.kdePath,draft.kdeCliPath);Connectors::Setup::scan(config);EnableWindow(GetDlgItem(h,631),FALSE);EnableWindow(GetDlgItem(h,616),FALSE);SetDlgItemText(h,614,L"Looking for your paired phone…");return 0;}
if(id==632){wchar_t cli[MAX_PATH],app[MAX_PATH];if(Connectors::Setup::detect(cli,draft.kdeCliPath)&&Connectors::Setup::companion(cli,app)){if((INT_PTR)ShellExecute(h,L"open",app,0,0,SW_SHOWNORMAL)<=32)settingsError(L"Open KDE Connect from your Start menu, then return here and choose Find my phone.");}else settingsError(L"Install or open KDE Connect from your Start menu, then choose Find my phone.");return 0;}
if(id==633){ShellExecute(h,L"open",L"https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp",0,0,SW_SHOWNORMAL);return 0;}
if(id==634){SetDlgItemText(h,614,L"On your phone: KDE Connect > this PC > Plugin settings > Notification sync. Allow notification access, then send yourself a test notification.");settingsError(L"Keep both devices on the same Wi-Fi. Some apps may hide notification content.");return 0;}
if(id==635){draft.androidEnabled=false;if(applySettings())settingsError(L"Phone sync is off. Your pairing in KDE Connect is unchanged.");updateConnectorStatus();return 0;}
if(id==637){ShellExecute(h,L"open",L"https://mail.google.com/",0,0,SW_SHOWNORMAL);return 0;}
if(id==640&&HIWORD(wp)==CBN_SELCHANGE){int index=(int)SendDlgItemMessage(h,640,CB_GETCURSEL,0,0)-1;if(index>=0&&index<settingsPhoneCount)wcscpy_s(draft.androidDeviceId,settingsPhones[index].id);updateConnectorStatus();return 0;}

if(id==617){ShellExecute(h,L"open",L"ms-settings:bluetooth",0,0,SW_SHOWNORMAL);return 0;}
if(id==620){wchar_t guide[MAX_PATH];GetModuleFileName(0,guide,MAX_PATH);wchar_t* end=wcsrchr(guide,L'\\');if(end){wcscpy_s(end+1,MAX_PATH-(end-guide)-1,settingsPage==5&&connectorAdvanced[0]?L"GooglePublisherSetup.txt":L"Connectors.txt");ShellExecute(h,L"open",guide,0,0,SW_SHOWNORMAL);}return 0;}
if(id==621){ShellExecute(h,L"open",L"ms-windows-store://pdp/?ProductId=9N93MRMSXBF0",0,0,SW_SHOWNORMAL);return 0;}
if(id==623){ShellExecute(h,L"open",L"explorer.exe",L"shell:AppsFolder\\Microsoft.YourPhone_8wekyb3d8bbwe!App",0,SW_SHOWNORMAL);return 0;}
if(id==9002||id==IDOK){applySettings();return 0;}
if(id==IDCANCEL){DestroyWindow(h);return 0;}
if(id==9001){
    GetDlgItemText(h,113,draft.displayName,ARRAYSIZE(draft.displayName));
    selectTheme(-1);
    Preferences defaults;
    defaultPreferences(defaults);
    for(const auto& color:appearanceColors)draft.*color.value=defaults.*color.value;
    draft.sizePercent=defaults.sizePercent;
    draft.textPercent=defaults.textPercent;
    draft.cornerDip=defaults.cornerDip;
    draft.blurPasses=defaults.blurPasses;
    draft.tintPercent=defaults.tintPercent;
    draft.glassMode=defaults.glassMode;
    draftAnimations=true;
    draftDocked=false;
    populateSettings();
    return 0;
}
break;}
case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);SettingsSkin::paint(h,dc);EndPaint(h,&ps);return 0;}
case WM_ERASEBKGND:return 1;
case WM_CTLCOLORDLG:return SettingsSkin::controlColors((HDC)wp,SettingsSkin::dialogBrush,SettingsSkin::surface());
case WM_CTLCOLOREDIT:case WM_CTLCOLORSTATIC:case WM_CTLCOLORLISTBOX:return SettingsSkin::controlColors((HDC)wp,SettingsSkin::inputBrush,SettingsSkin::field());
case WM_DRAWITEM:return SettingsSkin::dropdown((DRAWITEMSTRUCT*)lp);
case WM_MEASUREITEM:if(((MEASUREITEMSTRUCT*)lp)->CtlType==ODT_COMBOBOX){((MEASUREITEMSTRUCT*)lp)->itemHeight=D(28);return TRUE;}break;
case WM_SIZE:resizeSettings(h);InvalidateRect(h,0,FALSE);return 0;
case WM_GETMINMAXINFO:((MINMAXINFO*)lp)->ptMinTrackSize={D(920),D(638)};return 0;
case WM_MOUSEWHEEL:SettingsSkin::scroll-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*38;SettingsSkin::layout();return 0;
case WM_DPICHANGED:{
    readDraft();settingsDpi=HIWORD(wp);SettingsSkin::releaseCanvas();SettingsSkin::fonts();
    for(int i=0;i<settingsPageCount;i++){SendMessage(settingsNavigation[i],WM_SETFONT,(WPARAM)settingsFont,TRUE);MoveWindow(settingsNavigation[i],D(17),D(81+settingsNavigationRow(i)*43),D(164),D(39),TRUE);}
    SendMessage(settingsMessage,WM_SETFONT,(WPARAM)settingsFont,TRUE);int footerIds[]={9001,9002,IDCANCEL};for(int id:footerIds)SendDlgItemMessage(h,id,WM_SETFONT,(WPARAM)settingsFont,TRUE);
    populateSettings();RECT* suggested=(RECT*)lp;SetWindowPos(h,0,suggested->left,suggested->top,suggested->right-suggested->left,suggested->bottom-suggested->top,SWP_NOZORDER);return 0;
}
case WM_CLOSE:DestroyWindow(h);return 0;
case WM_DESTROY:SecureZeroMemory(settingsPhones,sizeof(settingsPhones));settingsPhoneCount=0;SettingsSkin::destroy();SecureZeroMemory(pendingClientSecret,sizeof(pendingClientSecret));for(int i=0;i<settingsControlCount;i++)settingsControls[i]=0;settingsControlCount=0;settingsWindow=settingsTabs=settingsMessage=0;DeleteObject(settingsFont);DeleteObject(settingsTitleFont);settingsFont=settingsTitleFont=0;return 0;
}return DefWindowProc(h,msg,wp,lp);}
static bool showSettings(int page=0){
    if(settingsWindow){
        ShowWindow(settingsWindow,SW_RESTORE);
        SetForegroundWindow(settingsWindow);
        if(!readDraft())return false;
        settingsPage=page;
        TabCtrl_SetCurSel(settingsTabs,page);
        populateSettings();
        return true;
    }
    ZeroMemory(connectorAdvanced,sizeof(connectorAdvanced));iphoneSetup=prefs.iphoneEnabled&&!prefs.androidEnabled;settingsDeviceRevision=-1;editingApp=-1;appsListScroll=0;selectedStation=-1;workstationAddOpen=false;draft=prefs;draftAnimations=animations;draftDocked=docked;draftStartup=startupEnabled();matchEquippedTheme();settingsPage=page;
    WNDCLASS cls={};cls.lpfnWndProc=settingsProc;cls.hInstance=GetModuleHandle(0);cls.lpszClassName=L"WinPixelSettings";cls.hCursor=LoadCursor(0,IDC_ARROW);cls.hbrBackground=0;cls.hIcon=brandIcon();RegisterClass(&cls);
    MONITORINFO info={sizeof(info)};GetMonitorInfo(MonitorFromWindow(win,MONITOR_DEFAULTTOPRIMARY),&info);settingsDpi=dpi;RECT rect={0,0,D(920),D(720)};AdjustWindowRectEx(&rect,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,FALSE,WS_EX_APPWINDOW);int width=rect.right-rect.left,height=rect.bottom-rect.top;int available=info.rcWork.bottom-info.rcWork.top-D(20);if(height>available)height=available;
    wchar_t settingsCaption[96];swprintf_s(settingsCaption,L"%s Settings",appName());
    HWND h=CreateWindowEx(WS_EX_APPWINDOW,L"WinPixelSettings",settingsCaption,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,info.rcWork.left+(info.rcWork.right-info.rcWork.left-width)/2,info.rcWork.top+(info.rcWork.bottom-info.rcWork.top-height)/2,width,height,0,0,GetModuleHandle(0),0);
    if(!h)return false;
    ShowWindow(h,SW_SHOW);ShowWindow(h,SW_RESTORE);SetForegroundWindow(h);
    return true;
}

static void showWorkstationSettings(int station=-1,bool create=false){
    if(!showSettings(workstationSettingsPage))return;
    if(create){
        SendMessage(settingsWindow,WM_COMMAND,441,0);
    }else if(station>=0&&station<workstationCount&&draft.workstations[station].name[0]){
        selectedStation=station;
        populateSettings();
        SetFocus(GetDlgItem(settingsWindow,444));
    }
}
