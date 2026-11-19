#pragma once
#include <uxtheme.h>

// Native controls retain their editing, keyboard and accessibility behavior.
// Only their paint path changes. Animation timers exist solely during input.
namespace SettingsSkin {
enum Kind { Text, Button, Switch, Slider, Hue, Combo, Edit, Navigation, MaterialCard };
struct Item {
    Kind kind;
    RECT bounds;
    HFONT font=nullptr,strong=nullptr;
    bool primary=false;
    bool hover=false;
    float hoverFrom=0,hoverTo=0,switchFrom=0,switchTo=0;
    ULONGLONG hoverAt=0,switchAt=0;
};
constexpr int footerHeight=82;
static int scrollLimit();
static HFONT smallFont=0,strongFont=0;
static HBRUSH inputBrush=0,dialogBrush=0;
static HDC canvas=0;
static HBITMAP canvasBitmap=0;
static HGDIOBJ canvasOld=0;
static int canvasWidth=0,canvasHeight=0,scroll=0;
static const wchar_t* sections[]={L"Appearance",L"Behavior",L"Tools",L"Launchers",L"Notes",L"Gmail",L"Phone",L"Discord",L"Shortcuts",L"Workstations"};
static const wchar_t* descriptions[]={
    L"Choose how the desktop island looks.",L"Set where the island appears and how it opens.",
    L"Configure clipboard history, focus sessions, and startup.",L"Choose the apps and websites shown in Launchers.",
    L"Edit the quick note shown in Widgets.",L"Manage the inbox shown on the island.",
    L"Set up Android or iPhone notifications.",L"Use Discord’s desktop mute shortcut.",
    L"Keyboard and trackpad controls for the desktop island.",
    L"Create and edit sets of apps, browser tabs, and music."};
static Color foreground(){return Color(255,241,241,246);}
static Color muted(){return Color(255,174,175,190);}
static Color surface(){return Color(255,24,24,31);}
static Color sidebar(){return Color(255,19,19,26);}
static Color field(){return Color(255,35,35,45);}
static Color outline(){return Color(255,62,62,78);}
static Color brand(){return Color(255,164,174,255);}
static Color brandInk(){return Color(255,25,29,55);}
static Color colorFromHex(unsigned hex){return Color(255,(hex>>16)&255,(hex>>8)&255,hex&255);}
static bool parseHex(const wchar_t* text,unsigned& hex){
    if(wcslen(text)!=6)return false;
    if(wcsspn(text,L"0123456789abcdefABCDEF")!=6)return false;
    hex=(unsigned)wcstoul(text,nullptr,16);
    return true;
}
static COLORREF rgb(Color c){return RGB(c.GetR(),c.GetG(),c.GetB());}
static LRESULT controlColors(HDC dc,HBRUSH brush,Color background){
    SetTextColor(dc,rgb(foreground()));
    SetBkColor(dc,rgb(background));
    SetBkMode(dc,OPAQUE);
    return (LRESULT)brush;
}
template<typename Paint>
static void buffered(HDC target,int width,int height,Paint paint){
    if(!target||width<=0||height<=0)return;
    HDC memory=CreateCompatibleDC(target);
    HBITMAP bitmap=memory?CreateCompatibleBitmap(target,width,height):nullptr;
    HGDIOBJ previous=bitmap?SelectObject(memory,bitmap):nullptr;
    if(previous&&previous!=HGDI_ERROR){
        paint(memory);
        BitBlt(target,0,0,width,height,memory,0,0,SRCCOPY);
        SelectObject(memory,previous);
    }else paint(target);
    if(bitmap)DeleteObject(bitmap);
    if(memory)DeleteDC(memory);
}
static float progress(ULONGLONG start){float t=(GetTickCount64()-start)/160.f;return t>=1?1:1-powf(1-t,3);}
static Color blend(Color a,Color b,float t){return Color(255,(BYTE)(a.GetR()+(b.GetR()-a.GetR())*t),(BYTE)(a.GetG()+(b.GetG()-a.GetG())*t),(BYTE)(a.GetB()+(b.GetB()-a.GetB())*t));}
static void rect(Graphics& g,float x,float y,float w,float h,float radius,Color fill,Color border=Color(0,0,0,0)){
    if(w<=0||h<=0)return;float r=fminf(radius,fminf(w,h)*.5f);GraphicsPath path;
    if(r<1)path.AddRectangle(RectF(x,y,w,h));else{path.AddArc(x,y,r*2,r*2,180,90);path.AddArc(x+w-r*2,y,r*2,r*2,270,90);path.AddArc(x+w-r*2,y+h-r*2,r*2,r*2,0,90);path.AddArc(x,y+h-r*2,r*2,r*2,90,90);path.CloseFigure();}
    if(fill.GetA()){SolidBrush brush(fill);g.FillPath(&brush,&path);}if(border.GetA()){Pen pen(border,(REAL)(D(1)>1?D(1):1));g.DrawPath(&pen,&path);}
}
static void text(HDC dc,const wchar_t* value,RECT bounds,HFONT font,Color color,UINT flags=DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS){
    HGDIOBJ old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,rgb(color));DrawText(dc,value,-1,&bounds,flags|DT_NOPREFIX);SelectObject(dc,old);
}
// Acquire the GDI+ HDC before GDI text, so buffered shapes cannot cover text.
static void text(Graphics& graphics,HDC dc,const wchar_t* value,RECT bounds,HFONT font,Color color,UINT flags=DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS){
    graphics.Flush(FlushIntentionSync);HDC target=graphics.GetHDC();if(target){text(target,value,bounds,font,color,flags);graphics.ReleaseHDC(target);}else text(dc,value,bounds,font,color,flags);
}
static void line(Graphics& g,float x,float y,float xx,float yy,Color color,float width){Pen pen(color,width);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);g.DrawLine(&pen,x,y,xx,yy);}
static void glyph(Graphics& g,int which,float x,float y,float size,Color color){
    GraphicsState state=g.Save();g.TranslateTransform(x,y);g.ScaleTransform(size/24.f,size/24.f);
    auto l=[&](float a,float b,float c,float d){line(g,a,b,c,d,color,1.6f);};
    auto r=[&](float a,float b,float c,float d,float radius){rect(g,a,b,c,d,radius,Color(0,0,0,0),color);};
    Pen pen(color,1.6f);
    switch(which){
    case 0:g.DrawEllipse(&pen,3.f,3.f,18.f,18.f);for(int i=0;i<3;i++){SolidBrush b(color);g.FillEllipse(&b,6.5f+i*4.5f,i==1?6.f:9.f,2.f,2.f);}break;
    case 1:for(int i=0;i<3;i++){int yy=5+i*7;l(3,yy,21,yy);r(i==1?6:14,yy-2,3,4,1);}break;
    case 2:for(int i=0;i<4;i++)r(4+(i%2)*10,4+(i/2)*10,6,6,1.5);break;
    case 3:r(3,5,18,15,3);l(8,3,16,3);l(10,10,15,13);l(15,13,10,16);break;
    case 4:r(5,3,14,18,3);l(9,8,15,8);l(9,12,15,12);l(9,16,13,16);break;
    case 5:r(3,5,18,14,3);l(4,7,12,13);l(12,13,20,7);break;
    case 6:r(6,2,12,20,3);l(10,5,14,5);l(11,19,13,19);break;
    case 7:r(3,4,18,13,4);l(8,17,6,21);l(6,21,12,17);l(8,10,9,10);l(15,10,16,10);break;
    case 8:r(2,5,20,14,3);for(int i=0;i<6;i++)l(5+i*2.8f,9,5+i*2.8f,10);l(7,15,17,15);break;
    case 9:l(5,12,10,17);l(10,17,19,7);break;
    case 10:r(3,4,18,16,3);l(12,4,12,20);l(3,11,12,11);break;
    }
    g.Restore(state);
}
static void releaseCanvas(){if(canvas){SelectObject(canvas,canvasOld);DeleteObject(canvasBitmap);DeleteDC(canvas);}canvas=0;canvasBitmap=0;canvasWidth=canvasHeight=0;}
static void fonts(){
    if(settingsFont)DeleteObject(settingsFont);if(settingsTitleFont)DeleteObject(settingsTitleFont);if(smallFont)DeleteObject(smallFont);if(strongFont)DeleteObject(strongFont);
    settingsFont=CreateFont(-D(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
    settingsTitleFont=CreateFont(-D(31),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
    smallFont=CreateFont(-D(12),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
    strongFont=CreateFont(-D(14),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
    if(!inputBrush)inputBrush=CreateSolidBrush(rgb(field()));
    if(!dialogBrush)dialogBrush=CreateSolidBrush(rgb(surface()));
}
static void chrome(HWND window){
    BOOL dark=TRUE;
    DWORD color=rgb(sidebar()), corner=2;
    DwmSetWindowAttribute(window,20,&dark,sizeof(dark));
    DwmSetWindowAttribute(window,35,&color,sizeof(color));
    DwmSetWindowAttribute(window,33,&corner,sizeof(corner));
    int size=D(16);
    SendMessage(window,WM_SETICON,ICON_SMALL,(LPARAM)brandIcon(size,size));
    SendMessage(window,WM_SETICON,ICON_BIG,(LPARAM)brandIcon(D(32),D(32)));
}
static void drawBrand(Graphics& graphics,HDC dc,int x,int y,int size){
    graphics.Flush(FlushIntentionSync);
    HDC target=graphics.GetHDC();
    DrawIconEx(target?target:dc,x,y,brandIcon(size,size),size,size,0,nullptr,DI_NORMAL);
    if(target)graphics.ReleaseHDC(target);
}
static void preview(HDC dc,Graphics& g){
    rect(g,D(228),D(110),D(650),D(69),D(12),sidebar(),outline());
    text(g,dc,L"Island preview",RECT{D(248),D(122),D(490),D(146)},strongFont,foreground());
    text(g,dc,appearanceModes[draft.glassMode?0:1],RECT{D(248),D(146),D(490),D(166)},smallFont,muted());

    Color body=draft.glassMode?glassBase(draft):rgbColor(draft.backgroundRgb);
    Color edge=draft.glassMode?mixColor(body,Color(255,255,255,255),.42):outline();
    int radius=(int)SendDlgItemMessage(settingsWindow,105,TBM_GETPOS,0,0);
    bool flatTop=SendDlgItemMessage(settingsWindow,109,BM_GETCHECK,0,0)==BST_CHECKED;
    wchar_t name[64];
    GetDlgItemText(settingsWindow,113,name,ARRAYSIZE(name));
    rect(g,D(564),D(123),D(294),D(44),D(radius),body,edge);
    if(flatTop)rect(g,D(564),D(123),D(294),D(20),0,body);
    if(draft.glassMode){
        int inset=flatTop?6:(radius<22?radius:22);
        line(g,D(564+inset),D(125),D(858-inset),D(125),edge,D(1));
    }
    drawBrand(g,dc,D(575),D(131),D(28));
    text(g,dc,name,RECT{D(613),D(123),D(793),D(167)},strongFont,rgbColor(draft.fontRgb));
    Color accent=colorFromHex(draft.accentRgb);
    const int bars[]={7,15,20,12,7};
    for(int i=0;i<5;i++)rect(g,D(806+i*7),D(145-bars[i]/2),D(3),D(bars[i]),D(2),accent);
}
static Item* item(HWND h){DWORD_PTR data=0;extern LRESULT CALLBACK procedure(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);return GetWindowSubclass(h,procedure,77,&data)?(Item*)data:0;}
static void drawControl(HWND h,HDC dc,Item& model){
    HFONT bodyFont=model.font?model.font:settingsFont;
    HFONT emphasisFont=model.strong?model.strong:strongFont;
    UINT dpi=GetDpiForWindow(h);
    auto D=[dpi](int dip){return MulDiv(dip,dpi,96);};
    RECT bounds;GetClientRect(h,&bounds);int w=bounds.right,hh=bounds.bottom,id=GetDlgCtrlID(h);Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    bool enabled=IsWindowEnabled(h)!=FALSE,focused=GetFocus()==h;
    float hover=model.hoverFrom+(model.hoverTo-model.hoverFrom)*progress(model.hoverAt);
    Color bg=model.kind==Navigation?sidebar():surface();g.Clear(bg);
    wchar_t value[4096];GetWindowText(h,value,4096);RECT label={D(12),0,w-D(12),hh};
    if(model.kind==Text){
        bool heading=model.primary||(id>=660&&id<=669);
        Color color=id>=1100&&id<1200?foreground():muted();
        RECT r={0,0,w,hh};bool multiple=hh>D(28)||wcschr(value,L'\n');
        UINT align=(GetWindowLongPtr(h,GWL_STYLE)&SS_TYPEMASK)==SS_CENTER?DT_CENTER:DT_LEFT;
        UINT format=multiple?DT_WORDBREAK:DT_SINGLELINE|DT_VCENTER;
        text(g,dc,value,r,heading?emphasisFont:id==8002?smallFont:bodyFont,heading?foreground():color,align|format|DT_END_ELLIPSIS);return;
    }
    if(model.kind==Switch){
        bool checked=SendMessage(h,BM_GETCHECK,0,0)==BST_CHECKED;
        float t=model.switchFrom+(model.switchTo-model.switchFrom)*progress(model.switchAt);
        if(progress(model.switchAt)>=1)t=checked?1:0;
        label={0,0,w-D(65),hh};text(g,dc,value,label,bodyFont,enabled?foreground():muted());
        float x=w-D(48),y=(hh-D(24))*.5f;
        rect(g,x,y,D(45),D(24),D(12),blend(Color(255,69,70,86),brand(),t));
        SolidBrush knob(Color(255,250,252,255));g.FillEllipse(&knob,x+D(3)+D(21)*t,y+D(3),(REAL)D(18),(REAL)D(18));
        if(focused)rect(g,x-D(3),y-D(3),D(51),D(30),D(15),Color(0,0,0,0),brand());return;
    }
    if(model.kind==Slider||model.kind==Hue){
        RECT channel={},thumb={};
        SendMessage(h,TBM_GETCHANNELRECT,0,(LPARAM)&channel);
        SendMessage(h,TBM_GETTHUMBRECT,0,(LPARAM)&thumb);
        float left=channel.left,width=channel.right-channel.left,center=hh*.5f;
        float filled=(thumb.left+thumb.right)*.5f-left;
        if(model.kind==Hue&&width>0){
            Color stops[]={Color(255,255,0,0),Color(255,255,255,0),Color(255,0,255,0),
                           Color(255,0,255,255),Color(255,0,0,255),Color(255,255,0,255),Color(255,255,0,0)};
            REAL positions[]={0,1.f/6,2.f/6,.5f,4.f/6,5.f/6,1};
            RectF track(left,center-D(3),width,D(6));
            LinearGradientBrush spectrum(track,stops[0],stops[6],LinearGradientModeHorizontal);
            spectrum.SetInterpolationColors(stops,positions,7);
            g.FillRectangle(&spectrum,track);
        }else{
            rect(g,left,center-D(2),width,D(4),D(2),outline());
            if(filled>0)rect(g,left,center-D(2),filled,D(4),D(2),brand());
        }
        SolidBrush knob(foreground());g.FillEllipse(&knob,left+filled-D(7),center-D(7),(REAL)D(14),(REAL)D(14));
        if(focused){Pen ring(brand(),D(1));g.DrawEllipse(&ring,left+filled-D(10),center-D(10),(REAL)D(20),(REAL)D(20));}return;
    }
    if(model.kind==Combo){
        rect(g,D(1),D(1),w-D(2),hh-D(2),D(8),field(),focused?brand():outline());
        int index=(int)SendMessage(h,CB_GETCURSEL,0,0);value[0]=0;if(index>=0)SendMessage(h,CB_GETLBTEXT,index,(LPARAM)value);
        label.right=w-D(35);text(g,dc,value,label,bodyFont,foreground());line(g,w-D(24),hh/2-D(2),w-D(20),hh/2+D(2),muted(),(D(1)*1.4f));line(g,w-D(20),hh/2+D(2),w-D(16),hh/2-D(2),muted(),(D(1)*1.4f));return;
    }
    if(model.kind==Navigation){
        int page=id-8100;bool selected=page==settingsPage;
        Color tint=selected?Color(255,43,45,69):blend(bg,field(),hover);
        rect(g,D(1),D(1),w-D(2),hh-D(2),D(8),tint);
        if(selected)rect(g,D(1),D(11),D(3),hh-D(22),D(2),brand());
        glyph(g,page==workstationSettingsPage?10:page,D(13),(hh-D(18))*.5f,D(18),selected?brand():muted());
        label.left=D(42);text(g,dc,value,label,selected?emphasisFont:bodyFont,selected?foreground():muted());
        if(focused)rect(g,D(2),D(2),w-D(4),hh-D(4),D(7),Color(0,0,0,0),brand());
        return;
    }
    if(model.kind==MaterialCard){
        bool glass=id==8200,selected=glass==draft.glassMode;
        Color sample=glass?glassBase(draft):rgbColor(draft.backgroundRgb);
        rect(g,D(1),D(1),w-D(2),hh-D(2),D(9),
             selected?Color(255,39,40,60):blend(sidebar(),field(),hover),selected||focused?brand():outline());
        rect(g,D(13),D(12),D(52),D(30),D(9),sample,glass?muted():outline());
        if(glass)line(g,D(24),D(14),D(54),D(14),foreground(),D(1));
        label={D(78),0,w-D(40),hh};
        text(g,dc,value,label,emphasisFont,foreground());
        if(selected)glyph(g,9,w-D(35),(hh-D(18))*.5f,D(18),brand());
        return;
    }
    for(const auto& color:appearanceColors){
        if(id!=color.button)continue;
        rect(g,D(1),D(1),w-D(2),hh-D(2),D(8),blend(field(),outline(),hover*.4f),focused?brand():outline());
        rect(g,D(11),D(7),D(18),D(18),D(5),rgbColor(draft.*color.value),outline());
        line(g,D(36),D(14),D(40),D(18),muted(),D(1));
        line(g,D(40),D(18),D(44),D(14),muted(),D(1));
        return;
    }
    if(id==8410||id==8411){
        bool selected=SendMessage(h,BM_GETCHECK,0,0)==BST_CHECKED;
        Color fill=selected?Color(255,43,45,69):blend(bg,field(),hover);
        rect(g,D(1),D(1),w-D(2),hh-D(2),D(8),fill,selected||focused?brand():outline());
        text(g,dc,value,label,selected?emphasisFont:bodyFont,selected?brand():foreground(),
             DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        return;
    }
    bool primary=model.primary||id==IDOK||id==9002;
    bool quiet=id==9001||id==IDCANCEL;
    bool pressed=(SendMessage(h,BM_GETSTATE,0,0)&BST_PUSHED)!=0;
    Color fill=primary?blend(brand(),foreground(),hover*.22f):
               quiet?blend(bg,field(),hover):blend(field(),outline(),hover*.44f);
    if(!enabled)fill=field();
    if(pressed)fill=blend(fill,sidebar(),.18f);
    rect(g,D(1),D(1),w-D(2),hh-D(2),D(8),fill,primary||quiet?Color(0,0,0,0):outline());
    Color ink=!enabled?muted():primary?brandInk():foreground();
    label={D(4),0,w-D(4),hh};
    text(g,dc,value,label,primary?emphasisFont:bodyFont,ink,
         DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_CENTER);
    if(focused)rect(g,D(3),D(3),w-D(6),hh-D(6),D(6),Color(0,0,0,0),primary?brandInk():brand());
}
static void animation(HWND h,Item& model){
    ULONGLONG now=GetTickCount64();float t=progress(model.hoverAt);model.hoverFrom+=(model.hoverTo-model.hoverFrom)*t;model.hoverTo=model.hover?1:0;model.hoverAt=now;
    t=progress(model.switchAt);model.switchFrom+=(model.switchTo-model.switchFrom)*t;model.switchTo=SendMessage(h,BM_GETCHECK,0,0)==BST_CHECKED?1:0;model.switchAt=now;
    if(draftAnimations)SetTimer(h,71,16,0);else{model.hoverFrom=model.hoverTo;model.switchFrom=model.switchTo;model.hoverAt=model.switchAt=now-160;}
    InvalidateRect(h,0,FALSE);
}
LRESULT CALLBACK procedure(HWND h,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    Item* model=(Item*)data;if(!model)return DefSubclassProc(h,message,wp,lp);
    if(message==WM_NCDESTROY){KillTimer(h,71);RemoveWindowSubclass(h,procedure,77);HeapFree(GetProcessHeap(),0,model);return DefSubclassProc(h,message,wp,lp);}
    if(message==WM_MOUSEWHEEL){if(GetParent(h)==settingsWindow&&scrollLimit()>0)return SendMessage(settingsWindow,message,wp,lp);}
    if(message==WM_ERASEBKGND&&model->kind!=Edit)return 1;
    if(message==WM_TIMER&&wp==71){InvalidateRect(h,0,FALSE);if(progress(model->hoverAt)>=1&&progress(model->switchAt)>=1)KillTimer(h,71);return 0;}
    if((message==WM_PAINT||message==WM_PRINTCLIENT)&&model->kind!=Edit){
        // The native trackbar invalidates its own thumb bounds, which are smaller than our focus ring.
        if(message==WM_PAINT&&(model->kind==Slider||model->kind==Hue))
            InvalidateRect(h,nullptr,FALSE);
        PAINTSTRUCT paint;HDC dc=message==WM_PAINT?BeginPaint(h,&paint):(HDC)wp;RECT r;GetClientRect(h,&r);
        buffered(dc,r.right,r.bottom,[&](HDC frame){drawControl(h,frame,*model);});
        if(message==WM_PAINT)EndPaint(h,&paint);return 0;
    }
    bool animated=model->kind==Button||model->kind==Switch||model->kind==Navigation||model->kind==MaterialCard;
    if(message==WM_MOUSEMOVE&&animated&&!model->hover){model->hover=true;TRACKMOUSEEVENT track={sizeof(track),TME_LEAVE,h,0};TrackMouseEvent(&track);animation(h,*model);}
    if(message==WM_MOUSELEAVE&&animated){model->hover=false;animation(h,*model);}
    if(message==WM_SETFOCUS||message==WM_KILLFOCUS){InvalidateRect(h,0,FALSE);InvalidateRect(GetParent(h),0,FALSE);}
    if(message==WM_KEYDOWN&&model->kind==Navigation&&(wp==VK_UP||wp==VK_DOWN)){int row=settingsNavigationRow(GetDlgCtrlID(h)-8100);int page=settingsNavigationOrder[(row+(wp==VK_DOWN?1:settingsPageCount-1))%settingsPageCount];SendMessage(settingsWindow,WM_COMMAND,8100+page,0);SetFocus(GetDlgItem(settingsWindow,8100+settingsPage));return 0;}
    LRESULT result=DefSubclassProc(h,message,wp,lp);
    model=item(h);
    if(!model)return result;
    if(message==BM_SETCHECK||message==WM_LBUTTONUP||message==WM_KEYUP){if(model->kind==Switch)animation(h,*model);else InvalidateRect(h,0,FALSE);}
    if(message==TBM_SETPOS||message==CB_SETCURSEL||message==WM_SETTEXT||
       message==WM_ENABLE||message==WM_LBUTTONDOWN)InvalidateRect(h,0,FALSE);
    return result;
}
static void prepare(HWND h,Kind kind,RECT logical,HFONT font=nullptr,HFONT strong=nullptr,bool primary=false){
    Item* model=(Item*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Item));if(!model)return;model->kind=kind;model->bounds=logical;model->hoverAt=model->switchAt=GetTickCount64()-160;
    model->font=font;model->strong=strong;model->primary=primary;
    if(!SetWindowSubclass(h,procedure,77,(DWORD_PTR)model)){HeapFree(GetProcessHeap(),0,model);return;}
    if(kind==Edit||kind==Combo)SetWindowTheme(h,L"DarkMode_Explorer",0);else SetWindowTheme(h,L"",L"");
    SendMessage(h,WM_SETFONT,(WPARAM)(font?font:settingsFont),TRUE);
}
static int scrollLimit(){
    RECT client;
    GetClientRect(settingsWindow,&client);
    int lastRow=0;
    for(int i=0;i<settingsControlCount;i++){
        HWND child=settingsControls[i];
        auto* model=item(child);
        if(model&&GetDlgCtrlID(child)!=101&&model->bounds.bottom>lastRow)
            lastRow=model->bounds.bottom;
    }
    int visibleBottom=(client.bottom-D(footerHeight+10))*96/(int)settingsDpi;
    return lastRow>visibleBottom?lastRow-visibleBottom:0;
}
static void layout(){
    int limit=scrollLimit();if(scroll>limit)scroll=limit;if(scroll<0)scroll=0;RECT client;GetClientRect(settingsWindow,&client);
    for(int i=0;i<settingsControlCount;i++){HWND child=settingsControls[i];Item* model=item(child);if(!model)continue;RECT r=model->bounds;bool edit=model->kind==Edit;int height=model->kind==Combo?240:r.bottom-r.top;
        MoveWindow(child,D(r.left+(edit?9:0)),D(r.top-scroll+(edit?5:0)),D(r.right-r.left-(edit?18:0)),D(height-(edit?10:0)),FALSE);
        bool visible=GetDlgCtrlID(child)!=101&&D(r.top-scroll)>=D(107)&&D(r.bottom-scroll)<=client.bottom-D(footerHeight+10);ShowWindow(child,visible?SW_SHOWNOACTIVATE:SW_HIDE);
    }RedrawWindow(settingsWindow,0,0,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
static void paint(HWND window,HDC dc){
    RECT client;GetClientRect(window,&client);int w=client.right,h=client.bottom;if(w<=0||h<=0)return;
    if(!canvas||w!=canvasWidth||h!=canvasHeight){releaseCanvas();canvas=CreateCompatibleDC(dc);canvasBitmap=CreateCompatibleBitmap(dc,w,h);canvasOld=SelectObject(canvas,canvasBitmap);canvasWidth=w;canvasHeight=h;}
    Graphics g(canvas);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.Clear(surface());
    rect(g,0,0,D(198),h,0,sidebar());
    line(g,D(198),0,D(198),h,outline(),D(1));
    drawBrand(g,canvas,D(24),D(20),D(34));
    text(g,canvas,L"BezelBrew",RECT{D(68),D(20),D(178),D(54)},strongFont,foreground());
    line(g,D(24),D(67),D(174),D(67),outline(),D(1));
    text(g,canvas,sections[settingsPage],RECT{D(228),D(17),w-D(30),D(57)},settingsTitleFont,foreground());
    text(g,canvas,descriptions[settingsPage],RECT{D(230),D(57),w-D(30),D(82)},settingsFont,muted());
    line(g,D(228),D(94),w-D(30),D(94),outline(),D(1));
    line(g,D(228),h-D(footerHeight),w-D(30),h-D(footerHeight),outline(),D(1));
    GraphicsState clip=g.Save();g.SetClip(Rect(D(214),D(98),w-D(234),h-D(footerHeight+102)));
    if(settingsPage==0&&scroll==0)preview(canvas,g);
    for(int i=0;i<settingsControlCount;i++){
        HWND child=settingsControls[i];Item* model=item(child);if(!model||model->kind!=Edit||!IsWindowVisible(child))continue;
        RECT r=model->bounds;rect(g,D(r.left),D(r.top-scroll),D(r.right-r.left),D(r.bottom-r.top),D(8),field(),GetFocus()==child?brand():outline());
    }
    g.Restore(clip);int limit=scrollLimit();if(limit>0){float track=h-D(footerHeight+128),height=track*track/(track+D(limit));float y=D(112)+(track-height)*scroll/limit;rect(g,w-D(31),y,D(3),height,D(1),outline());}
    g.Flush(FlushIntentionSync);BitBlt(dc,0,0,w,h,canvas,0,0,SRCCOPY);
}
static LRESULT dropdown(DRAWITEMSTRUCT* draw){
    if(draw->CtlType!=ODT_COMBOBOX)return FALSE;RECT r=draw->rcItem;HBRUSH brush=CreateSolidBrush(rgb(draw->itemState&ODS_SELECTED?Color(255,52,54,82):field()));FillRect(draw->hDC,&r,brush);DeleteObject(brush);wchar_t value[512]=L"";if(draw->itemID!=(UINT)-1)SendMessage(draw->hwndItem,CB_GETLBTEXT,draw->itemID,(LPARAM)value);r.left+=D(12);text(draw->hDC,value,r,settingsFont,foreground());return TRUE;
}
static void destroy(){releaseCanvas();if(inputBrush)DeleteObject(inputBrush);if(dialogBrush)DeleteObject(dialogBrush);inputBrush=dialogBrush=0;if(smallFont)DeleteObject(smallFont);if(strongFont)DeleteObject(strongFont);smallFont=strongFont=0;}
}
