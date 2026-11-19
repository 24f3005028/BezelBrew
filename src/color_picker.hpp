#pragma once

namespace AccentPicker {
constexpr int hexId=2001, hueId=2002, planeId=2003, previewId=2004, firstSwatch=2010;
struct Swatch { unsigned hex; const wchar_t* name; };
constexpr Swatch swatches[]={
    {0xA4AEFF,L"Periwinkle"},{0x0A84FF,L"Blue"},{0x49D5BB,L"Mint"},
    {0x8DDA83,L"Green"},{0xFFC173,L"Amber"},{0xFF9283,L"Coral"},
    {0xFFB6D3,L"Light pink"},{0xD8DCEA,L"Silver"}
};
struct Selection {
    unsigned hex;
    const wchar_t* title;
    float hue=0, saturation=0, value=0;
    bool writingHex=false;
    unsigned shownHex=0xffffffff;
    Bitmap* gradient=nullptr;
    float gradientHue=-1;
    ~Selection(){delete gradient;}
};
static float unit(float value){return fminf(1.f,fmaxf(0.f,value));}
static Color hsv(float hue,float saturation,float value){
    float chroma=value*saturation;
    float second=chroma*(1.f-fabsf(fmodf(hue/60.f,2.f)-1.f));
    float min=value-chroma;
    float r=0,g=0,b=0;
    if(hue<60){r=chroma;g=second;}
    else if(hue<120){r=second;g=chroma;}
    else if(hue<180){g=chroma;b=second;}
    else if(hue<240){g=second;b=chroma;}
    else if(hue<300){r=second;b=chroma;}
    else{r=chroma;b=second;}
    return Color(255,(BYTE)lroundf((r+min)*255),(BYTE)lroundf((g+min)*255),(BYTE)lroundf((b+min)*255));
}
static void setHex(Selection& color,unsigned hex){
    Color rgb=SettingsSkin::colorFromHex(hex);
    float r=rgb.GetR()/255.f,g=rgb.GetG()/255.f,b=rgb.GetB()/255.f;
    float high=fmaxf(r,fmaxf(g,b)),delta=high-fminf(r,fminf(g,b));
    color.hex=hex;
    color.value=high;
    color.saturation=high>0?delta/high:0;
    if(delta>0){
        float sector=high==r?(g-b)/delta:high==g?2+(b-r)/delta:4+(r-g)/delta;
        color.hue=fmodf(360.f+sector*60.f,360.f);
    }
}
static void refresh(HWND dialog,Selection& color,bool rewrite){
    if(rewrite){
        wchar_t text[7],current[7];
        swprintf_s(text,L"%06X",color.hex);
        GetDlgItemText(dialog,hexId,current,ARRAYSIZE(current));
        if(wcscmp(text,current)){
            color.writingHex=true;
            SetDlgItemText(dialog,hexId,text);
            color.writingHex=false;
        }
        HWND confirm=GetDlgItem(dialog,IDOK);
        if(!IsWindowEnabled(confirm))EnableWindow(confirm,TRUE);
    }
    int hue=lroundf(color.hue);
    if(SendDlgItemMessage(dialog,hueId,TBM_GETPOS,0,0)!=hue)
        SendDlgItemMessage(dialog,hueId,TBM_SETPOS,TRUE,hue);
    InvalidateRect(GetDlgItem(dialog,planeId),nullptr,FALSE);
    if(color.shownHex==color.hex)return;
    InvalidateRect(GetDlgItem(dialog,previewId),nullptr,FALSE);
    for(int i=0;i<(int)ARRAYSIZE(swatches);i++){
        if(swatches[i].hex==color.shownHex||swatches[i].hex==color.hex)
            InvalidateRect(GetDlgItem(dialog,firstSwatch+i),nullptr,FALSE);
    }
    color.shownHex=color.hex;
}
static void updateFromHsv(HWND dialog,Selection& color){
    color.hex=hsv(color.hue,color.saturation,color.value).GetValue()&0xffffff;
    refresh(dialog,color,true);
}
static void drawGradient(Graphics& g,int width,int height,float hue){
    Rect bounds(0,0,width,height);
    LinearGradientBrush white(bounds,Color(255,255,255,255),hsv(hue,1,1),LinearGradientModeHorizontal);
    LinearGradientBrush black(bounds,Color(0,0,0,0),Color(255,0,0,0),LinearGradientModeVertical);
    g.FillRectangle(&white,bounds);
    g.FillRectangle(&black,bounds);
}
static LRESULT CALLBACK sampleProc(HWND h,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto& color=*(Selection*)data;
    HWND dialog=GetParent(h);
    int id=GetDlgCtrlID(h);
    bool plane=id==planeId;
    RECT r;
    GetClientRect(h,&r);
    auto scale=[&](int dip){return MulDiv(dip,GetDpiForWindow(h),96);};
    switch(msg){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:
    case WM_PRINTCLIENT:{
        using namespace SettingsSkin;
        PAINTSTRUCT ps;
        HDC dc=msg==WM_PAINT?BeginPaint(h,&ps):(HDC)wp;
        buffered(dc,r.right,r.bottom,[&](HDC frame){
            Graphics g(frame);
            g.Clear(surface());
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            if(plane){
                if(!color.gradient||color.gradientHue!=color.hue||
                   color.gradient->GetWidth()!=(UINT)r.right||color.gradient->GetHeight()!=(UINT)r.bottom){
                    delete color.gradient;
                    color.gradient=new Bitmap(r.right,r.bottom,PixelFormat32bppPARGB);
                    if(color.gradient&&color.gradient->GetLastStatus()==Ok){
                        Graphics cached(color.gradient);
                        drawGradient(cached,r.right,r.bottom,color.hue);
                        color.gradientHue=color.hue;
                    }else{
                        delete color.gradient;
                        color.gradient=nullptr;
                    }
                }
                if(color.gradient)g.DrawImage(color.gradient,0,0,r.right,r.bottom);
                else drawGradient(g,r.right,r.bottom,color.hue);
                float margin=scale(7);
                float x=margin+color.saturation*(r.right-2*margin);
                float y=margin+(1-color.value)*(r.bottom-2*margin);
                Pen dark(Color(255,15,15,20),(REAL)scale(4));
                Pen light(Color(255,255,255,255),(REAL)scale(2));
                g.DrawEllipse(&dark,x-margin,y-margin,2*margin,2*margin);
                g.DrawEllipse(&light,x-margin,y-margin,2*margin,2*margin);
                rect(g,0,0,r.right-1,r.bottom-1,0,Color(0,0,0,0),GetFocus()==h?brand():outline());
            }else if(id==previewId){
                rect(g,1,1,r.right-2,r.bottom-2,scale(8),colorFromHex(color.hex),outline());
            }else{
                Color sample=colorFromHex(swatches[id-firstSwatch].hex);
                bool selected=color.hex==swatches[id-firstSwatch].hex;
                rect(g,1,1,r.right-2,r.bottom-2,scale(8),sample,selected?foreground():outline());
                if(selected)glyph(g,9,(r.right-scale(18))*.5f,(r.bottom-scale(18))*.5f,scale(18),brandInk());
                if(GetFocus()==h)rect(g,scale(3),scale(3),r.right-scale(6),r.bottom-scale(6),scale(6),Color(0,0,0,0),brandInk());
            }
        });
        if(msg==WM_PAINT)EndPaint(h,&ps);
        return 0;
    }
    case WM_SETFOCUS:case WM_KILLFOCUS:InvalidateRect(h,nullptr,FALSE);break;
    case WM_GETDLGCODE:if(plane)return DLGC_WANTARROWS;break;
    case WM_KEYDOWN:
        if(plane){
            float step=GetKeyState(VK_SHIFT)<0?.1f:.02f;
            switch(wp){
            case VK_LEFT:color.saturation=unit(color.saturation-step);break;
            case VK_RIGHT:color.saturation=unit(color.saturation+step);break;
            case VK_UP:color.value=unit(color.value+step);break;
            case VK_DOWN:color.value=unit(color.value-step);break;
            default:return DefSubclassProc(h,msg,wp,lp);
            }
            updateFromHsv(dialog,color);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
        if(plane){
            if(msg==WM_LBUTTONDOWN){SetFocus(h);SetCapture(h);}
            if(GetCapture()==h){
                float margin=scale(7);
                color.saturation=unit((GET_X_LPARAM(lp)-margin)/fmaxf(1,r.right-2*margin));
                color.value=1-unit((GET_Y_LPARAM(lp)-margin)/fmaxf(1,r.bottom-2*margin));
                updateFromHsv(dialog,color);
                if(msg==WM_LBUTTONUP)ReleaseCapture();
            }
            return 0;
        }
        break;
    case WM_NCDESTROY:RemoveWindowSubclass(h,sampleProc,78);break;
    }
    return DefSubclassProc(h,msg,wp,lp);
}

struct Control {
    const wchar_t* type;
    const wchar_t* text;
    int id,x,y,w,h;
    DWORD style;
};
static constexpr Control controls[]={
    {WC_BUTTON,L"Saturation and brightness",planeId,20,20,350,146,WS_TABSTOP},
    {TRACKBAR_CLASS,L"Hue",hueId,16,176,358,32,TBS_NOTICKS|WS_TABSTOP},
    {WC_STATIC,L"Selected color",previewId,19,264,34,34,0},
    {WC_EDIT,L"",hexId,256,269,107,25,ES_AUTOHSCROLL|WS_TABSTOP},
    {WC_BUTTON,L"Cancel",IDCANCEL,162,322,80,34,WS_TABSTOP},
    {WC_BUTTON,L"Use color",IDOK,252,322,118,34,BS_DEFPUSHBUTTON|WS_TABSTOP}
};

static INT_PTR CALLBACK dialogProc(HWND dialog,UINT msg,WPARAM wp,LPARAM lp){
    using namespace SettingsSkin;
    auto* color=(Selection*)GetWindowLongPtr(dialog,DWLP_USER);
    UINT dpi=GetDpiForWindow(dialog);
    auto scale=[&](int dip){return MulDiv(dip,dpi,96);};
    switch(msg){
    case WM_INITDIALOG:{
        color=(Selection*)lp;
        SetWindowLongPtr(dialog,DWLP_USER,lp);
        SetWindowText(dialog,color->title);
        chrome(dialog);
        RECT owner,frame={0,0,scale(390),scale(376)};
        GetWindowRect(GetParent(dialog),&owner);
        AdjustWindowRectExForDpi(&frame,(DWORD)GetWindowLongPtr(dialog,GWL_STYLE),FALSE,
                                 (DWORD)GetWindowLongPtr(dialog,GWL_EXSTYLE),dpi);
        int w=frame.right-frame.left,h=frame.bottom-frame.top;
        SetWindowPos(dialog,nullptr,owner.left+(owner.right-owner.left-w)/2,
                     owner.top+(owner.bottom-owner.top-h)/2,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
        auto create=[&](const Control& spec){
            return CreateWindow(spec.type,spec.text,WS_CHILD|WS_VISIBLE|spec.style,scale(spec.x),
                                scale(spec.y),scale(spec.w),scale(spec.h),dialog,
                                (HMENU)(INT_PTR)spec.id,GetModuleHandle(0),nullptr);
        };
        for(const auto& spec:controls){
            if(spec.id==previewId){
                for(int i=0;i<(int)ARRAYSIZE(swatches);i++){
                    HWND swatch=create({WC_BUTTON,swatches[i].name,firstSwatch+i,20+i*45,218,35,32,WS_TABSTOP});
                    SetWindowSubclass(swatch,sampleProc,78,(DWORD_PTR)color);
                }
            }
            HWND child=create(spec);
            if(spec.id==planeId||spec.id==previewId)SetWindowSubclass(child,sampleProc,78,(DWORD_PTR)color);
            else{
                Kind kind=spec.id==hexId?Edit:spec.id==hueId?Hue:Button;
                prepare(child,kind,RECT{});
            }
        }
        SendDlgItemMessage(dialog,hexId,EM_SETLIMITTEXT,6,0);
        SendDlgItemMessage(dialog,hueId,TBM_SETRANGE,TRUE,MAKELPARAM(0,360));
        refresh(dialog,*color,true);
        return TRUE;
    }
    case WM_HSCROLL:
        if(color&&(HWND)lp==GetDlgItem(dialog,hueId)){
            color->hue=(float)SendDlgItemMessage(dialog,hueId,TBM_GETPOS,0,0);
            updateFromHsv(dialog,*color);
            return TRUE;
        }
        break;
    case WM_COMMAND:{
        int id=LOWORD(wp);
        if(id==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}
        if(!color)break;
        if(id==IDOK||id==hexId&&HIWORD(wp)==EN_CHANGE&&!color->writingHex){
            wchar_t text[7];
            unsigned value;
            GetDlgItemText(dialog,hexId,text,7);
            bool valid=parseHex(text,value);
            EnableWindow(GetDlgItem(dialog,IDOK),valid);
            if(valid){
                setHex(*color,value);
                if(id==IDOK)EndDialog(dialog,IDOK);
                else refresh(dialog,*color,false);
            }
            return TRUE;
        }
        if(id>=firstSwatch&&id<firstSwatch+(int)ARRAYSIZE(swatches)){
            setHex(*color,swatches[id-firstSwatch].hex);
            refresh(dialog,*color,true);
            return TRUE;
        }
        break;
    }
    case WM_ERASEBKGND:return TRUE;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORDLG:
        return controlColors((HDC)wp,dialogBrush,surface());
    case WM_CTLCOLOREDIT:
        return controlColors((HDC)wp,inputBrush,field());
    case WM_PAINT:case WM_PRINTCLIENT:{
        if(!color)break;
        PAINTSTRUCT ps;
        HDC dc=msg==WM_PAINT?BeginPaint(dialog,&ps):(HDC)wp;
        RECT bounds;
        GetClientRect(dialog,&bounds);
        buffered(dc,bounds.right,bounds.bottom,[&](HDC frame){
            Graphics g(frame);
            g.Clear(surface());
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            text(g,frame,L"Selected color",RECT{scale(62),scale(265),scale(191),scale(297)},settingsFont,foreground());
            text(g,frame,L"Hex",RECT{scale(205),scale(265),scale(241),scale(297)},settingsFont,muted());
            rect(g,scale(245),scale(265),scale(125),scale(32),scale(8),field(),
                 GetFocus()==GetDlgItem(dialog,hexId)?brand():outline());
        });
        if(msg==WM_PAINT)EndPaint(dialog,&ps);
        return TRUE;
    }
    }
    return FALSE;
}
static bool choose(HWND owner,const wchar_t* title,unsigned initial,unsigned& result){
    struct alignas(DWORD) Template {
        DLGTEMPLATE dialog;
        WORD menu,windowClass,title;
    } spec={};
    spec.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;
    spec.dialog.dwExtendedStyle=WS_EX_CONTROLPARENT;
    Selection color={initial,title};
    setHex(color,initial);
    if(DialogBoxIndirectParam(GetModuleHandle(0),&spec.dialog,owner,dialogProc,(LPARAM)&color)!=IDOK)return false;
    result=color.hex;
    return true;
}
}
