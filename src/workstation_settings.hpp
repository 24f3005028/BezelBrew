#pragma once

#include "workstation_link_dialog.hpp"

static int selectedStation=-1;
static bool workstationAddOpen=false;
constexpr int stationAdd=443,stationAppChoice=470,stationWebsiteChoice=471,stationSpotifyChoice=472;
constexpr int stationSpotifyEdit=456,stationSpotifyRemove=457;
constexpr int stationAppBase=9100,stationRemoveAppBase=9120;
constexpr int stationTabBase=9140,stationRemoveTabBase=9160;
static void populateSettings();

static bool browseTarget(AppTarget& target){
    const COMDLG_FILTERSPEC types[]={{L"Apps and shortcuts",L"*.exe;*.lnk;*.url"}};
    if(!selectFile(settingsWindow,types,ARRAYSIZE(types),target.path,FOS_NODEREFERENCELINKS))return false;
    const wchar_t* filename=wcsrchr(target.path,L'\\');
    filename=filename?filename+1:target.path;
    const wchar_t* extension=wcsrchr(filename,L'.');
    size_t length=extension?(size_t)(extension-filename):wcslen(filename);
    wcsncpy_s(target.name,filename,length<ARRAYSIZE(target.name)?length:ARRAYSIZE(target.name)-1);
    if(extension&&!_wcsicmp(extension,L".url")){
        wchar_t link[webAddressSize],url[webAddressSize];
        GetPrivateProfileString(L"InternetShortcut",L"URL",L"",link,ARRAYSIZE(link),target.path);
        if(LaunchTargets::webAddress(link,url)&&wcslen(url)<ARRAYSIZE(target.path))wcscpy_s(target.path,url);
    }
    return true;
}
static bool pickTarget(AppTarget& target){
    LauncherDialog::Dialog dialog={};
    INT_PTR result=LauncherDialog::show(settingsWindow,dialog);
    if(result==LauncherDialog::extraId)return browseTarget(target);
    if(result!=IDOK)return false;
    target=dialog.chosen;
    return true;
}
static void updateSpotifySettings(){
    HWND button=GetDlgItem(settingsWindow,454);
    if(!GetDlgItem(settingsWindow,455))return;
    bool available=SpotifyPlayback::configured(),linked=SpotifyPlayback::linked(configFile);
    if(!available){
        SetDlgItemText(settingsWindow,455,L"Tracks and playlists open in Spotify. Press Play there to start.");
    }else{
        SetWindowText(button,linked?L"Disconnect Spotify":L"Connect Spotify");
        EnableWindow(button,!SpotifyPlayback::busy());
        SetDlgItemText(settingsWindow,455,linked?L"Starts playback on your active Spotify device.":
            L"Connect Spotify Premium to start playback automatically.");
    }
}
static bool readWorkstationDraft(){
    if(selectedStation<0)return true;
    wchar_t name[64];
    GetDlgItemText(settingsWindow,444,name,ARRAYSIZE(name));
    size_t length=wcslen(name);
    while(length&&name[length-1]<=L' ')name[--length]=0;
    const wchar_t* title=name+wcsspn(name,L" \t");
    if(!title[0]){
        settingsError(L"Give your workstation a name.");
        SetFocus(GetDlgItem(settingsWindow,444));
        return false;
    }
    for(int i=0;i<workstationCount;i++)if(i!=selectedStation&&!_wcsicmp(draft.workstations[i].name,title)){
        settingsError(L"A workstation already uses that name.");
        SetFocus(GetDlgItem(settingsWindow,444));
        return false;
    }
    wcscpy_s(draft.workstations[selectedStation].name,title);
    return true;
}
static int stationAppCount(const Workstation& station){
    int count=0;
    while(count<workstationApps&&station.apps[count].path[0])count++;
    return count;
}
static int stationWebsiteCount(const Workstation& station){
    int count=0;
    while(count<workstationTabs&&station.tabs[count][0])count++;
    return count;
}
static void workstationSettings(){
    HWND stations=control(WC_COMBOBOX,L"",440,38,112,365,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_GROUP|WS_VSCROLL);
    int current=-1,count=0;
    for(int i=0;i<workstationCount;i++){
        if(!draft.workstations[i].name[0])continue;
        int row=(int)SendMessage(stations,CB_ADDSTRING,0,(LPARAM)draft.workstations[i].name);
        SendMessage(stations,CB_SETITEMDATA,row,i);
        if(selectedStation==i)current=row;
        count++;
    }
    if(current<0&&count){
        current=0;
        selectedStation=(int)SendMessage(stations,CB_GETITEMDATA,current,0);
    }
    if(current>=0)SendMessage(stations,CB_SETCURSEL,current,0);
    else{
        choice(stations,L"No workstations yet");
        SendMessage(stations,CB_SETCURSEL,0,0);
        EnableWindow(stations,FALSE);
    }
    HWND add=control(WC_BUTTON,L"New workstation",441,417,112,152,32,WS_TABSTOP);
    EnableWindow(add,count<workstationCount);
    if(selectedStation<0){
        control(WC_STATIC,L"Create a workstation to open your apps, websites, and Spotify music together.",0,38,164,625,44);
        return;
    }
    control(WC_BUTTON,L"Remove",442,581,112,107,32,WS_TABSTOP);
    auto& station=draft.workstations[selectedStation];
    caption(L"Workstation name",157);
    HWND name=control(WC_EDIT,station.name,444,218,157,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(name,EM_SETLIMITTEXT,63,0);
    int apps=stationAppCount(station),tabs=stationWebsiteCount(station);
    control(WC_BUTTON,workstationAddOpen?L"−  Add item":L"+  Add item",stationAdd,38,206,127,33,WS_TABSTOP);
    if(workstationAddOpen){
        EnableWindow(control(WC_BUTTON,L"App…",stationAppChoice,177,206,92,33,WS_TABSTOP),apps<workstationApps);
        EnableWindow(control(WC_BUTTON,L"Website…",stationWebsiteChoice,280,206,105,33,WS_TABSTOP),tabs<workstationTabs);
        control(WC_BUTTON,station.spotify[0]?L"Change Spotify…":L"Spotify track or playlist…",
                stationSpotifyChoice,397,206,203,33,WS_TABSTOP);
    }

    int y=255;
    if(apps){
        control(WC_STATIC,L"Apps",660,38,y,325,23);
        y+=28;
        for(int i=0;i<apps;i++){
            int x=38+(i%2)*331,row=y+(i/2)*40;
            const auto& app=station.apps[i];
            control(WC_BUTTON,app.name[0]?app.name:L"Choose app…",stationAppBase+i,x,row,235,32,WS_TABSTOP);
            control(WC_BUTTON,L"Remove",stationRemoveAppBase+i,x+244,row,75,32,WS_TABSTOP);
        }
        y+=((apps+1)/2)*40+12;
    }
    if(tabs){
        control(WC_STATIC,L"Websites",661,38,y,325,23);
        y+=28;
        for(int i=0;i<tabs;i++){
            int x=38+(i%2)*331,row=y+(i/2)*40;
            const wchar_t* url=station.tabs[i];
            const wchar_t* host=wcsstr(url,L"://");
            control(WC_BUTTON,host?host+3:url,stationTabBase+i,x,row,235,32,WS_TABSTOP);
            control(WC_BUTTON,L"Remove",stationRemoveTabBase+i,x+244,row,75,32,WS_TABSTOP);
        }
        y+=((tabs+1)/2)*40+12;
    }
    if(!apps&&!tabs&&!station.spotify[0]){
        control(WC_STATIC,L"Add what you want to open when this workstation runs.",0,38,y,650,32);
        y+=45;
    }
    if(station.spotify[0]){
        const wchar_t* label=LaunchTargets::spotifyPlaylist(station.spotify)?L"Spotify playlist · Change…":L"Spotify track · Change…";
        control(WC_BUTTON,label,stationSpotifyEdit,38,y,235,32,WS_TABSTOP);
        control(WC_BUTTON,L"Remove",stationSpotifyRemove,282,y,75,32,WS_TABSTOP);
        y+=44;
    }
    if(SpotifyPlayback::configured()){
        control(WC_BUTTON,L"Connect Spotify",454,38,y,174,32,WS_TABSTOP);
        y+=42;
    }
    if(station.spotify[0]||SpotifyPlayback::configured())control(WC_STATIC,L"",455,38,y,650,40);
    updateSpotifySettings();
}
static void revealWorkstationControl(int id){
    HWND child=GetDlgItem(settingsWindow,id);
    const auto* item=SettingsSkin::item(child);
    if(!item)return;
    RECT client;
    GetClientRect(settingsWindow,&client);
    int bottom=MulDiv(client.bottom-D(SettingsSkin::footerHeight+10),96,settingsDpi);
    int needed=item->bounds.bottom-bottom;
    if(SettingsSkin::scroll<needed)SettingsSkin::scroll=needed;
    int limit=item->bounds.top-108;
    if(SettingsSkin::scroll>limit)SettingsSkin::scroll=limit;
    SettingsSkin::layout();
    if(IsWindowVisible(child))SetFocus(child);
}
static void refreshWorkstation(int focusId){
    int scroll=SettingsSkin::scroll;
    workstationAddOpen=false;
    populateSettings();
    SettingsSkin::scroll=scroll;
    revealWorkstationControl(focusId);
}
static bool workstationCommand(HWND window,int id,int notification){
    if(id==440&&notification==CBN_SELCHANGE){
        int chosenRow=(int)SendDlgItemMessage(window,440,CB_GETCURSEL,0,0);
        int index=(int)SendDlgItemMessage(window,440,CB_GETITEMDATA,chosenRow,0);
        if(index>=0&&index<workstationCount&&readWorkstationDraft()){
            selectedStation=index;
            workstationAddOpen=false;
            populateSettings();
            SetFocus(GetDlgItem(window,440));
        }else{
            int rows=(int)SendDlgItemMessage(window,440,CB_GETCOUNT,0,0);
            for(int i=0;i<rows;i++)if(SendDlgItemMessage(window,440,CB_GETITEMDATA,i,0)==selectedStation)
                SendDlgItemMessage(window,440,CB_SETCURSEL,i,0);
        }
        return true;
    }
    if(id==441){
        if(!readWorkstationDraft())return true;
        for(int i=0;i<workstationCount;i++)if(!draft.workstations[i].name[0]){
            selectedStation=i;
            workstationAddOpen=false;
            for(int suffix=1;suffix<=workstationCount;suffix++){
                wchar_t candidate[64];swprintf_s(candidate,L"Workstation %d",suffix);
                bool taken=false;
                for(const auto& station:draft.workstations)if(!_wcsicmp(station.name,candidate))taken=true;
                if(!taken){wcscpy_s(draft.workstations[i].name,candidate);break;}
            }
            populateSettings();
            HWND name=GetDlgItem(window,444);
            SetFocus(name);
            SendMessage(name,EM_SETSEL,0,-1);
            break;
        }
        return true;
    }
    if(id==442&&selectedStation>=0){
        draft.workstations[selectedStation]={};
        selectedStation=-1;
        workstationAddOpen=false;
        populateSettings();
        settingsError(L"Workstation removed from the draft. Save changes to finish.");
        return true;
    }
    if(id==454){
        if(SpotifyPlayback::linked(configFile))SpotifyPlayback::disconnect(configFile);
        else if(SpotifyPlayback::start(win,configFile))settingsError(L"Finish Spotify sign-in in your browser.");
        else settingsError(L"Spotify sign-in is unavailable right now.");
        updateSpotifySettings();
        return true;
    }
    if(selectedStation<0||notification!=BN_CLICKED)return false;
    auto& station=draft.workstations[selectedStation];
    if(id==stationAdd){
        if(readWorkstationDraft()){
            workstationAddOpen=!workstationAddOpen;
            populateSettings();
            SetFocus(GetDlgItem(window,stationAdd));
        }
        return true;
    }

    int apps=stationAppCount(station),tabs=stationWebsiteCount(station);
    if((id==stationAppChoice&&apps<workstationApps)||(id>=stationAppBase&&id<stationAppBase+apps)){
        if(!readWorkstationDraft())return true;
        int slot=id==stationAppChoice?apps:id-stationAppBase;
        AppTarget chosen={};
        if(pickTarget(chosen)){
            station.apps[slot]=chosen;
            refreshWorkstation(stationAppBase+slot);
        }
        return true;
    }
    if((id==stationWebsiteChoice&&tabs<workstationTabs)||(id>=stationTabBase&&id<stationTabBase+tabs)){
        if(!readWorkstationDraft())return true;
        int slot=id==stationWebsiteChoice?tabs:id-stationTabBase;
        WorkstationLinkDialog::Selection selection={};
        wcscpy_s(selection.value,station.tabs[slot]);
        if(WorkstationLinkDialog::show(window,selection)){
            wcscpy_s(station.tabs[slot],selection.value);
            refreshWorkstation(stationTabBase+slot);
        }
        return true;
    }
    if(id==stationSpotifyChoice||id==stationSpotifyEdit){
        if(!readWorkstationDraft())return true;
        WorkstationLinkDialog::Selection selection={true};
        if(station.spotify[0]){
            wchar_t url[80];
            LaunchTargets::spotifyAddress(station.spotify,url);
            wcscpy_s(selection.value,url);
        }
        if(WorkstationLinkDialog::show(window,selection)){
            wcscpy_s(station.spotify,selection.value);
            refreshWorkstation(stationSpotifyEdit);
        }
        return true;
    }
    if(id>=stationRemoveAppBase&&id<stationRemoveAppBase+apps){
        if(readWorkstationDraft()){
            int slot=id-stationRemoveAppBase;
            for(int i=slot;i+1<apps;i++)station.apps[i]=station.apps[i+1];
            station.apps[apps-1]={};
            refreshWorkstation(slot+1<apps?stationAppBase+slot:stationAdd);
        }
        return true;
    }
    if(id>=stationRemoveTabBase&&id<stationRemoveTabBase+tabs){
        if(readWorkstationDraft()){
            int slot=id-stationRemoveTabBase;
            for(int i=slot;i+1<tabs;i++)wcscpy_s(station.tabs[i],station.tabs[i+1]);
            station.tabs[tabs-1][0]=0;
            refreshWorkstation(slot+1<tabs?stationTabBase+slot:stationAdd);
        }
        return true;
    }
    if(id==stationSpotifyRemove){
        if(readWorkstationDraft()){
            station.spotify[0]=0;
            refreshWorkstation(stationAdd);
        }
        return true;
    }
    return false;
}
