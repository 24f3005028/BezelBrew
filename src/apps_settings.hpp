#pragma once

static int editingApp=-1,appsListScroll=0;
static bool appWebsite=false;

static const wchar_t* trimAppField(wchar_t* value){
    size_t end=wcslen(value);
    while(end&&value[end-1]<=L' ')value[--end]=0;
    return value+wcsspn(value,L" \t");
}

static bool readAppDraft(){
    if(editingApp<0)return true;
    AppTarget item={};
    wchar_t name[ARRAYSIZE(item.name)],path[ARRAYSIZE(item.path)];
    GetDlgItemText(settingsWindow,401,name,ARRAYSIZE(name));
    GetDlgItemText(settingsWindow,402,path,ARRAYSIZE(path));
    const wchar_t* title=trimAppField(name);
    const wchar_t* target=trimAppField(path);
    if(!target[0]){
        settingsError(L"Choose what this shortcut opens.");
        SetFocus(GetDlgItem(settingsWindow,402));
        return false;
    }
    wcscpy_s(item.name,title);
    if(appWebsite||!_wcsnicmp(target,L"http:",5)||!_wcsnicmp(target,L"https:",6)){
        wchar_t url[webAddressSize];
        if(!LaunchTargets::webAddress(target,url)||wcslen(url)>=ARRAYSIZE(item.path)){
            settingsError(L"Enter a valid website address.");
            SetFocus(GetDlgItem(settingsWindow,402));
            return false;
        }
        wcscpy_s(item.path,url);
        if(!item.name[0]){
            const wchar_t* host=wcsstr(url,L"://")+3;
            size_t length=wcscspn(host,L"/?#");
            wcsncpy_s(item.name,host,length<ARRAYSIZE(item.name)?length:ARRAYSIZE(item.name)-1);
        }
    }else{
        wcscpy_s(item.path,target);
    }
    if(!item.name[0]){
        settingsError(L"Enter a name for this shortcut.");
        SetFocus(GetDlgItem(settingsWindow,401));
        return false;
    }
    draft.apps[editingApp]=item;
    return true;
}

static void appsSettings(){
    if(editingApp<0){
        HWND add=control(WC_BUTTON,L"+  Add shortcut",420,38,110,160,36,WS_TABSTOP);
        int count=0;
        for(int i=0;i<appLimit;i++){
            const auto& app=draft.apps[i];
            if(!app.path[0])continue;
            control(WC_BUTTON,app.name[0]?app.name:L"Shortcut",appCardBase+i,
                38+(count%2)*331,161+(count/2)*55,319,45,WS_TABSTOP);
            count++;
        }
        EnableWindow(add,count<appLimit);
        if(!count)control(WC_STATIC,L"Add an app, website, file, or folder to open from the island.",0,38,166,650,38);
        else if(count==appLimit)control(WC_STATIC,L"32 shortcuts added. Remove one to make room.",0,214,112,455,35);
        return;
    }
    const auto& app=draft.apps[editingApp];
    control(WC_STATIC,app.path[0]?L"Edit shortcut":L"New shortcut",660,38,113,650,25);
    caption(L"Name",155);
    HWND name=control(WC_EDIT,app.name,401,218,155,470,34,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(name,EM_SETLIMITTEXT,ARRAYSIZE(app.name)-1,0);
    SendMessage(name,EM_SETCUEBANNER,FALSE,(LPARAM)L"Name shown on the island");
    caption(L"Opens",207);
    HWND target=control(WC_EDIT,app.path,402,218,207,470,34,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(target,EM_SETLIMITTEXT,ARRAYSIZE(app.path)-1,0);
    SendMessage(target,EM_SETCUEBANNER,FALSE,(LPARAM)L"Paste a path, website, or app link");

    control(WC_BUTTON,L"Choose app…",430,218,255,113,32,WS_TABSTOP);
    control(WC_BUTTON,L"Website",434,338,255,95,32,WS_TABSTOP);
    control(WC_BUTTON,L"Browse file…",421,440,255,116,32,WS_TABSTOP);
    control(WC_BUTTON,L"Browse folder…",422,563,255,125,32,WS_TABSTOP);
    if(app.path[0])control(WC_BUTTON,L"Remove shortcut",425,38,321,146,34,WS_TABSTOP);
    control(WC_BUTTON,L"Cancel edit",424,486,321,92,34,WS_TABSTOP);
    control(WC_BUTTON,L"Done",423,589,321,99,34,WS_TABSTOP);
}

static void closeAppEditor(){
    if(editingApp<0)return;
    int slot=editingApp;
    editingApp=-1;
    populateSettings();
    SettingsSkin::scroll=appsListScroll;
    SettingsSkin::layout();
    HWND card=GetDlgItem(settingsWindow,appCardBase+slot);
    if(!card||!IsWindowVisible(card))card=GetDlgItem(settingsWindow,420);
    if(IsWindowVisible(card)&&IsWindowEnabled(card))SetFocus(card);
}

static bool appsCommand(HWND window,int id,int notification){
    if(notification!=BN_CLICKED)return false;
    if(editingApp<0){
        int slot=-1;
        if(id==420){
            for(int i=0;i<appLimit;i++)if(!draft.apps[i].path[0]){slot=i;break;}
        }else if(id>=appCardBase&&id<appCardBase+appLimit&&draft.apps[id-appCardBase].path[0]){
            slot=id-appCardBase;
        }
        if(slot<0)return false;
        appsListScroll=SettingsSkin::scroll;
        editingApp=slot;
        appWebsite=false;
        populateSettings();
        SetFocus(GetDlgItem(window,401));
        return true;
    }
    if(id==423){
        if(readAppDraft())closeAppEditor();
    }else if(id==424||id==425){
        if(id==425){
            for(int i=editingApp;i+1<appLimit;i++)draft.apps[i]=draft.apps[i+1];
            draft.apps[appLimit-1]={};
        }
        closeAppEditor();
    }else if(id==434){
        appWebsite=true;
        wchar_t current[MAX_PATH];
        GetDlgItemText(window,402,current,ARRAYSIZE(current));
        bool web=!_wcsnicmp(current,L"https://",8)||!_wcsnicmp(current,L"http://",7);
        if(!web)SetDlgItemText(window,402,L"https://");
        HWND target=GetDlgItem(window,402);
        SetFocus(target);
        SendMessage(target,EM_SETSEL,web?0:8,web?-1:8);
    }else if(id==430||id==421||id==422){
        AppTarget chosen={};
        const COMDLG_FILTERSPEC allFiles={L"All files",L"*.*"};
        bool selected=id==430?pickTarget(chosen):id==422?
            selectFile(window,nullptr,0,chosen.path,FOS_PICKFOLDERS):
            selectFile(window,&allFiles,1,chosen.path,FOS_NODEREFERENCELINKS);
        if(selected){
            appWebsite=false;
            if(!chosen.name[0]){
                const wchar_t* leaf=wcsrchr(chosen.path,L'\\');
                leaf=leaf&&leaf[1]?leaf+1:chosen.path;
                wcsncpy_s(chosen.name,leaf,_TRUNCATE);
            }
            if(!GetWindowTextLength(GetDlgItem(window,401)))SetDlgItemText(window,401,chosen.name);
            SetDlgItemText(window,402,chosen.path);
            settingsError(L"");
        }
    }else return false;
    return true;
}
