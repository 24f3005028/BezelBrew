#pragma once

namespace LauncherDialog {
constexpr int listId=2301, searchId=2302, extraId=2303, statusId=2304;
struct Dialog {
    const Workstation* stations;
    AppTarget* apps=nullptr;
    int appCount=0,index=-1;
    int divider=0;
    bool empty=false;
    HFONT font=nullptr,strongFont=nullptr,headingFont=nullptr,detailFont=nullptr;
    HBRUSH inputBrush=nullptr,surfaceBrush=nullptr;
    AppTarget chosen={};
    LauncherIcons::Entry appIcons[32];
    unsigned nextIcon=0;
};

static void loadApps(Dialog& state){
    IShellItem* folder=nullptr;
    if(FAILED(SHGetKnownFolderItem(FOLDERID_AppsFolder,KF_FLAG_DEFAULT,nullptr,IID_PPV_ARGS(&folder))))return;
    IEnumShellItems* list=nullptr;
    HRESULT bound=folder->BindToHandler(nullptr,BHID_EnumItems,IID_PPV_ARGS(&list));
    folder->Release();
    if(FAILED(bound))return;
    constexpr int capacity=1024;
    state.apps=(AppTarget*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(AppTarget)*capacity);
    IShellItem* item=nullptr;
    while(state.apps&&state.appCount<capacity&&list->Next(1,&item,nullptr)==S_OK){
        PWSTR label=nullptr,relative=nullptr;
        if(SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY,&label))&&label&&*label&&
           SUCCEEDED(item->GetDisplayName(SIGDN_PARENTRELATIVEPARSING,&relative))&&relative&&*relative){
            auto& entry=state.apps[state.appCount];
            if(swprintf_s(entry.path,L"shell:AppsFolder\\%s",relative)>0){
                wcsncpy_s(entry.name,label,_TRUNCATE);
                state.appCount++;
            }
        }
        CoTaskMemFree(label);
        CoTaskMemFree(relative);
        item->Release();
    }
    list->Release();
}
static bool contains(const wchar_t* text,const wchar_t* needle){
    size_t length=wcslen(needle);
    if(!length)return true;
    for(;*text;text++)if(!_wcsnicmp(text,needle,length))return true;
    return false;
}
static Bitmap* appImage(Dialog& state,const wchar_t* target){
    for(auto& entry:state.appIcons)if(!wcscmp(entry.target,target))return entry.image;
    auto& entry=state.appIcons[state.nextIcon++%ARRAYSIZE(state.appIcons)];
    return LauncherIcons::image(entry,target);
}
static void populate(HWND dialog,Dialog& state){
    HWND list=GetDlgItem(dialog,listId);
    SendMessage(list,WM_SETREDRAW,FALSE,0);
    SendMessage(list,LB_RESETCONTENT,0,0);
    wchar_t query[80]={};
    GetDlgItemText(dialog,searchId,query,ARRAYSIZE(query));
    int count=state.stations?workstationCount:state.appCount;
    for(int i=0;i<count;i++){
        const wchar_t* label=state.stations?state.stations[i].name:state.apps[i].name;
        if(!*label||!contains(label,query))continue;
        LRESULT row=SendMessage(list,LB_ADDSTRING,0,(LPARAM)label);
        if(row>=0)SendMessage(list,LB_SETITEMDATA,row,i);
    }
    int rows=(int)SendMessage(list,LB_GETCOUNT,0,0);
    if(rows>0)SendMessage(list,LB_SETCURSEL,0,0);
    SendMessage(list,WM_SETREDRAW,TRUE,0);
    if(!state.stations){
        ShowWindow(list,rows?SW_SHOW:SW_HIDE);
        ShowWindow(GetDlgItem(dialog,statusId),rows?SW_HIDE:SW_SHOW);
        InvalidateRect(dialog,nullptr,FALSE);
    }
    InvalidateRect(list,nullptr,TRUE);
    EnableWindow(GetDlgItem(dialog,IDOK),rows>0);
}
static void describe(const Workstation& station,wchar_t (&summary)[96]){
    int apps=0,tabs=0;
    for(const auto& app:station.apps)if(app.path[0])apps++;
    for(const auto& tab:station.tabs)if(tab[0])tabs++;
    if(!apps&&!tabs&&!station.track[0]){
        wcscpy_s(summary,L"No items added");
        return;
    }
    if(apps)swprintf_s(summary,L"%d app%s",apps,apps==1?L"":L"s");
    if(tabs){
        wchar_t count[24];
        swprintf_s(count,L"%s%d tab%s",apps?L"  ·  ":L"",tabs,tabs==1?L"":L"s");
        wcscat_s(summary,count);
    }
    if(station.track[0])wcscat_s(summary,apps||tabs?L"  ·  Spotify":L"Spotify");
}
static INT_PTR CALLBACK procedure(HWND dialog,UINT message,WPARAM wp,LPARAM lp){
    using namespace SettingsSkin;
    auto* state=(Dialog*)GetWindowLongPtr(dialog,DWLP_USER);
    int dpi=GetDpiForWindow(dialog);
    auto px=[dpi](int dip){return MulDiv(dip,dpi,96);};
    switch(message){
    case WM_INITDIALOG:{
        state=(Dialog*)lp;
        SetWindowLongPtr(dialog,DWLP_USER,lp);
        SetWindowText(dialog,state->stations?L"Quick Open · Workstations":L"Choose an app");
        chrome(dialog);
        auto font=[&](int size,int weight){
            return CreateFont(-px(size),0,0,0,weight,0,0,0,DEFAULT_CHARSET,0,0,
                              ANTIALIASED_QUALITY,0,L"Segoe UI");
        };
        state->font=font(14,FW_NORMAL);
        state->strongFont=font(14,FW_SEMIBOLD);
        state->headingFont=font(19,FW_SEMIBOLD);
        state->detailFont=font(12,FW_NORMAL);
        state->inputBrush=CreateSolidBrush(rgb(field()));
        state->surfaceBrush=CreateSolidBrush(rgb(surface()));
        int stationCount=0;
        if(state->stations){
            for(int i=0;i<workstationCount;i++)if(state->stations[i].name[0])stationCount++;
            state->empty=stationCount==0;
        }
        int rows=stationCount<5?stationCount:5;
        int listHeight=state->stations?rows*48:252;
        int footer=state->empty?145:state->stations?55+listHeight+22:336;
        state->divider=footer-12;
        RECT frame={0,0,px(450),px(footer+51)},work;
        AdjustWindowRectExForDpi(&frame,(DWORD)GetWindowLongPtr(dialog,GWL_STYLE),FALSE,
                                 (DWORD)GetWindowLongPtr(dialog,GWL_EXSTYLE),dpi);
        MONITORINFO monitor={sizeof(monitor)};
        GetMonitorInfo(MonitorFromWindow(GetParent(dialog),MONITOR_DEFAULTTONEAREST),&monitor);
        work=monitor.rcWork;
        int width=frame.right-frame.left,height=frame.bottom-frame.top;
        SetWindowPos(dialog,nullptr,work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,
                     width,height,SWP_NOZORDER|SWP_NOACTIVATE);
        auto add=[&](const wchar_t* type,const wchar_t* caption,int id,int x,int y,int w,int h,DWORD style){
            return CreateWindow(type,caption,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(h),
                                dialog,(HMENU)(INT_PTR)id,GetModuleHandle(0),nullptr);
        };
        auto label=[&](const wchar_t* caption,int id,int x,int y,int w,int h,bool heading=false,DWORD style=0){
            HWND child=add(WC_STATIC,caption,id,x,y,w,h,SS_NOPREFIX|style);
            prepare(child,Text,RECT{},state->font,state->headingFont,heading);
        };
        auto button=[&](const wchar_t* caption,int id,int x,int w,bool primary=false){
            HWND child=add(WC_BUTTON,caption,id,x,footer,w,34,WS_TABSTOP|(primary?BS_DEFPUSHBUTTON:BS_PUSHBUTTON));
            prepare(child,Button,RECT{},state->font,state->strongFont,primary);
        };
        if(state->empty){
            label(L"No workstations yet",2305,80,25,348,27,true);
            label(L"Save a set of apps, browser tabs, and a Spotify track to open together.",statusId,80,58,332,48);
            button(L"Close",IDCANCEL,157,85);
            button(L"Create workstation…",extraId,253,177,true);
            SendMessage(dialog,DM_SETDEFID,extraId,0);
            SetFocus(GetDlgItem(dialog,extraId));
            return FALSE;
        }
        if(state->stations){
            label(L"Choose a workstation",2305,20,17,410,27,true);
        }else{
            HWND search=add(WC_EDIT,L"",searchId,51,25,368,24,ES_AUTOHSCROLL|WS_TABSTOP);
            prepare(search,Edit,RECT{},state->font,state->strongFont);
            SendMessage(search,EM_SETLIMITTEXT,79,0);
            SendMessage(search,EM_SETCUEBANNER,TRUE,(LPARAM)L"Search installed apps");
        }
        HWND list=add(WC_LISTBOX,L"",listId,20,state->stations?55:63,410,listHeight,LBS_NOTIFY|LBS_SORT|LBS_OWNERDRAWFIXED|
                      LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|WS_VSCROLL|WS_TABSTOP);
        SetWindowTheme(list,L"DarkMode_Explorer",nullptr);
        SendMessage(list,WM_SETFONT,(WPARAM)state->font,FALSE);
        SendMessage(list,LB_SETITEMHEIGHT,0,px(state->stations?48:42));
        if(!state->stations)label(L"No matching apps",statusId,20,164,410,24,false,SS_CENTER);
        button(state->stations?L"Manage workstations":L"Browse files…",extraId,20,165);
        button(L"Cancel",IDCANCEL,247,78);
        button(state->stations?L"Run":L"Choose",IDOK,336,94,true);
        if(!state->stations)loadApps(*state);
        populate(dialog,*state);
        SendMessage(dialog,DM_SETDEFID,IDOK,0);
        return TRUE;
    }
    case WM_COMMAND:{
        if(!state)break;
        int id=LOWORD(wp),notification=HIWORD(wp);
        if(id==searchId&&notification==EN_CHANGE){if(GetDlgItem(dialog,listId))populate(dialog,*state);return TRUE;}
        if(id==IDCANCEL||id==extraId){EndDialog(dialog,id);return TRUE;}
        if(id==IDOK||(id==listId&&notification==LBN_DBLCLK)){
            int row=(int)SendDlgItemMessage(dialog,listId,LB_GETCURSEL,0,0);
            if(row<0)return TRUE;
            state->index=(int)SendDlgItemMessage(dialog,listId,LB_GETITEMDATA,row,0);
            if(state->index<0||state->index>=(state->stations?workstationCount:state->appCount))return TRUE;
            if(!state->stations)state->chosen=state->apps[state->index];
            EndDialog(dialog,IDOK);
            return TRUE;
        }
        break;
    }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:case WM_PRINTCLIENT:{
        if(!state)break;
        PAINTSTRUCT ps;
        HDC dc=message==WM_PAINT?BeginPaint(dialog,&ps):(HDC)wp;
        RECT area;GetClientRect(dialog,&area);
        buffered(dc,area.right,area.bottom,[&](HDC frame){
            Graphics g(frame);
            g.Clear(surface());
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            SettingsSkin::line(g,px(20),px(state->divider),area.right-px(20),px(state->divider),outline(),1);
            if(!state->stations){
                rect(g,px(20),px(18),px(410),px(36),px(8),field(),
                     GetFocus()==GetDlgItem(dialog,searchId)?brand():outline());
                Pen pen(muted(),px(1));
                g.DrawEllipse(&pen,px(32),px(29),px(10),px(10));
                SettingsSkin::line(g,px(40),px(37),px(44),px(41),muted(),px(1));
            }
            if(state->empty){
                rect(g,px(20),px(23),px(45),px(45),px(12),Color(255,43,45,69));
                glyph(g,3,px(30),px(33),px(25),brand());
            }
        });
        if(message==WM_PAINT)EndPaint(dialog,&ps);
        return TRUE;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        if(state){
            bool edit=message==WM_CTLCOLOREDIT;
            return controlColors((HDC)wp,edit?state->inputBrush:state->surfaceBrush,edit?field():surface());
        }
        break;
    case WM_DRAWITEM:{
        if(!state)break;
        auto& draw=*(DRAWITEMSTRUCT*)lp;
        if(draw.CtlType!=ODT_LISTBOX)break;
        RECT area=draw.rcItem;
        FillRect(draw.hDC,&area,state->surfaceBrush);
        if(draw.itemID==(UINT)-1||draw.itemData>=(UINT_PTR)(state->stations?workstationCount:state->appCount))return TRUE;
        bool selected=(draw.itemState&ODS_SELECTED)!=0;
        Graphics g(draw.hDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        int top=area.top+px(2),left=area.left+px(1);
        int height=area.bottom-area.top-px(4);
        rect(g,left,top,area.right-left-px(2),height,px(8),
             selected?Color(255,43,45,69):field(),selected?brand():outline());
        if(state->stations){
            const auto& station=state->stations[draw.itemData];
            glyph(g,3,left+px(10),top+px(11),px(22),selected?brand():muted());
            text(g,draw.hDC,station.name,RECT{left+px(43),top+px(3),area.right-px(30),top+px(23)},
                 state->font,foreground());
            wchar_t summary[96]={};
            describe(station,summary);
            text(g,draw.hDC,summary,RECT{left+px(43),top+px(23),area.right-px(30),top+px(40)},
                 state->detailFont,muted());
        }else{
            const auto& app=state->apps[draw.itemData];
            if(Bitmap* image=appImage(*state,app.path)){
                int icon=px(24);
                g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                g.DrawImage(image,Rect(left+px(11),top+(height-icon)/2,icon,icon));
            }
            text(g,draw.hDC,app.name,RECT{left+px(46),top,area.right-px(30),top+height},
                 state->font,foreground());
        }
        if(selected)glyph(g,9,area.right-px(25),top+(height-px(17))/2,px(17),brand());
        return TRUE;
    }
    case WM_NCDESTROY:
        if(state){
            for(auto& entry:state->appIcons)LauncherIcons::release(entry);
            if(state->apps)HeapFree(GetProcessHeap(),0,state->apps);
            if(state->font)DeleteObject(state->font);
            if(state->strongFont)DeleteObject(state->strongFont);
            if(state->headingFont)DeleteObject(state->headingFont);
            if(state->detailFont)DeleteObject(state->detailFont);
            if(state->inputBrush)DeleteObject(state->inputBrush);
            if(state->surfaceBrush)DeleteObject(state->surfaceBrush);
        }
        break;
    }
    return FALSE;
}
static INT_PTR show(HWND owner,Dialog& state){
    struct alignas(DWORD) Template {DLGTEMPLATE dialog;WORD menu,windowClass,title;} layout={};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;
    return DialogBoxIndirectParam(GetModuleHandle(0),&layout.dialog,owner,procedure,(LPARAM)&state);
}
}
