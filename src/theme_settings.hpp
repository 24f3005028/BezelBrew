#pragma once

static bool readDraft();
static int selectedTheme=-1;
static wchar_t themeNameDraft[64]={};
constexpr int themeListId=8300,themeNameId=8301,themeSaveId=8302,themeImportId=8303,
              themeExportId=8304,themeRemoveId=8305;
static const COMDLG_FILTERSPEC themeFileType={L"BezelBrew theme",L"*.bbtheme"};

static ThemeLook currentLook(){
    return {draft.glassMode,draft.backgroundRgb,draft.fontRgb,draft.accentRgb,draft.glassTintRgb,
            draft.blurPasses,draft.tintPercent,draft.sizePercent,draft.textPercent,draft.cornerDip,
            draftAnimations,draftDocked};
}
static void loadLook(const ThemeLook& look){
    draft.glassMode=look.glass!=0;
    draft.backgroundRgb=look.background;
    draft.fontRgb=look.text;
    draft.accentRgb=look.accent;
    draft.glassTintRgb=look.tint;
    draft.blurPasses=look.blur;
    draft.tintPercent=look.strength;
    draft.sizePercent=look.size;
    draft.textPercent=look.textSize;
    draft.cornerDip=look.corner;
    draftAnimations=look.animations!=0;
    draftDocked=look.docked!=0;
}
static void selectTheme(int index){
    selectedTheme=index;
    themeNameDraft[0]=0;
    if(index>=0){
        const auto& theme=draft.themes[index];
        wcscpy_s(themeNameDraft,theme.name);
        loadLook(theme.look);
    }
}
static void matchEquippedTheme(){
    selectedTheme=-1;
    themeNameDraft[0]=0;
    ThemeLook look=currentLook();
    wchar_t encoded[160];
    SavedThemes::encode(look,encoded);
    for(int i=0;i<savedThemeCount;i++)if(draft.themes[i].name[0]){
        wchar_t saved[160];
        SavedThemes::encode(draft.themes[i].look,saved);
        if(!wcscmp(encoded,saved)){
            selectedTheme=i;
            wcscpy_s(themeNameDraft,draft.themes[i].name);
            return;
        }
    }
}
static void fillThemeList(HWND list){
    SendMessage(list,CB_RESETCONTENT,0,0);
    choice(list,L"Saved themes");
    SendMessage(list,CB_SETITEMDATA,0,-1);
    int selected=0;
    for(int i=0;i<savedThemeCount;i++)if(draft.themes[i].name[0]){
        int row=(int)SendMessage(list,CB_ADDSTRING,0,(LPARAM)draft.themes[i].name);
        SendMessage(list,CB_SETITEMDATA,row,i);
        if(selectedTheme==i)selected=row;
    }
    SendMessage(list,CB_SETCURSEL,selected,0);
}
static void themeControls(){
    HWND list=control(WC_COMBOBOX,L"",themeListId,38,190,305,240,CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL);
    fillThemeList(list);
    control(WC_BUTTON,L"Import…",themeImportId,354,190,103,32,WS_TABSTOP);
    HWND exportButton=control(WC_BUTTON,L"Export…",themeExportId,468,190,103,32,WS_TABSTOP);
    HWND remove=control(WC_BUTTON,L"Remove",themeRemoveId,582,190,106,32,WS_TABSTOP);
    EnableWindow(exportButton,selectedTheme>=0);
    EnableWindow(remove,selectedTheme>=0);
    HWND name=control(WC_EDIT,themeNameDraft,themeNameId,38,230,495,32,ES_AUTOHSCROLL|WS_TABSTOP);
    SendMessage(name,EM_SETLIMITTEXT,63,0);
    SendMessage(name,EM_SETCUEBANNER,FALSE,(LPARAM)L"Name this theme");
    control(WC_BUTTON,L"Save theme",themeSaveId,545,230,143,32,WS_TABSTOP);
}
static int themeWithName(const wchar_t* name){
    for(int i=0;i<savedThemeCount;i++)if(draft.themes[i].name[0]&&!_wcsicmp(draft.themes[i].name,name))return i;
    return -1;
}
static int freeTheme(){
    for(int i=0;i<savedThemeCount;i++)if(!draft.themes[i].name[0])return i;
    settingsError(L"Your theme library is full. Remove a theme first.");
    return -1;
}
static bool storeTheme(int index,const SavedTheme& theme){
    SavedTheme previous=draft.themes[index];
    draft.themes[index]=theme;
    if(!saveEnabled||!configFile[0]||!unicodeConfig(configFile)||!SavedThemes::writeLibrary(configFile,draft.themes)){
        draft.themes[index]=previous;
        settingsError(L"Couldn’t save the theme library. Check the settings folder permissions.");
        return false;
    }
    prefs.themes[index]=theme;
    return true;
}
static bool selectThemeExportPath(HWND owner,const wchar_t* name,wchar_t (&path)[MAX_PATH]){
    IFileSaveDialog* dialog=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileSaveDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))return false;
    FILEOPENDIALOGOPTIONS options=0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_OVERWRITEPROMPT|FOS_NOCHANGEDIR);
    dialog->SetFileTypes(1,&themeFileType);
    dialog->SetDefaultExtension(L"bbtheme");
    wchar_t filename[64];
    wcscpy_s(filename,name);
    for(wchar_t* c=filename;*c;c++)if(wcschr(L"<>:\"/\\|?*",*c))*c=L'_';
    dialog->SetFileName(filename);
    IShellItem* item=nullptr;
    bool selected=false;
    if(SUCCEEDED(dialog->Show(owner))&&SUCCEEDED(dialog->GetResult(&item))){
        PWSTR value=nullptr;
        if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&value))){
            if(wcslen(value)<ARRAYSIZE(path)){wcscpy_s(path,value);selected=true;}
            else settingsError(L"Choose a file with a shorter path.");
            CoTaskMemFree(value);
        }
        item->Release();
    }
    dialog->Release();
    return selected;
}
static bool themeCommand(HWND window,int id,int notification){
    switch(id){
    case themeListId:{
        if(notification!=CBN_SELCHANGE)return false;
        LRESULT row=SendDlgItemMessage(window,themeListId,CB_GETCURSEL,0,0);
        int index=(int)SendDlgItemMessage(window,themeListId,CB_GETITEMDATA,row,0);
        GetDlgItemText(window,113,draft.displayName,ARRAYSIZE(draft.displayName));
        if(index<0||index>=savedThemeCount||!draft.themes[index].name[0]){
            selectTheme(-1);
            SetDlgItemText(window,themeNameId,L"");
            EnableWindow(GetDlgItem(window,themeExportId),FALSE);
            EnableWindow(GetDlgItem(window,themeRemoveId),FALSE);
            return true;
        }
        selectTheme(index);
        populateSettings();
        SetFocus(GetDlgItem(window,themeListId));
        settingsError(L"Theme loaded. Select Save changes to apply it.");
        return true;
    }
    case themeSaveId:{
        if(!readDraft())return true;
        if(!SavedThemes::validName(themeNameDraft)){
            settingsError(L"Enter a theme name without leading or trailing spaces.");
            SetFocus(GetDlgItem(window,themeNameId));
            return true;
        }
        int index=themeWithName(themeNameDraft);
        if(index>=0&&index!=selectedTheme){
            settingsError(L"That name is in use. Select the saved theme to update it, or choose another name.");
            SetFocus(GetDlgItem(window,themeNameId));
            return true;
        }
        if(index<0)index=freeTheme();
        if(index<0)return true;
        SavedTheme theme={};
        wcscpy_s(theme.name,themeNameDraft);
        theme.look=currentLook();
        if(storeTheme(index,theme)){
            selectTheme(index);
            populateSettings();
            SetFocus(GetDlgItem(window,themeListId));
            settingsError(L"Theme saved. Select Save changes to apply it.");
        }
        return true;
    }
    case themeImportId:{
        int index=freeTheme();
        if(index<0)return true;
        wchar_t path[MAX_PATH];
        if(!selectFile(window,&themeFileType,1,path))return true;
        SavedTheme theme={};
        if(!SavedThemes::importFile(path,theme)){
            settingsError(L"That file isn’t a valid BezelBrew theme.");
            return true;
        }
        wchar_t name[64];
        wcscpy_s(name,theme.name);
        for(int suffix=2;themeWithName(theme.name)>=0;suffix++)swprintf_s(theme.name,L"%.53s (%d)",name,suffix);
        if(storeTheme(index,theme)){
            GetDlgItemText(window,113,draft.displayName,ARRAYSIZE(draft.displayName));
            selectTheme(index);
            populateSettings();
            SetFocus(GetDlgItem(window,themeListId));
            settingsError(L"Theme imported. Select Save changes to apply it.");
        }
        return true;
    }
    case themeExportId:{
        if(selectedTheme<0)return true;
        const SavedTheme theme=draft.themes[selectedTheme];
        wchar_t path[MAX_PATH];
        if(selectThemeExportPath(window,theme.name,path)){
            settingsError(SavedThemes::exportFile(path,theme)?L"Theme exported. You can share the .bbtheme file.":
                                                             L"Couldn’t export the theme to that file.");
        }
        return true;
    }
    case themeRemoveId:
        if(selectedTheme>=0&&storeTheme(selectedTheme,SavedTheme{})){
            selectTheme(-1);
            fillThemeList(GetDlgItem(window,themeListId));
            SetDlgItemText(window,themeNameId,L"");
            EnableWindow(GetDlgItem(window,themeExportId),FALSE);
            EnableWindow(GetDlgItem(window,themeRemoveId),FALSE);
            SetFocus(GetDlgItem(window,themeListId));
            settingsError(L"Theme removed from your library.");
        }
        return true;
    }
    return false;
}
