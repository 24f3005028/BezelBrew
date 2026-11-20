#pragma once

namespace WorkstationLinkDialog {
constexpr int inputId=2320,statusId=2321,openSpotifyId=2322,pasteId=2323;
struct Selection {
    bool spotify;
    wchar_t value[webAddressSize];
};

static INT_PTR CALLBACK procedure(HWND dialog,UINT message,WPARAM wp,LPARAM lp){
    using namespace SettingsSkin;
    auto* selection=(Selection*)GetWindowLongPtr(dialog,DWLP_USER);
    UINT dpi=GetDpiForWindow(dialog);
    auto px=[dpi](int value){return MulDiv(value,dpi,96);};
    switch(message){
    case WM_INITDIALOG:{
        selection=(Selection*)lp;
        SetWindowLongPtr(dialog,DWLP_USER,lp);
        SetWindowText(dialog,selection->spotify?L"Choose Spotify music":L"Website shortcut");
        chrome(dialog);
        RECT frame={0,0,px(486),px(232)},parent;
        AdjustWindowRectExForDpi(&frame,(DWORD)GetWindowLongPtr(dialog,GWL_STYLE),FALSE,
                                 (DWORD)GetWindowLongPtr(dialog,GWL_EXSTYLE),dpi);
        GetWindowRect(GetParent(dialog),&parent);
        int width=frame.right-frame.left,height=frame.bottom-frame.top;
        SetWindowPos(dialog,nullptr,(parent.left+parent.right-width)/2,(parent.top+parent.bottom-height)/2,
                     width,height,SWP_NOZORDER|SWP_NOACTIVATE);
        auto child=[&](Kind kind,const wchar_t* value,int id,int x,int y,int w,int h,bool primary=false){
            const wchar_t* cls=kind==Edit?WC_EDIT:kind==Button?WC_BUTTON:WC_STATIC;
            DWORD style=kind==Edit?ES_AUTOHSCROLL|WS_TABSTOP:kind==Button?WS_TABSTOP:SS_NOPREFIX;
            HWND control=CreateWindow(cls,value,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(h),
                                      dialog,(HMENU)(INT_PTR)id,GetModuleHandle(0),nullptr);
            prepare(control,kind,RECT{},nullptr,nullptr,primary);
            return control;
        };
        child(Text,selection->spotify?
              L"Find a track or playlist in Spotify. Choose Share → Copy link, then paste it here.":
              L"Enter the website you want this workstation to open.",0,20,18,446,44);
        if(selection->spotify){
            child(Button,L"Open Spotify",openSpotifyId,20,65,118,32);
            child(Button,L"Paste link",pasteId,148,65,96,32);
        }
        HWND edit=child(Edit,selection->value,inputId,30,117,426,22);
        SendMessage(edit,EM_SETLIMITTEXT,webAddressSize-1,0);
        SendMessage(edit,EM_SETCUEBANNER,TRUE,(LPARAM)(selection->spotify?L"Spotify track or playlist link":L"example.com"));
        child(Text,L"",statusId,20,150,446,26);
        child(Button,L"Cancel",IDCANCEL,254,182,88,34);
        child(Button,selection->spotify?L"Use selection":L"Use website",IDOK,352,182,114,34,true);
        SendMessage(dialog,DM_SETDEFID,IDOK,0);
        SetFocus(edit);
        SendMessage(edit,EM_SETSEL,0,-1);
        return FALSE;
    }
    case WM_COMMAND:{
        int id=LOWORD(wp);
        HWND edit=GetDlgItem(dialog,inputId);
        if(id==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}
        if(id==openSpotifyId&&selection&&selection->spotify){
            if(!LaunchTargets::open(dialog,L"spotify:")&&!LaunchTargets::open(dialog,L"https://open.spotify.com/search"))
                SetDlgItemText(dialog,statusId,L"Spotify couldn’t open. You can still paste a link.");
            return TRUE;
        }
        if(id==pasteId&&selection&&selection->spotify){
            SetFocus(edit);
            SendMessage(edit,EM_SETSEL,0,-1);
            SendMessage(edit,WM_PASTE,0,0);
            return TRUE;
        }
        if(id==IDOK&&selection){
            wchar_t input[webAddressSize],url[webAddressSize],spotify[spotifyUriSize];
            GetWindowText(edit,input,ARRAYSIZE(input));
            bool valid=selection->spotify?LaunchTargets::spotifyItem(input,spotify):LaunchTargets::webAddress(input,url);
            if(!valid){
                SetDlgItemText(dialog,statusId,selection->spotify?
                    L"Paste a Spotify track or playlist link." : L"Enter a valid http or https website.");
                SetFocus(edit);
                return TRUE;
            }
            wcscpy_s(selection->value,selection->spotify?spotify:url);
            EndDialog(dialog,IDOK);
            return TRUE;
        }
        break;
    }
    case WM_ERASEBKGND:return TRUE;
    case WM_PAINT:case WM_PRINTCLIENT:{
        PAINTSTRUCT ps;
        HDC dc=message==WM_PAINT?BeginPaint(dialog,&ps):(HDC)wp;
        RECT area;GetClientRect(dialog,&area);
        buffered(dc,area.right,area.bottom,[&](HDC frame){
            Graphics g(frame);
            g.Clear(surface());
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            rect(g,px(20),px(109),px(446),px(35),px(8),field(),outline());
        });
        if(message==WM_PAINT)EndPaint(dialog,&ps);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORDLG:
        return controlColors((HDC)wp,dialogBrush,surface());
    case WM_CTLCOLOREDIT:return controlColors((HDC)wp,inputBrush,field());
    }
    return FALSE;
}

static bool show(HWND owner,Selection& selection){
    struct alignas(DWORD) Template {DLGTEMPLATE dialog;WORD menu,windowClass,title;} layout={};
    layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;
    return DialogBoxIndirectParam(GetModuleHandle(0),&layout.dialog,owner,procedure,(LPARAM)&selection)==IDOK;
}
}
