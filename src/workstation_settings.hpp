#pragma once

static bool shortcutWebsite[4]={};
static int selectedStation=-1;
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
        SetDlgItemText(settingsWindow,455,L"Track links open in Spotify. Autoplay is unavailable in this build.");
    }else{
        SetWindowText(button,linked?L"Disconnect Spotify":L"Connect Spotify");
        EnableWindow(button,!SpotifyPlayback::busy());
        SetDlgItemText(settingsWindow,455,linked?L"Starts the track on your active Spotify device.":
            L"Connect Spotify Premium to start the track automatically.");
    }
}
static bool readShortcutDraft(){
    for(int i=0;i<4;i++){
        auto& name=draft.launcherNames[i];
        auto& target=draft.launcherTargets[i];
        GetDlgItemText(settingsWindow,401+i*2,name,ARRAYSIZE(name));
        GetDlgItemText(settingsWindow,402+i*2,target,ARRAYSIZE(target));
        if(!name[0]&&!target[0])continue;
        if(!target[0]||(wcscmp(target,L"https://")==0)){
            settingsError(L"Choose an app, enter a website, or clear both fields.");
            SetFocus(GetDlgItem(settingsWindow,402+i*2));
            return false;
        }
        if(shortcutWebsite[i]||!_wcsnicmp(target,L"http:",5)||!_wcsnicmp(target,L"https:",6)){
            wchar_t url[webAddressSize];
            if(!LaunchTargets::webAddress(target,url)||wcslen(url)>=ARRAYSIZE(target)){
                settingsError(L"Enter a valid website address.");
                SetFocus(GetDlgItem(settingsWindow,402+i*2));
                return false;
            }
            wcscpy_s(target,url);
            if(!name[0]){
                const wchar_t* host=wcsstr(url,L"://")+3;
                size_t length=wcscspn(host,L"/?#");
                wcsncpy_s(name,host,length<ARRAYSIZE(name)?length:ARRAYSIZE(name)-1);
            }
        }
        if(!name[0]){
            settingsError(L"Enter a name for this shortcut.");
            SetFocus(GetDlgItem(settingsWindow,401+i*2));
            return false;
        }
    }
    return true;
}
static bool readWorkstationDraft(){
    if(selectedStation<0)return true;
    auto& saved=draft.workstations[selectedStation];
    Workstation edited=saved;
    GetDlgItemText(settingsWindow,444,edited.name,ARRAYSIZE(edited.name));
    size_t length=wcslen(edited.name);
    while(length&&edited.name[length-1]<=L' ')edited.name[--length]=0;
    size_t begin=wcsspn(edited.name,L" \t");
    if(begin)memmove(edited.name,edited.name+begin,(length-begin+1)*sizeof(wchar_t));
    if(!edited.name[0]){
        settingsError(L"Give your workstation a name.");
        SetFocus(GetDlgItem(settingsWindow,444));
        return false;
    }
    for(int i=0;i<workstationCount;i++)if(i!=selectedStation&&!_wcsicmp(draft.workstations[i].name,edited.name)){
        settingsError(L"A workstation already uses that name.");
        SetFocus(GetDlgItem(settingsWindow,444));
        return false;
    }
    for(int i=0;i<workstationTabs;i++){
        wchar_t url[webAddressSize];
        int id=450+i;
        GetDlgItemText(settingsWindow,id,url,ARRAYSIZE(url));
        edited.tabs[i][0]=0;
        if(url[0]&&!LaunchTargets::webAddress(url,edited.tabs[i])){
            settingsError(L"Enter a valid web address for each tab, or leave it blank.");
            SetFocus(GetDlgItem(settingsWindow,id));
            return false;
        }
    }
    wchar_t track[webAddressSize];
    GetDlgItemText(settingsWindow,456,track,ARRAYSIZE(track));
    edited.track[0]=0;
    if(track[0]&&!LaunchTargets::trackId(track,edited.track)){
        settingsError(L"Paste a Spotify track link, such as one copied from Share.");
        SetFocus(GetDlgItem(settingsWindow,456));
        return false;
    }
    saved=edited;
    return true;
}
static void launcherSettings(){
    for(int i=0;i<4;i++){
        int y=116+i*112;
        wchar_t heading[30];swprintf_s(heading,L"Shortcut %d",i+1);
        control(WC_STATIC,heading,660+i,38,y+4,163,24,0);
        control(WC_STATIC,L"Opens",0,38,y+38,160,24);
        HWND label=control(WC_EDIT,draft.launcherNames[i],401+i*2,218,y,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
        HWND target=control(WC_EDIT,draft.launcherTargets[i],402+i*2,218,y+34,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
        SendMessage(label,EM_SETLIMITTEXT,63,0);
        SendMessage(target,EM_SETLIMITTEXT,MAX_PATH-1,0);
        SendMessage(label,EM_SETCUEBANNER,FALSE,(LPARAM)L"Shortcut name");
        SendMessage(target,EM_SETCUEBANNER,FALSE,(LPARAM)L"Website, app, or shortcut");
        control(WC_BUTTON,L"Choose app",430+i,218,y+69,132,30,WS_TABSTOP);
        control(WC_BUTTON,L"Enter website",434+i,364,y+69,144,30,WS_TABSTOP);
        control(WC_BUTTON,L"Browse shortcut…",421+i,522,y+69,166,30,WS_TABSTOP);
    }
}
static void workstationSettings(){
    HWND stations=control(WC_COMBOBOX,L"",440,38,158,365,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_GROUP|WS_VSCROLL);
    int current=-1,first=-1;
    for(int i=0;i<workstationCount;i++){
        if(!draft.workstations[i].name[0])continue;
        if(first<0)first=i;
        int row=(int)SendMessage(stations,CB_ADDSTRING,0,(LPARAM)draft.workstations[i].name);
        SendMessage(stations,CB_SETITEMDATA,row,i);
        if(selectedStation==i)current=row;
    }
    if(current<0&&first>=0){selectedStation=first;current=0;}
    if(current>=0)SendMessage(stations,CB_SETCURSEL,current,0);
    else{
        choice(stations,L"No workstations yet");
        SendMessage(stations,CB_SETCURSEL,0,0);
        EnableWindow(stations,FALSE);
    }
    HWND add=control(WC_BUTTON,L"New workstation",441,417,158,152,32,WS_TABSTOP);
    int count=(int)SendMessage(stations,CB_GETCOUNT,0,0);
    EnableWindow(add,first<0||count<workstationCount);
    if(selectedStation<0){
        control(WC_STATIC,L"Create a workstation to open your apps, Chrome tabs, and a Spotify track together.",0,38,213,625,53);
        return;
    }
    control(WC_BUTTON,L"Remove",442,581,158,107,32,WS_TABSTOP);
    auto& station=draft.workstations[selectedStation];
    caption(L"Workstation name",201);
    HWND name=control(WC_EDIT,station.name,444,218,201,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(name,EM_SETLIMITTEXT,63,0);
    control(WC_STATIC,L"Apps",660,38,247,625,24);
    for(int i=0;i<workstationApps;i++){
        int x=38+(i%2)*332,y=278+(i/2)*39;
        wchar_t empty[36];swprintf_s(empty,L"Choose app %d…",i+1);
        const auto& app=station.apps[i];
        control(WC_BUTTON,app.path[0]?(app.name[0]?app.name:app.path):empty,460+i,x,y,245,32,WS_TABSTOP);
        HWND clear=control(WC_BUTTON,L"Clear",464+i,x+252,y,65,32,WS_TABSTOP);
        EnableWindow(clear,app.path[0]!=0);
    }
    control(WC_STATIC,L"Chrome tabs",661,38,362,625,24);
    for(int i=0;i<workstationTabs;i++){
        int x=38+(i%2)*332,y=391+(i/2)*39;
        HWND edit=control(WC_EDIT,station.tabs[i],450+i,x,y,318,32,ES_AUTOHSCROLL|WS_TABSTOP);
        wchar_t prompt[36];swprintf_s(prompt,L"Website for tab %d",i+1);
        SendMessage(edit,EM_SETLIMITTEXT,webAddressSize-1,0);
        SendMessage(edit,EM_SETCUEBANNER,FALSE,(LPARAM)prompt);
    }
    caption(L"Spotify track",479);
    wchar_t track[80]=L"";
    if(station.track[0])swprintf_s(track,L"https://open.spotify.com/track/%s",station.track);
    HWND edit=control(WC_EDIT,track,456,218,479,470,32,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(edit,EM_SETLIMITTEXT,webAddressSize-1,0);
    SendMessage(edit,EM_SETCUEBANNER,FALSE,(LPARAM)L"Paste a Spotify track link");
    control(WC_STATIC,L"",455,38,523,650,43);
    if(SpotifyPlayback::configured())control(WC_BUTTON,L"Connect Spotify",454,38,571,174,32,WS_TABSTOP);
    updateSpotifySettings();
}
static bool launcherCommand(HWND window,int id){
    bool browse=id>=421&&id<=424,app=id>=430&&id<=433,website=id>=434&&id<=437;
    if(!browse&&!app&&!website)return false;
    int slot=id-(browse?421:app?430:434);
    int nameId=401+slot*2,targetId=nameId+1;
    if(website){
        shortcutWebsite[slot]=true;
        wchar_t current[MAX_PATH];
        GetDlgItemText(window,targetId,current,ARRAYSIZE(current));
        if(_wcsnicmp(current,L"http://",7)&&_wcsnicmp(current,L"https://",8)){
            SetDlgItemText(window,targetId,L"https://");
            SetDlgItemText(window,nameId,L"");
        }
        HWND target=GetDlgItem(window,targetId);
        SetFocus(target);
        SendMessage(target,EM_SETSEL,0,-1);
    }else{
        AppTarget chosen={};
        if(browse?browseTarget(chosen):pickTarget(chosen)){
            shortcutWebsite[slot]=false;
            SetDlgItemText(window,nameId,chosen.name);
            SetDlgItemText(window,targetId,chosen.path);
        }
    }
    return true;
}
static bool workstationCommand(HWND window,int id,int notification){
    if(id==440&&notification==CBN_SELCHANGE){
        int chosenRow=(int)SendDlgItemMessage(window,440,CB_GETCURSEL,0,0);
        int index=(int)SendDlgItemMessage(window,440,CB_GETITEMDATA,chosenRow,0);
        if(index>=0&&index<workstationCount&&readWorkstationDraft()){
            selectedStation=index;
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
    if(id>=460&&id<468&&selectedStation>=0){
        bool clear=id>=464;
        int index=id-(clear?464:460);
        AppTarget chosen={};
        if(clear||pickTarget(chosen)){
            draft.workstations[selectedStation].apps[index]=chosen;
            wchar_t empty[36];swprintf_s(empty,L"Choose app %d…",index+1);
            SetDlgItemText(window,460+index,chosen.path[0]?chosen.name:empty);
            EnableWindow(GetDlgItem(window,464+index),chosen.path[0]!=0);
        }
        return true;
    }
    return false;
}
