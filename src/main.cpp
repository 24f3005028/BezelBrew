#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
using namespace Gdiplus;
#include "glass.hpp"
#include "media.hpp"
#include "connectors.hpp"
#include "launch_targets.hpp"
#include "launcher_icons.hpp"
#include "preferences.hpp"
#include "spotify_playback.hpp"
#include "discord_voice.hpp"
#include "background_apps.hpp"

// No webview; optional connectors sleep between refreshes; the frame-clock worker sleeps when not animating.
static HWND win,glassWindow,tooltips;
static const wchar_t* appName(){return prefs.displayName;}
static HICON brandIcon(int width=0,int height=0) {
    return (HICON)LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_BEZELBREW),
                            IMAGE_ICON,width,height,LR_SHARED);
}
// Keep internal class/config/mutex names stable across visible renames.
static void excludeOverlayFromShell(HWND window){
    if(!window)return;
    LONG_PTR style=GetWindowLongPtr(window,GWL_EXSTYLE);
    LONG_PTR hidden=(style&~WS_EX_APPWINDOW)|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE;
    if(style!=hidden){bool visible=IsWindowVisible(window)!=FALSE;if(visible)ShowWindow(window,SW_HIDE);SetWindowLongPtr(window,GWL_EXSTYLE,hidden);SetWindowPos(window,0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);if(visible)ShowWindow(window,SW_SHOWNOACTIVATE);}
    ITaskbarList* taskbar=0;
    if(SUCCEEDED(CoCreateInstance(CLSID_TaskbarList,0,CLSCTX_INPROC_SERVER,IID_ITaskbarList,(void**)&taskbar))){if(SUCCEEDED(taskbar->HrInit()))taskbar->DeleteTab(window);taskbar->Release();}
}
static ULONGLONG mediaReceived=0,lastMusicPaint=0,titleStarted=0;static bool titleOverflow=false;static IAudioEndpointVolume* outputVolume=0;static float masterVolume=0;static bool volumeKnown=false,volumeMuted=false;
static NativeMedia::Snapshot nowPlaying={};static Bitmap* albumArt=0;
static HANDLE frameClock=0,frameStop=0,frameThread=0;static LONG framePending=0;
static DWORD WINAPI frameWorker(void*){HANDLE events[]={frameStop,frameClock};for(;;){DWORD result=WaitForMultipleObjects(2,events,FALSE,INFINITE);if(result!=WAIT_OBJECT_0+1)break;if(InterlockedCompareExchange(&framePending,1,0)==0)PostMessage(win,WM_APP+20,0,0);}return 0;}
static LARGE_INTEGER profileFrequency={};static double frameLast=0,frameSum=0,frameWorst=0,paintWorst=0;static int frameCount=0;
static double profileNow(){LARGE_INTEGER t;QueryPerformanceCounter(&t);return 1000.0*t.QuadPart/profileFrequency.QuadPart;}
static UINT dpi=96, taskbarMessage;
static ULONG_PTR graphicsToken;
static HFONT fonts[5]={};
static HANDLE singleton;
static ULONGLONG pausedUntil=0,pausedFocusRemaining=0;
static bool paused=false;
static bool expanded=false, pinned=false, tracking=false, focus=false;
static ULONGLONG focusTotalSeconds=1500;
static bool inBreak=false;
static bool animations=true, docked=false;
static bool settingsSaveSucceeded=true;
static wchar_t configFile[MAX_PATH]={}; static bool saveEnabled=true;
static Connectors::View connectorView={};static int connectorFeed=0;
static int selected=0, scrollOffset=0, hover=-1, pressed=-1;
static int clipCount=0, fileCount=0;
static wchar_t clips[12][2048]={},files[12][MAX_PATH]={};
static wchar_t status[160]=L"";
static ULONGLONG deadline=0,statusUntil=0;
static ULONGLONG cpuIdle=0,cpuTotal=0;
static unsigned cpuPercent=0;
static NOTIFYICONDATA tray={sizeof(tray)};
constexpr int pageTop=58;
static const int fullWidth=680, fullHeight=284, unusedDimension=0;
static double smallWidth=400,smallHeight=12;
static double openness=0, fromOpen=0, toOpen=0, tabReveal=1, hoverReveal=1;
static ULONGLONG motionStart=0,tabStart=0,hoverStart=0;
static bool moving=false,tabMoving=false,hoverMoving=false;
static int previousHover=-1;
static bool contentDirty=true;static DWORD* scenePixels=0;static DWORD* cachedPixels=0;
static HDC backDC=0;
static HBITMAP backBitmap=0; static HGDIOBJ oldBitmap=0;
static int bufferWidth=0,bufferHeight=0;
static int contentOffset=0;
static int slideSource=0,slideTarget=0,slideDirection=1;
static double slideFrom=0,slideGoal=1;
static bool swipeCandidate=false,swiping=false;static int swipeX=0,swipeY=0;static double swipeOffset=0;
static int horizontalWheel=0;static ULONGLONG lastWheel=0,lastWheelEvent=0;
static double contentOpacity=1;
constexpr int workstationPage=10;
static const wchar_t* tabNames[]={L"Music",L"Shelf",L"Clipboard",L"Widgets",L"Apps",L"Focus",L"Settings",L"Workstations"};
constexpr int primaryPages[]={0,1,2,3,5,6,7,workstationPage};
constexpr int primaryTabCount=ARRAYSIZE(primaryPages);
static_assert(ARRAYSIZE(tabNames)==primaryTabCount);
static int primaryPage(int index){return primaryPages[index];}
static int primaryIndex(int page){
    for(int i=0;i<primaryTabCount;i++)if(primaryPages[i]==page)return i;
    return -1;
}
static int nextPrimaryPage(int page,int direction){
    int index=primaryIndex(page);
    if(index<0)return primaryPage(direction>0?0:primaryTabCount-1);
    return primaryPage((index+direction+primaryTabCount)%primaryTabCount);
}
constexpr RECT tileBounds(int index){
    int left=36+index*52;
    return {left,9,left+36,45};
}
enum class ToolbarGlyph { Wave, Tray, Copies, Widgets, Launch, Focus, Tuning, Screens, Pin, Mail, Glass, ClearApps };
constexpr ToolbarGlyph primaryGlyphs[]={
    ToolbarGlyph::Wave, ToolbarGlyph::Tray, ToolbarGlyph::Copies, ToolbarGlyph::Widgets,
    ToolbarGlyph::Launch, ToolbarGlyph::Focus, ToolbarGlyph::Tuning, ToolbarGlyph::Screens
};
static_assert(ARRAYSIZE(primaryGlyphs)==primaryTabCount);
struct ToolbarControl {
    int id;
    RECT bounds;
    ToolbarGlyph glyph;
    const wchar_t* name;
};
constexpr ToolbarControl pinControl={900,tileBounds(8),ToolbarGlyph::Pin,L"Pin or unpin island"};
constexpr ToolbarControl mailControl={902,tileBounds(9),ToolbarGlyph::Mail,L"Mail"};
constexpr ToolbarControl appearanceControl={901,tileBounds(10),ToolbarGlyph::Glass,L"Appearance"};
constexpr ToolbarControl clearAppsControl={903,tileBounds(11),ToolbarGlyph::ClearApps,L"Clear memory"};
static int startupPage(int savedTab){return savedTab==8?workstationPage:savedTab;}
static const wchar_t* appearanceModes[]={L"Glass",L"Solid"};
enum Icon {Music,IconFolder,Clipboard,Grid,Timer,Gear,Palette,Previous,Play,Next,Volume,Mic,Check,Plus,Trash,Arrow,Pause,Power,Monitor,Close,Minus,Envelope,Note,Phone,Voice};
constexpr RECT quickNoteBounds={596,67,646,109};
constexpr UINT_PTR musicTitleTip=20,quickNoteTip=21;
struct Hit { int id,x,y,w,h; const wchar_t* tip; };
static Hit hits[64]; static int hitCount=0;
static double uiScale=.55;
static int S(double n){return (int)floor(n*dpi/96.0*uiScale+.5);}
static void updatePageTooltips(){
    if(!tooltips)return;
    TOOLINFO tip={sizeof(tip)};
    tip.hwnd=win;
    tip.uId=musicTitleTip;
    if(selected==0)tip.rect={S(181),S(65),S(646),S(105)};
    SendMessage(tooltips,TTM_NEWTOOLRECT,0,(LPARAM)&tip);
    tip.uId=quickNoteTip;
    const auto& note=quickNoteBounds;
    tip.rect=selected==3?RECT{S(note.left),S(note.top),S(note.right),S(note.bottom)}:RECT{};
    SendMessage(tooltips,TTM_NEWTOOLRECT,0,(LPARAM)&tip);
}
static float F(double n){return (float)(n*dpi/96.0*uiScale);}
static double lerp(double a,double b,double t){return a+(b-a)*t;}
static double ease(double t){if(t<0)t=0;if(t>1)t=1;return 1-pow(1-t,4);}
static Color rgbColor(int rgb){return Color(255,(rgb>>16)&255,(rgb>>8)&255,rgb&255);}
static Color mixColor(Color a,Color b,double weight){
    return Color(255,(BYTE)lerp(a.GetR(),b.GetR(),weight),(BYTE)lerp(a.GetG(),b.GetG(),weight),
                 (BYTE)lerp(a.GetB(),b.GetB(),weight));
}
static bool lightInk(Color c){return c.GetR()*.2126+c.GetG()*.7152+c.GetB()*.0722>=142;}
static Color glassBase(const Preferences& appearance){
    Color ink=rgbColor(appearance.fontRgb);
    Color neutral=lightInk(ink)?Color(255,36,40,50):Color(255,206,214,227);
    return mixColor(neutral,rgbColor(appearance.glassTintRgb),appearance.tintPercent/100.0);
}
static Color base(){return prefs.glassMode?glassBase(prefs):rgbColor(prefs.backgroundRgb);}
static Color primary(){return rgbColor(prefs.fontRgb);}
static Color secondary(){return mixColor(primary(),base(),.24);}
static Color accent(){return rgbColor(prefs.accentRgb);}
static Color panel(double strength=.10){
    Color ink=primary();
    if(prefs.glassMode&&OpticalGlass::surface)
        return Color((BYTE)(255*strength),ink.GetR(),ink.GetG(),ink.GetB());
    return mixColor(base(),ink,strength);
}
static Color fade(Color c,double alpha=1){
    return Color((BYTE)(c.GetA()*contentOpacity*alpha),c.GetR(),c.GetG(),c.GetB());
}
static void repaint(){contentDirty=true;InvalidateRect(win,0,FALSE);}
static void say(const wchar_t* message){wcsncpy_s(status,message,_TRUNCATE);statusUntil=GetTickCount64()+4000;SetTimer(win,4,4000,0);repaint();}
static void sampleCpu(){FILETIME a,b,c;if(GetSystemTimes(&a,&b,&c)){ULARGE_INTEGER i,k,u;i.LowPart=a.dwLowDateTime;i.HighPart=a.dwHighDateTime;k.LowPart=b.dwLowDateTime;k.HighPart=b.dwHighDateTime;u.LowPart=c.dwLowDateTime;u.HighPart=c.dwHighDateTime;ULONGLONG total=k.QuadPart+u.QuadPart;if(cpuTotal&&total>cpuTotal){ULONGLONG delta=total-cpuTotal,idle=i.QuadPart-cpuIdle;cpuPercent=(unsigned)(100*(delta>idle?delta-idle:0)/delta);}cpuIdle=i.QuadPart;cpuTotal=total;}}
static void releaseBuffer(){if(cachedPixels){HeapFree(GetProcessHeap(),0,cachedPixels);cachedPixels=0;}if(!expanded&&!moving)OpticalGlass::release();if(backDC){SelectObject(backDC,oldBitmap);DeleteObject(backBitmap);DeleteDC(backDC);backDC=0;backBitmap=0;scenePixels=0;}bufferWidth=bufferHeight=0;}
static void createFonts(){for(int i=0;i<5;i++){if(fonts[i])DeleteObject(fonts[i]);int sizes[]={10,12,15,19,29};fonts[i]=CreateFont(-MulDiv(sizes[i]*prefs.sizePercent*prefs.textPercent,dpi,960000),0,0,0,i>=2?FW_SEMIBOLD:FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");}}
static void label(HDC dc,int x,int y,int w,int h,const wchar_t* value,Color color,int size=1,UINT flags=DT_LEFT){Color c=fade(color);SetTextColor(dc,RGB(c.GetR(),c.GetG(),c.GetB()));SetBkMode(dc,TRANSPARENT);SelectObject(dc,fonts[size]);RECT r={S(x),S(y+contentOffset),S(x+w),S(y+h+contentOffset)};DrawText(dc,value,-1,&r,flags|(flags&DT_WORDBREAK?DT_EDITCONTROL:DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS)|DT_NOPREFIX);}
static void rounded(Graphics& g,float x,float y,float w,float h,float r,Color c,bool stroke=false,float line=1){x=F(x);y=F(y+contentOffset);w=F(w);h=F(h);r=fminf(F(r),fminf(w,h)*.5f);if(w<=0||h<=0)return;GraphicsPath path;if(r<1)path.AddRectangle(RectF(x,y,w,h));else{path.AddArc(x,y,2*r,2*r,180,90);path.AddArc(x+w-2*r,y,2*r,2*r,270,90);path.AddArc(x+w-2*r,y+h-2*r,2*r,2*r,0,90);path.AddArc(x,y+h-2*r,2*r,2*r,90,90);path.CloseFigure();}if(stroke){Pen p(fade(c),(REAL)(F(line)<1?1:F(line)));g.DrawPath(&p,&path);}else{SolidBrush b(fade(c));g.FillPath(&b,&path);}}
static void line(Graphics& g,float x,float y,float xx,float yy,Color c,float width=1.6){Pen p(fade(c),(REAL)(F(width)<1?1:F(width)));p.SetStartCap(LineCapRound);p.SetEndCap(LineCapRound);g.DrawLine(&p,(REAL)F(x),(REAL)F(y+contentOffset),(REAL)F(xx),(REAL)F(yy+contentOffset));}
static void ellipse(Graphics& g,float x,float y,float w,float h,Color c,bool fill=false,float width=1.5){if(fill){SolidBrush b(fade(c));g.FillEllipse(&b,(REAL)F(x),(REAL)F(y+contentOffset),(REAL)F(w),(REAL)F(h));}else{Pen p(fade(c),(REAL)(F(width)<1?1:F(width)));g.DrawEllipse(&p,(REAL)F(x),(REAL)F(y+contentOffset),(REAL)F(w),(REAL)F(h));}}
static void triangle(Graphics& g,float x,float y,float size,Color c,bool left=false){PointF pts[3]={{(REAL)F(x+(left?size:0)),(REAL)F(y+contentOffset)},{(REAL)F(x+(left?0:size)),(REAL)F(y+size*.65+contentOffset)},{(REAL)F(x+(left?size:0)),(REAL)F(y+size*1.3+contentOffset)}};SolidBrush b(fade(c));g.FillPolygon(&b,pts,3);}
static void icon(Graphics& g,Icon kind,int x,int y,int size,Color c){float k=size/24.f; // vector icons share a 24-unit coordinate system
#define L(a,b,d,e) line(g,x+(a)*k,y+(b)*k,x+(d)*k,y+(e)*k,c,1.6*k)
#define R(a,b,w,h,r) rounded(g,x+(a)*k,y+(b)*k,(w)*k,(h)*k,(r)*k,c,true,1.5*k)
#define E(a,b,w,h) ellipse(g,x+(a)*k,y+(b)*k,(w)*k,(h)*k,c)
    switch(kind){
    case Music:L(9,17,9,4);L(9,4,20,2);L(20,2,20,15);L(9,8,20,6);ellipse(g,x+2*k,y+15*k,7*k,5*k,c);ellipse(g,x+13*k,y+13*k,7*k,5*k,c);break;
    case IconFolder:L(2,6,9,6);L(9,6,12,9);L(12,9,22,9);L(22,9,22,21);L(22,21,2,21);L(2,21,2,6);break;
    case Clipboard:R(4,5,16,17,2);rounded(g,x+8*k,y+2*k,8*k,5*k,1.5*k,base());R(8,2,8,5,1.5);break;
    case Note:R(4,3,16,18,3);L(8,8,16,8);L(8,12,16,12);L(8,16,13,16);break;
    case Grid:for(int i=0;i<4;i++)R(3+(i%2)*11,3+(i/2)*11,7,7,1);break;
    case Timer:E(3,5,18,18);L(12,9,12,14);L(12,14,16,16);L(9,1,15,1);L(12,1,12,4);break;
    case Gear:for(int i=0;i<8;i++){double a=i*3.14159265/4;L(12+8*cos(a),12+8*sin(a),12+11*cos(a),12+11*sin(a));}E(4,4,16,16);E(9,9,6,6);break;
    case Palette:E(2,2,20,20);for(int i=0;i<4;i++){double a=(i*50+170)*3.14159265/180;ellipse(g,x+(11+6*cos(a))*k,y+(11+6*sin(a))*k,2*k,2*k,c,true);}break;
    case Previous:triangle(g,x+2*k,y+4*k,9*k,c,true);triangle(g,x+10*k,y+4*k,9*k,c,true);break;
    case Next:triangle(g,x+3*k,y+4*k,9*k,c);triangle(g,x+11*k,y+4*k,9*k,c);break;
    case Play:triangle(g,x+5*k,y+2*k,15*k,c);break;
    case Pause:rounded(g,x+5*k,y+3*k,5*k,18*k,1*k,c);rounded(g,x+14*k,y+3*k,5*k,18*k,1*k,c);break;
    case Volume:L(2,9,6,9);L(6,9,12,4);L(12,4,12,20);L(12,20,6,15);L(6,15,2,15);L(2,15,2,9);L(17,8,19,12);L(19,12,17,16);L(21,5,23,12);L(23,12,21,19);break;
    case Mic:R(8,2,8,13,4);L(5,10,5,13);L(5,13,8,18);L(8,18,16,18);L(16,18,19,13);L(19,13,19,10);L(12,18,12,22);L(8,22,16,22);break;
    case Check:L(4,12,9,17);L(9,17,21,5);break;
    case Plus:L(12,4,12,20);L(4,12,20,12);break;
    case Trash:R(6,7,12,15,2);L(3,5,21,5);L(9,2,15,2);L(10,10,10,18);L(14,10,14,18);break;
    case Arrow:L(4,12,20,12);L(15,7,20,12);L(15,17,20,12);break;
    case Power:E(3,3,18,18);L(12,1,12,11);break;
    case Monitor:R(2,3,20,14,2);L(12,17,12,22);L(7,22,17,22);break;
    case Envelope:R(2,4,20,16,3);L(3,6,12,13);L(12,13,21,6);break;
    case Phone:R(6,2,12,20,3);L(10,5,14,5);L(11,19,13,19);break;
    case Voice:R(3,3,18,14,4);L(8,17,6,21);L(6,21,12,17);E(8,9,2,2);E(14,9,2,2);break;
    case Minus:L(5,12,19,12);break;
    case Close:L(6,6,18,18);L(18,6,6,18);break;
    }
#undef L
#undef R
#undef E
}
static void addHit(int id,int x,int y,int w,int h,const wchar_t* tip){if(hitCount<64)hits[hitCount++]={id,x,y,w,h,tip};}
static int hitAt(int x,int y){if(!expanded||moving||tabMoving||swiping)return -1;for(int i=hitCount-1;i>=0;i--){Hit& h=hits[i];if(x>=h.x&&x<h.x+h.w&&y>=h.y&&y<h.y+h.h)return h.id;}return -1;}
static void hoverBox(Graphics& g,int id,int x,int y,int w,int h,int radius=12){double v=id==hover?hoverReveal:id==previousHover?1-hoverReveal:0;if(pressed==id)v=1;if(v>0)rounded(g,x,y,w,h,radius,panel(v*.14));}
static void iconButton(Graphics& g,int id,int x,int y,int w,int h,Icon glyph,const wchar_t* tip,Color color,int size=24){addHit(id,x,y,w,h,tip);hoverBox(g,id,x,y,w,h);icon(g,glyph,x+(w-size)/2,y+(h-size)/2,size,pressed==id?accent():color);}
static void drawToolbarGlyph(Graphics& g,ToolbarGlyph kind,float x,float y,float size,Color ink){
    const GraphicsState state=g.Save();
    g.TranslateTransform(F(x),F(y));
    g.ScaleTransform(F(size)/24.f,F(size)/24.f);
    Pen pen(ink,1.9f);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    SolidBrush solid(ink);
    const auto stroke=[&](float x1,float y1,float x2,float y2){
        g.DrawLine(&pen,x1,y1,x2,y2);
    };
    const auto mark=[&](int x,int y,int w,int h){g.FillEllipse(&solid,x,y,w,h);};
    switch(kind){
    case ToolbarGlyph::Wave:
        stroke(4,10,4,14);
        stroke(8,7,8,17);
        stroke(12,3,12,21);
        stroke(16,8,16,16);
        stroke(20,10,20,14);
        break;
    case ToolbarGlyph::Tray: {
        stroke(7,4,17,4);
        stroke(5,8,19,8);
        const Point tray[]={{3,12},{8,12},{10,15},{14,15},{16,12},{21,12},{21,19},{19,21},{5,21},{3,19}};
        g.FillPolygon(&solid,tray,ARRAYSIZE(tray));
        break;
    }
    case ToolbarGlyph::Copies: {
        const Point back[]={{7,3},{19,3},{21,5},{21,16}};
        g.DrawLines(&pen,back,ARRAYSIZE(back));
        const Point front[]={{3,7},{15,7},{17,9},{17,21},{3,21}};
        g.DrawPolygon(&pen,front,ARRAYSIZE(front));
        stroke(7,12,13,12);
        stroke(7,16,11,16);
        break;
    }
    case ToolbarGlyph::Widgets:
        g.FillRectangle(&solid,3,4,18,6);
        g.DrawRectangle(&pen,4,14,9,6);
        g.FillRectangle(&solid,16,13,5,8);
        break;
    case ToolbarGlyph::Launch: {
        const Point frame[]={{12,6},{4,6},{4,20},{18,20},{18,12}};
        g.DrawLines(&pen,frame,ARRAYSIZE(frame));
        stroke(12,12,21,3);
        const Point arrow[]={{15,3},{21,3},{21,9}};
        g.DrawLines(&pen,arrow,ARRAYSIZE(arrow));
        break;
    }
    case ToolbarGlyph::Focus: {
        const Point corners[][3]={
            {{3,8},{3,3},{8,3}},{{16,3},{21,3},{21,8}},
            {{21,16},{21,21},{16,21}},{{8,21},{3,21},{3,16}}
        };
        for(const auto& corner:corners)g.DrawLines(&pen,corner,ARRAYSIZE(corner));
        mark(9,9,6,6);
        break;
    }
    case ToolbarGlyph::Tuning:
        stroke(7,3,7,21);
        stroke(17,3,17,21);
        mark(3,5,8,8);
        mark(13,11,8,8);
        break;
    case ToolbarGlyph::Screens: {
        const Point back[]={{10,4},{21,4},{21,14}};
        g.DrawLines(&pen,back,ARRAYSIZE(back));
        g.DrawRectangle(&pen,3,8,14,9);
        stroke(10,17,10,21);
        stroke(6,21,14,21);
        break;
    }
    case ToolbarGlyph::Pin: {
        const Point head[]={{9,3},{15,3},{15,8},{19,12},{19,14},{5,14},{5,12},{9,8}};
        g.FillPolygon(&solid,head,ARRAYSIZE(head));
        stroke(12,13,12,21);
        break;
    }
    case ToolbarGlyph::Mail:
        g.DrawRectangle(&pen,3,5,18,14);
        stroke(4,7,12,13);
        stroke(12,13,20,7);
        break;
    case ToolbarGlyph::ClearApps: {
        const Point body[]={{6,8},{7,21},{17,21},{18,8}};
        g.DrawLines(&pen,body,ARRAYSIZE(body));
        stroke(4,6,20,6);
        stroke(9,3,15,3);
        stroke(10,10,10,17);
        stroke(14,10,14,17);
        break;
    }
    case ToolbarGlyph::Glass: {
        GraphicsPath drop;
        drop.AddBezier(12,3,9,7,5,11,5,15);
        drop.AddBezier(5,15,5,23,19,23,19,15);
        drop.AddBezier(19,15,19,11,15,7,12,3);
        drop.CloseFigure();
        g.DrawPath(&pen,&drop);
        g.DrawArc(&pen,8,11,8,7,90,89);
        break;
    }
    }
    g.Restore(state);
}
static void toolbarButton(Graphics& g,const ToolbarControl& control,bool active,float size=22){
    const auto& b=control.bounds;
    const int width=b.right-b.left,height=b.bottom-b.top;
    addHit(control.id,b.left,b.top,width,height,control.name);
    Color edge=active?fade(accent(),.7):panel(.19);
    rounded(g,b.left,b.top,width,height,8,active?fade(accent(),.28):panel(.12));
    hoverBox(g,control.id,b.left,b.top,width,height,8);
    rounded(g,b.left,b.top,width,height,8,edge,true);
    drawToolbarGlyph(g,control.glyph,b.left+(width-size)/2,b.top+(height-size)/2,size,
                     active||pressed==control.id||hover==control.id?primary():secondary());
}
static void action(Graphics& g,HDC dc,int id,int x,int y,int w,const wchar_t* value,Icon glyph,bool active=false){addHit(id,x,y,w,42,value);rounded(g,x,y,w,42,12,panel(active?.21:.10));hoverBox(g,id,x,y,w,42);icon(g,glyph,x+13,y+12,18,active?accent():secondary());label(dc,x+42,y,w-50,42,value,primary());}
static void squareTile(Graphics& g,HDC dc,int id,int center,int top,int size,Icon glyph,
                       const wchar_t* name,int labelWidth=0,bool selected=false){
    const int left=center-size/2,captionHeight=23,glyphSize=size*5/9;
    addHit(id,left,top,size,size,labelWidth?nullptr:name);
    if(labelWidth)addHit(id,center-labelWidth/2,top+size,labelWidth,captionHeight,nullptr);
    rounded(g,left,top,size,size,10,selected?fade(accent(),.25):panel(.13));
    hoverBox(g,id,left,top,size,size,10);
    rounded(g,left,top,size,size,10,selected?fade(accent(),.75):panel(.22),true);
    icon(g,glyph,center-glyphSize/2,top+(size-glyphSize)/2,glyphSize,
         selected||pressed==id?accent():primary());
    if(labelWidth)label(dc,center-labelWidth/2,top+size,labelWidth,captionHeight,name,primary(),1,DT_CENTER);
}
static LRESULT CALLBACK glassProc(HWND h,UINT message,WPARAM w,LPARAM l){
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){PAINTSTRUCT ps;BeginPaint(h,&ps);EndPaint(h,&ps);return 0;}
    if(message==WM_DROPFILES)return SendMessage(win,message,w,l);
    return DefWindowProc(h,message,w,l);
}
static void presentScene(){
    if(!backDC||!scenePixels||!cachedPixels)return;
    RECT r;GetWindowRect(win,&r);POINT position={r.left,r.top},origin={0,0};SIZE size={r.right-r.left,r.bottom-r.top};
    double progress=openness<0?0:openness>1?1:openness;
    int rw=S(lerp(smallWidth,fullWidth,progress)),rh=S(lerp(smallHeight,fullHeight,progress));
    int left=(size.cx-rw)/2,radius=(int)lerp(3,MulDiv(prefs.cornerDip,dpi,96),progress);if(radius>rh/2)radius=rh/2;
    for(int y=0;y<size.cy;y++){
        DWORD* dst=scenePixels+y*bufferWidth;DWORD* src=cachedPixels+y*bufferWidth;
        ZeroMemory(dst,size.cx*4);
        if(y>=rh)continue;
        int edge=y<radius?(docked?0:radius-y):y>=rh-radius?y-(rh-radius):0;
        int inset=edge>0?radius-(int)sqrt((double)radius*radius-(double)edge*edge):0;
        int first=left+inset,last=left+rw-inset;if(first<0)first=0;if(last>size.cx)last=size.cx;
        if(last>first)memcpy(dst+first,src+first,(last-first)*4);
    }
    BLENDFUNCTION blend={AC_SRC_OVER,0,(BYTE)(8+247*progress),AC_SRC_ALPHA};
    UpdateLayeredWindow(win,0,&position,&size,backDC,&origin,0,&blend,ULW_ALPHA);
}
static HWND glassForeground=nullptr;
static void syncGlass(int x,int y,int w,int h){
    if(!prefs.glassMode||paused)return;
    HWND foreground=GetForegroundWindow();
    if(OpticalGlass::surface&&OpticalGlass::width==w&&OpticalGlass::height==h&&
       OpticalGlass::originX==x&&OpticalGlass::originY==y&&glassForeground==foreground)return;
    // The screen DC can include layered windows even without CAPTUREBLT.
    const bool visible=IsWindowVisible(win)!=FALSE;
    if(visible)ShowWindow(win,SW_HIDE);
    DwmFlush();
    double start=profileNow();
    const bool captured=OpticalGlass::capture(x,y,w,h);
    if(visible)ShowWindow(win,SW_SHOWNOACTIVATE);
    if(captured)OpticalGlass::render();
    OpticalGlass::renderMs=profileNow()-start;
    glassForeground=foreground;
    contentDirty=true;
}
static BOOL CALLBACK findPreferredMonitor(HMONITOR monitor,HDC,LPRECT,LPARAM data){MONITORINFOEX info={};info.cbSize=sizeof(info);GetMonitorInfo(monitor,&info);if(!_wcsicmp(info.szDevice,prefs.monitor))*(HMONITOR*)data=monitor;return TRUE;}
static MONITORINFO islandMonitor(){HMONITOR monitor=MonitorFromWindow(win,MONITOR_DEFAULTTOPRIMARY);if(prefs.monitor[0])EnumDisplayMonitors(0,0,findPreferredMonitor,(LPARAM)&monitor);MONITORINFO info={sizeof(info)};GetMonitorInfo(monitor,&info);return info;}
static int islandX(const MONITORINFO& mi,int width){int offset=MulDiv(prefs.offset,dpi,96);int x=prefs.alignment==0?mi.rcWork.left+offset:prefs.alignment==2?mi.rcWork.right-width+offset:mi.rcMonitor.left+(mi.rcMonitor.right-mi.rcMonitor.left-width)/2+offset;if(x<mi.rcWork.left)x=mi.rcWork.left;if(x+width>mi.rcWork.right)x=mi.rcWork.right-width;return x;}
static void configureMaterial(){
    OpticalGlass::blurPasses=prefs.blurPasses;
    OpticalGlass::tint=prefs.tintPercent/100.0;
    OpticalGlass::cornerPixels=MulDiv(prefs.cornerDip,dpi,96);
    OpticalGlass::paneWidth=S(fullWidth);
    OpticalGlass::paneHeight=S(fullHeight);
    OpticalGlass::flatTop=docked;
    OpticalGlass::lightInk=lightInk(primary());
    Color tint=rgbColor(prefs.glassTintRgb);
    OpticalGlass::tintColor={tint.GetB(),tint.GetG(),tint.GetR(),255};
}
static void place(){
    MONITORINFO mi=islandMonitor();
    int revealW=S(lerp(smallWidth,fullWidth,openness)),revealH=S(lerp(smallHeight,fullHeight,openness));
    bool active=expanded||moving;
    int w=active?S(fullWidth*1.025):S(smallWidth),h=active?S(fullHeight*1.025):S(smallHeight);
    int x=islandX(mi,w),y=mi.rcWork.top;
    RECT current;GetWindowRect(win,&current);
    if(current.left!=x||current.top!=y||current.right-current.left!=w||current.bottom-current.top!=h)
        SetWindowPos(win,HWND_TOPMOST,x,y,w,h,SWP_NOACTIVATE|SWP_NOCOPYBITS);
    int left=(w-revealW)/2,radius=MulDiv(prefs.cornerDip,dpi,96);
    HRGN region=CreateRoundRectRgn(left,0,left+revealW+1,revealH+1,radius*2,radius*2);
    if(docked){HRGN top=CreateRectRgn(left,0,left+revealW,radius);CombineRgn(region,region,top,RGN_OR);DeleteObject(top);}
    if(!moving||current.right-current.left!=w||current.bottom-current.top!=h){DeleteObject(region);region=CreateRoundRectRgn(0,0,w+1,h+1,radius*2,radius*2);if(docked){HRGN top=CreateRectRgn(0,0,w,radius);CombineRgn(region,region,top,RGN_OR);DeleteObject(top);}SetWindowRgn(win,region,FALSE);}else DeleteObject(region);
    if(active&&openness>.015){configureMaterial();syncGlass(x,y,w,h);}
    /* WM_PAINT presents once per frame, after the cached scene is ready. */
}
static void saveSettings(){
    if(!saveEnabled||!configFile[0])return;
    settingsSaveSucceeded=unicodeConfig(configFile);if(!settingsSaveSucceeded)return;
    WritePrivateProfileString(L"Appearance",L"Animations",animations?L"1":L"0",configFile);
    WritePrivateProfileString(L"Appearance",L"Docked",docked?L"1":L"0",configFile);
    settingsSaveSucceeded=writePreferences(configFile,prefs);
}
static void loadSettings(){
    GetModuleFileName(0,configFile,MAX_PATH);wchar_t* end=wcsrchr(configFile,L'\\');
    if(end)wcscpy_s(end+1,MAX_PATH-(end-configFile)-1,L"WinPixel.ini");
    wchar_t overridePath[MAX_PATH];if(GetEnvironmentVariable(L"WINPIXEL_CONFIG",overridePath,MAX_PATH)>0)wcscpy_s(configFile,overridePath);
    readPreferences(configFile,prefs);uiScale=.55*prefs.sizePercent/100.0;smallWidth=prefs.hoverWidth/uiScale;smallHeight=prefs.hoverHeight/uiScale;selected=startupPage(prefs.defaultTab);
    animations=GetPrivateProfileInt(L"Appearance",L"Animations",1,configFile)!=0;
    docked=GetPrivateProfileInt(L"Appearance",L"Docked",0,configFile)!=0;
}
static void readVolume(bool reconnect);
static bool frameTimer=false;
static void animationTimer(){
    bool needed=moving||tabMoving||hoverMoving||(animations&&expanded&&selected==0&&((prefs.waveformEnabled&&nowPlaying.status==4)||titleOverflow)&&!paused);
    if(needed&&!frameTimer){timeBeginPeriod(1);if(frameClock){LARGE_INTEGER due;due.QuadPart=-160000;SetWaitableTimerEx(frameClock,&due,16,0,0,0,0);}else SetTimer(win,3,16,0);frameTimer=true;}
    else if(!needed&&frameTimer){KillTimer(win,3);if(frameClock)CancelWaitableTimer(frameClock);timeEndPeriod(1);frameTimer=false;}
}
static void expand(bool value){if(paused)return;if(expanded==value)return;expanded=value;if(!value){swiping=swipeCandidate=false;if(GetCapture()==win)ReleaseCapture();}contentDirty=true;frameLast=0;fromOpen=openness;toOpen=value?1:0;motionStart=GetTickCount64();moving=animations;if(value){OpticalGlass::release();NativeMedia::refresh();readVolume(true);}if(value)SetTimer(win,1,1000,0);else if(!focus)KillTimer(win,1);hover=-1;pressed=-1;if(!moving){openness=toOpen;place();if(!expanded)releaseBuffer();}animationTimer();repaint();}
static void toggleIsland(){
    const bool open=!expanded;
    if(!open)pinned=false;
    expand(open);
}
static void choose(int index,int direction=0){
    if(index==selected)return;slideSource=selected;slideTarget=index;slideDirection=direction?direction:(index>selected?1:-1);
    selected=index;updatePageTooltips();scrollOffset=0;slideFrom=0;slideGoal=1;tabReveal=animations?0:1;tabStart=GetTickCount64();tabMoving=animations;hover=-1;sampleCpu();if(index==0){NativeMedia::refresh();readVolume(false);}animationTimer();repaint();
}
static void beginSwipe(int x,int y){swipeX=x;swipeY=y;swipeOffset=0;swipeCandidate=true;swiping=false;}
static void moveSwipe(int x,int y){
    if(!swipeCandidate)return;int dx=x-swipeX,dy=y-swipeY;
    if(!swiping&&abs(dy)>18&&abs(dy)>abs(dx)){swipeCandidate=false;return;}
    if(!swiping&&abs(dx)>18&&abs(dx)>abs(dy)){swiping=true;pressed=-1;hover=-1;tabMoving=false;slideSource=selected;}
    if(swiping){swipeOffset=dx;if(swipeOffset>fullWidth*.85)swipeOffset=fullWidth*.85;if(swipeOffset<-fullWidth*.85)swipeOffset=-fullWidth*.85;slideDirection=dx<0?1:-1;slideTarget=nextPrimaryPage(selected,slideDirection);tabReveal=fabs(swipeOffset)/fullWidth;KillTimer(win,2);repaint();}
}
static void finishSwipe(bool canceled=false){
    bool active=swiping;swiping=swipeCandidate=false;if(!active)return;
    bool commit=!canceled&&fabs(swipeOffset)>=48;
    slideFrom=tabReveal;slideGoal=commit?1:0;
    if(commit){selected=slideTarget;updatePageTooltips();scrollOffset=0;if(selected==0){NativeMedia::refresh();readVolume(false);}}
    tabStart=GetTickCount64();tabMoving=animations;
    if(!animations){tabReveal=slideGoal;}
    animationTimer();repaint();if(!pinned)SetTimer(win,2,prefs.closeDelay,0);
}
static void media(WORD key){INPUT in[2]={};in[0].type=in[1].type=INPUT_KEYBOARD;in[0].ki.wVk=in[1].ki.wVk=key;in[1].ki.dwFlags=KEYEVENTF_KEYUP;if(SendInput(2,in,sizeof(INPUT))!=2)say(L"Windows could not deliver the media command.");}
static void captureClipboard(){if(paused||!prefs.clipboardEnabled)return;if(!IsClipboardFormatAvailable(CF_UNICODETEXT)||!OpenClipboard(win))return;HANDLE h=GetClipboardData(CF_UNICODETEXT);const wchar_t* p=h?(const wchar_t*)GlobalLock(h):0;if(p&&p[0]&&(!clipCount||wcsncmp(clips[0],p,2047))){for(int i=prefs.historyLimit-1;i>0;i--)wcscpy_s(clips[i],clips[i-1]);wcsncpy_s(clips[0],p,2047);if(clipCount<prefs.historyLimit)clipCount++;scrollOffset=0;}if(p)GlobalUnlock(h);CloseClipboard();if(expanded)repaint();}
static void copyClip(int n){if(n<0||n>=clipCount)return;SIZE_T size=(wcslen(clips[n])+1)*sizeof(wchar_t);HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,size);if(!mem)return;void* p=GlobalLock(mem);if(!p){GlobalFree(mem);return;}memcpy(p,clips[n],size);GlobalUnlock(mem);if(OpenClipboard(win)){EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,mem))GlobalFree(mem);CloseClipboard();say(L"Copied to clipboard.");}else GlobalFree(mem);}
static void open(const wchar_t* target){HINSTANCE result=ShellExecute(win,L"open",target,0,0,SW_SHOWNORMAL);if((INT_PTR)result<=32)say(L"Windows could not open this shortcut.");}
static void songTitle(HDC dc,const wchar_t* value){
    SelectObject(dc,fonts[2]);SIZE measured={};GetTextExtentPoint32(dc,value,(int)wcslen(value),&measured);
    RECT r={S(181),S(65+contentOffset),S(646),S(105+contentOffset)};titleOverflow=measured.cx>r.right-r.left;
    if(!titleOverflow||!animations){label(dc,181,65,465,40,value,primary(),2);return;}
    double speed=24*dpi/96.0,gap=36*dpi/96.0,cycle=(measured.cx+gap)/speed+1.8;
    double phase=fmod((GetTickCount64()-titleStarted)/1000.0,cycle),offset=phase>1.8?(phase-1.8)*speed:0;
    int saved=SaveDC(dc);IntersectClipRect(dc,r.left,r.top,r.right,r.bottom);SetBkMode(dc,TRANSPARENT);Color c=primary();SetTextColor(dc,RGB(c.GetR(),c.GetG(),c.GetB()));RECT text={r.left-(int)offset,r.top,r.left-(int)offset+measured.cx+2,r.bottom};DrawText(dc,value,-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);text.left+=(int)(measured.cx+gap);text.right+=(int)(measured.cx+gap);DrawText(dc,value,-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);RestoreDC(dc,saved);
}
static LONGLONG songPosition(){LONGLONG p=nowPlaying.position;if(nowPlaying.status==4&&mediaReceived)p+=(GetTickCount64()-mediaReceived)*10000;if(nowPlaying.duration>0&&p>nowPlaying.duration)p=nowPlaying.duration;return p<0?0:p;}
static void readVolume(bool reconnect){
    if(reconnect&&outputVolume){outputVolume->Release();outputVolume=0;}
    if(!outputVolume){IMMDeviceEnumerator* e=0;IMMDevice* d=0;if(SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator),0,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),(void**)&e))){if(SUCCEEDED(e->GetDefaultAudioEndpoint(eRender,eMultimedia,&d))){d->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,0,(void**)&outputVolume);d->Release();}e->Release();}}
    BOOL mute=FALSE;volumeKnown=outputVolume&&SUCCEEDED(outputVolume->GetMasterVolumeLevelScalar(&masterVolume));if(outputVolume&&SUCCEEDED(outputVolume->GetMute(&mute)))volumeMuted=mute!=FALSE;
}
static void setVolumeAt(int x){if(!outputVolume)return;float value=(x-274)/240.f;if(value<0)value=0;if(value>1)value=1;if(SUCCEEDED(outputVolume->SetMasterVolumeLevelScalar(value,0))){masterVolume=value;volumeKnown=true;repaint();}}
static void drawMusic(Graphics& g,HDC dc){
    rounded(g,34,66,120,120,20,panel());
    if(albumArt){GraphicsState save=g.Save();GraphicsPath clip;float x=F(34),y=F(66+contentOffset),w=F(120),r=F(18);clip.AddArc(x,y,r*2,r*2,180,90);clip.AddArc(x+w-2*r,y,r*2,r*2,270,90);clip.AddArc(x+w-2*r,y+w-2*r,r*2,r*2,0,90);clip.AddArc(x,y+w-2*r,r*2,r*2,90,90);clip.CloseFigure();g.SetClip(&clip);g.SetInterpolationMode(InterpolationModeHighQualityBicubic);g.DrawImage(albumArt,x,y,w,w);g.Restore(save);}else icon(g,Music,74,105,40,secondary());
    songTitle(dc,nowPlaying.available?(nowPlaying.title[0]?nowPlaying.title:L"Unknown track"):L"No active player");
    label(dc,181,106,465,30,nowPlaying.available?(nowPlaying.artist[0]?nowPlaying.artist:wcsstr(nowPlaying.app,L"Spotify")?L"Spotify":wcsstr(nowPlaying.app,L"Chrome")?L"Chrome":L"Windows media"):L"Play music in an app",secondary(),1);
    iconButton(g,100,275,147,58,48,Previous,L"Previous track",primary(),31);
    iconButton(g,101,370,139,66,62,nowPlaying.status==4?Pause:Play,L"Play / pause",primary(),36);
    iconButton(g,102,473,147,58,48,Next,L"Next track",primary(),31);
    LONGLONG position=songPosition();double fraction=nowPlaying.duration>0?position/(double)nowPlaying.duration:0;float end=(float)(34+612*fraction);
    // The unplayed track begins at the thumb; nothing sits behind the waveform.
    if(end<646)rounded(g,end,215,646-end,4,2,panel(.35));
    if(fraction>0){
        if(nowPlaying.status==4&&animations&&prefs.waveformEnabled){
            GraphicsPath wave;double phase=GetTickCount64()/230.0;
            auto value=[&](float x){double edge=(end-x)/14.0;if(edge<0)edge=0;if(edge>1)edge=1;return (float)(217+3.2*edge*edge*(3-2*edge)*sin((x-34)*.19-phase));};
            for(float x=34;x<end;x+=4){float nx=x+4;if(nx>end)nx=end;float dx=nx-x,y=value(x),ny=value(nx),d=(value(x+.1f)-value(x-.1f))/.2f,nd=(value(nx+.1f)-value(nx-.1f))/.2f;wave.AddBezier(F(x),F(y+contentOffset),F(x+dx/3),F(y+d*dx/3+contentOffset),F(nx-dx/3),F(ny-nd*dx/3+contentOffset),F(nx),F(ny+contentOffset));}
            Pen pen(accent(),F(2.6));pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);g.DrawPath(&pen,&wave);
        }else rounded(g,34,215,(float)(612*fraction),4,2,accent());
        ellipse(g,end-4,213,8,8,primary(),true);
    }
    if(nowPlaying.duration>0)addHit(106,34,202,612,30,L"Seek in song");
    wchar_t elapsed[32]=L"--:--",remaining[32]=L"--:--";
    if(nowPlaying.duration>0){LONGLONG seconds=position/10000000,left=(nowPlaying.duration-position)/10000000;swprintf_s(elapsed,L"%lld:%02lld",seconds/60,seconds%60);swprintf_s(remaining,L"-%lld:%02lld",left/60,left%60);}
    label(dc,34,231,180,24,elapsed,secondary(),0);label(dc,466,231,180,24,remaining,secondary(),0,DT_RIGHT);
    icon(g,Volume,224,253,22,secondary());rounded(g,274,262,240,5,2.5,panel(.35));
    if(volumeKnown){float filled=masterVolume*240;if(filled>0)rounded(g,274,262,filled,5,2.5,volumeMuted?secondary():primary());ellipse(g,274+filled-5,259,10,10,primary(),true);addHit(105,263,246,264,33,L"System volume");}
    wchar_t volume[24];if(volumeKnown)swprintf_s(volume,volumeMuted?L"Muted":L"%d%%",(int)(masterVolume*100+.5));else wcscpy_s(volume,L"--");label(dc,539,247,100,31,volume,secondary(),0);
}
static void heading(HDC dc,const wchar_t* title,const wchar_t* subtitle){label(dc,34,67,540,31,title,primary(),2);label(dc,34,99,570,22,subtitle,secondary(),1);}
static void drawShelf(Graphics& g,HDC dc){heading(dc,L"File shelf",L"Drag files here for quick access.");iconButton(g,201,603,67,40,36,Trash,L"Clear shelf",secondary(),20);if(!fileCount){rounded(g,34,137,612,105,16,panel());icon(g,IconFolder,61,170,36,secondary());label(dc,117,155,490,30,L"Drop files here",primary(),2);label(dc,117,188,490,25,L"Click a saved file to reveal it in Explorer",secondary());}else for(int row=0;row<3&&row+scrollOffset<fileCount;row++){int i=row+scrollOffset,y=130+row*38;addHit(210+i,34,y,612,34,L"Reveal file in Explorer");rounded(g,34,y,612,34,10,panel());hoverBox(g,210+i,34,y,612,34);icon(g,IconFolder,46,y+8,17,accent());const wchar_t* name=wcsrchr(files[i],L'\\');label(dc,78,y,535,34,name?name+1:files[i],primary());icon(g,Arrow,620,y+10,14,secondary());}wchar_t footer[80];swprintf_s(footer,L"%d / 12 files   •   References only   •   Scroll for more",fileCount);label(dc,34,253,612,18,footer,secondary(),0);}
static void drawClipboard(Graphics& g,HDC dc){heading(dc,L"Clipboard",L"Select an entry to copy it again.");iconButton(g,301,603,67,40,36,Trash,L"Clear history",secondary(),20);if(!clipCount){rounded(g,34,137,612,105,16,panel());icon(g,Clipboard,61,170,36,secondary());label(dc,117,155,490,30,L"Nothing copied yet",primary(),2);label(dc,117,188,490,25,L"Copy text anywhere and it will appear here",secondary());}else for(int row=0;row<3&&row+scrollOffset<clipCount;row++){int i=row+scrollOffset,y=130+row*38;addHit(310+i,34,y,612,34,L"Copy text");rounded(g,34,y,612,34,10,panel());hoverBox(g,310+i,34,y,612,34);icon(g,Clipboard,46,y+8,17,secondary());label(dc,78,y,535,34,clips[i],primary());icon(g,Arrow,620,y+10,14,secondary());}label(dc,34,253,612,18,L"Memory only   •   12 recent entries   •   Click to copy",secondary(),0);}
static void tile(Graphics& g,HDC dc,int x,int y,int w,const wchar_t* name,const wchar_t* value,Icon glyph,Color color){rounded(g,x,y,w,105,18,panel());icon(g,glyph,x+16,y+17,20,color);label(dc,x+47,y+13,w-60,28,name,secondary(),1);label(dc,x+16,y+57,w-32,48,value,primary(),3);}
static void drawWidgets(Graphics& g,HDC dc){
    heading(dc,L"Widgets",L"Local time and system usage.");
    const auto& note=quickNoteBounds;
    int width=note.right-note.left,height=note.bottom-note.top;
    rounded(g,note.left,note.top,width,height,11,panel(.20));
    iconButton(g,706,note.left,note.top,width,height,Note,L"Quick note",primary(),25);
    SYSTEMTIME now;
    GetLocalTime(&now);
    wchar_t clock[40],ram[40],cpu[40];
    swprintf_s(clock,L"%02u:%02u",now.wHour,now.wMinute);
    MEMORYSTATUSEX m={sizeof(m)};
    GlobalMemoryStatusEx(&m);
    swprintf_s(ram,L"%lu%%",m.dwMemoryLoad);
    swprintf_s(cpu,L"%u%%",cpuPercent);
    tile(g,dc,34,133,194,L"Local time",clock,Timer,accent());
    tile(g,dc,243,133,194,L"CPU",cpu,Monitor,secondary());
    tile(g,dc,452,133,194,L"Memory",ram,Grid,secondary());
    addHit(707,452,245,194,30,clearAppsControl.name);
    rounded(g,452,245,194,30,8,panel(.14));
    hoverBox(g,707,452,245,194,30,8);
    label(dc,452,245,194,30,clearAppsControl.name,primary(),1,DT_CENTER);
}
constexpr int appColumns=4,visibleAppRows=2,appHitBase=1000;
static int appScroll=0,appWheel=0;
static int appRows(){
    int count=0;
    for(const auto& app:prefs.apps)if(app.path[0])count++;
    return (count+appColumns-1)/appColumns;
}
static void scrollApps(int rows){
    int next=appScroll+rows,limit=appRows()-visibleAppRows;
    if(next>limit)next=limit;
    if(next<0)next=0;
    if(next==appScroll)return;
    appScroll=next;
    hover=previousHover=-1;
    pressed=-1;
    repaint();
}
static void appScrollButton(Graphics& g,int id,int x,bool up){
    addHit(id,x,61,40,42,up?L"Scroll up":L"Scroll down");
    hoverBox(g,id,x,61,40,42);
    int start=up?85:79,middle=up?79:85;
    line(g,x+13,start,x+20,middle,secondary(),2);
    line(g,x+20,middle,x+27,start,secondary(),2);
}
static void drawApps(Graphics& g,HDC dc){
    label(dc,34,64,350,33,L"Apps",primary(),2);
    int rows=appRows();
    action(g,dc,506,536,61,110,L"Edit",Gear);
    if(!rows){
        action(g,dc,507,216,151,248,L"Add an app or shortcut",Plus);
        return;
    }
    if(appScroll>0)appScrollButton(g,508,442,true);
    if(appScroll+visibleAppRows<rows)appScrollButton(g,509,486,false);
    constexpr int tileSize=61,iconSize=44,spacing=152,labelHeight=24,rowHeight=85;
    const int first=appScroll*appColumns,last=first+visibleAppRows*appColumns;
    int item=0;
    for(int i=0;i<appLimit;i++){
        const auto& app=prefs.apps[i];
        if(!app.path[0])continue;
        int index=item++;
        if(index<first)continue;
        if(index>=last)break;
        int column=index%appColumns,row=(index-first)/appColumns;
        int center=112+column*spacing,top=(rows==1?134:103)+row*rowHeight;
        int left=center-tileSize/2,id=appHitBase+i;
        const wchar_t* name=app.name[0]?app.name:L"Shortcut";
        addHit(id,center-spacing/2+3,top,spacing-6,tileSize+labelHeight,name);
        rounded(g,left,top,tileSize,tileSize,15,panel(.11));
        hoverBox(g,id,left,top,tileSize,tileSize,15);
        rounded(g,left,top,tileSize,tileSize,15,panel(.19),true);
        if(Bitmap* image=LauncherIcons::image(i,app.path)){
            GraphicsState state=g.Save();
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(image,RectF(F(center-iconSize/2),F(top+(tileSize-iconSize)/2+contentOffset),F(iconSize),F(iconSize)));
            g.Restore(state);
        }
        label(dc,center-spacing/2+3,top+tileSize,spacing-6,labelHeight,name,primary(),1,DT_CENTER);
    }
}
static void drawWorkstations(Graphics& g,HDC dc){
    label(dc,34,64,500,33,L"Workstations",primary(),2);
    int count=0;
    for(int i=0;i<workstationCount;i++){
        const auto& station=prefs.workstations[i];
        if(!station.name[0])continue;
        int center=82+count*103;
        squareTile(g,dc,720+i,center,114,68,Play,station.name,98);
        squareTile(g,dc,730+i,center,224,34,Gear,L"Edit workstation");
        count++;
    }
    if(count==workstationCount)
        squareTile(g,dc,511,621,64,34,Gear,L"Manage workstations");
    else
        squareTile(g,dc,510,count?82+count*103:340,114,68,Plus,
                   count?L"Add":L"Add workstation",count?98:180);
}
static void drawFocus(Graphics& g,HDC dc){ULONGLONG left=focus&&deadline>GetTickCount64()?(deadline-GetTickCount64()+999)/1000:(ULONGLONG)prefs.focusMinutes*60;wchar_t clock[40];swprintf_s(clock,L"%02llu:%02llu",left/60,left%60);icon(g,Timer,45,101,53,accent());label(dc,121,82,290,75,clock,primary(),4);label(dc,123,159,350,24,focus?(inBreak?L"Rest your eyes and stretch a little.":L"Focus session in progress"):L"Ready to start",secondary());action(g,dc,601,449,111,197,focus?(inBreak?L"End break":L"End session"):L"Start focus",focus?Close:Play,focus);rounded(g,35,227,610,4,2,panel(.26));if(focus)rounded(g,35,227,(float)(610*(1-left/(double)(focusTotalSeconds))),4,2,accent());label(dc,35,246,610,22,focus?L"A notification will let you know when it is time for a break.":L"",secondary(),0);}
static void toggle(Graphics& g,HDC dc,int id,int x,int y,int w,const wchar_t* name,bool on){addHit(id,x,y,w,42,name);hoverBox(g,id,x,y,w,42);label(dc,x+12,y,w-70,42,name,primary());rounded(g,x+w-51,y+11,37,20,10,on?accent():panel(.30));ellipse(g,x+w-48+(on?16:0),y+14,14,14,on?base():primary(),true);}
static void drawSettings(Graphics& g,HDC dc){
    heading(dc,L"Settings",L"Appearance, behavior, tools, and shortcuts.");
    squareTile(g,dc,705,103,141,76,Gear,L"Open settings",136);
    squareTile(g,dc,706,245,141,76,Note,L"Quick note",136);
}
static void drawTheme(Graphics& g,HDC dc){
    heading(dc,L"Appearance",L"Choose the island material.");
    for(int i=0;i<2;i++){
        int x=34+i*312;
        bool selectedMode=prefs.glassMode==(i==0);
        addHit(801+i,x,133,300,63,appearanceModes[i]);
        Color sample=i==0?glassBase(prefs):rgbColor(prefs.backgroundRgb);
        rounded(g,x,133,300,63,13,panel(selectedMode?.19:.09));
        rounded(g,x,133,300,63,13,selectedMode?accent():panel(.30),true);
        rounded(g,x+14,151,35,26,8,sample);
        rounded(g,x+14,151,35,26,8,secondary(),true);
        label(dc,x+62,133,190,63,appearanceModes[i],primary(),2);
        if(selectedMode)icon(g,Check,x+255,151,22,accent());
    }
    action(g,dc,803,34,212,612,L"Customize colors",Palette);
}
static void drawMail(Graphics& g,HDC dc){
    const auto& v=connectorView;
    label(dc,34,66,285,31,L"Mail",primary(),2);
    if(connectorFeed!=2)squareTile(g,dc,953,570,64,34,Arrow,L"Refresh");
    squareTile(g,dc,952,621,64,34,Gear,L"Feed settings");
    wchar_t mail[24]=L"Mail",phone[24]=L"Phone";
    if(v.unread>0)swprintf_s(mail,L"Mail (%d)",v.unread);
    if(v.phoneCount>0)swprintf_s(phone,L"Phone (%d)",v.phoneCount);
    squareTile(g,dc,950,79,104,40,Envelope,mail,90,connectorFeed==0);
    squareTile(g,dc,951,175,104,40,Phone,phone,90,connectorFeed==1);
    squareTile(g,dc,954,271,104,40,Voice,L"Discord",90,connectorFeed==2);
    if(connectorFeed==2){
        label(dc,34,169,612,28,L"Discord voice",primary(),2);
        label(dc,34,199,332,64,L"Uses the mute shortcut in the Discord desktop window.",
              secondary(),1,DT_WORDBREAK);
        squareTile(g,dc,955,435,191,55,Mic,L"Toggle mute",140);
        squareTile(g,dc,956,577,191,55,Arrow,L"Open Discord",140);
        return;
    }
    int total=connectorFeed?v.phoneCount:v.mailCount;
    if(!total){label(dc,34,171,612,32,connectorFeed?L"Phone notifications":L"Your latest mail",primary(),2);
        label(dc,34,206,612,28,connectorFeed?L"Set up Android or iPhone in settings.":L"Connect Google in settings to see your inbox.",secondary(),1);
    }else for(int row=0;row<2&&row+scrollOffset<total;row++){
        int i=row+scrollOffset,y=172+row*36;addHit(960+i,34,y,612,35,L"Open notification");hoverBox(g,960+i,34,y,612,35);
        const wchar_t* title=connectorFeed?v.phone[i].title:v.mail[i].title;
        const wchar_t* from=connectorFeed?v.phone[i].app:v.mail[i].sender;
        label(dc,39,y,600,19,title,primary(),1);label(dc,39,y+18,600,17,from,secondary(),0);
    }
    wchar_t phoneState[240];swprintf_s(phoneState,L"Android: %s   ·   iPhone: %s",!prefs.androidEnabled?L"off":v.androidConnected?L"connected":L"setup / offline",!prefs.iphoneEnabled?L"off":v.iphoneConnected?L"subscribed":L"setup / unavailable");
    const wchar_t* state=connectorFeed?phoneState:v.gmailStatus;
    label(dc,34,248,612,27,state&&state[0]?state:L"Open settings to connect. Nothing is shared automatically.",secondary(),0);
}
static void drawPage(Graphics& g,HDC dc,int page,int xOffset,bool interactive){
    int originalHits=hitCount;int dcState=SaveDC(dc);GraphicsState gs=g.Save();
    IntersectClipRect(dc,S(20),S(pageTop),S(660),S(282));
    g.SetClip(RectF(F(20),F(pageTop),F(640),F(224)),CombineModeIntersect);
    SetViewportOrgEx(dc,S(xOffset),0,0);g.TranslateTransform(F(xOffset),0);
    switch(page){case 0:drawMusic(g,dc);break;case 1:drawShelf(g,dc);break;case 2:drawClipboard(g,dc);break;case 3:drawWidgets(g,dc);break;case 5:drawApps(g,dc);break;case 6:drawFocus(g,dc);break;case 7:drawSettings(g,dc);break;case 8:drawTheme(g,dc);break;case 9:drawMail(g,dc);break;case workstationPage:drawWorkstations(g,dc);break;}
    g.Restore(gs);RestoreDC(dc,dcState);if(!interactive)hitCount=originalHits;
}
static void paint(HDC dc){
    bool material=prefs.glassMode&&OpticalGlass::surface;
    if(material)OpticalGlass::paint(dc);
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    if(!material)g.Clear(base());
    hitCount=0;
    contentOpacity=1;
    contentOffset=0;
    if(openness<.005&&!expanded)return;
    contentOpacity=1;
    for(int i=0;i<primaryTabCount;i++){
        int page=primaryPage(i);
        toolbarButton(g,{page,tileBounds(i),primaryGlyphs[i],tabNames[i]},selected==page);
    }
    toolbarButton(g,pinControl,pinned,19);
    toolbarButton(g,mailControl,selected==9);
    toolbarButton(g,appearanceControl,selected==8);
    toolbarButton(g,clearAppsControl,false);
    contentOpacity=1;contentOffset=0;
    if(tabMoving||swiping){int oldX=(int)(-slideDirection*fullWidth*tabReveal),newX=(int)(slideDirection*fullWidth*(1-tabReveal));drawPage(g,dc,slideSource,oldX,false);drawPage(g,dc,slideTarget,newX,false);}
    else drawPage(g,dc,selected,0,true);
    contentOffset=0;contentOpacity=1;
    if(status[0]&&GetTickCount64()<statusUntil){rounded(g,124,246,432,27,10,base());label(dc,137,246,406,27,status,primary(),0,DT_CENTER);}

}
// Deterministic offline render: exercises the production paint path, not a mock UI.
static int renderPreviews(){
    dpi=96;createFonts();wcscpy_s(nowPlaying.title,L"A longer song title that remains fully readable");wcscpy_s(nowPlaying.artist,L"Artist name");nowPlaying.available=true;nowPlaying.status=4;nowPlaying.duration=2460000000LL;nowPlaying.position=1100000000LL;mediaReceived=titleStarted=GetTickCount64();volumeKnown=true;masterVolume=.42f;expanded=pinned=true;openness=toOpen=1;tabReveal=1;
    CreateDirectory(L"tests",0);CreateDirectory(L"tests\\previews",0);
    HDC screen=GetDC(0),dc=CreateCompatibleDC(screen);
    HBITMAP bmp=CreateCompatibleBitmap(screen,S(fullWidth*1.025),S(fullHeight*1.025));
    HGDIOBJ old=SelectObject(dc,bmp);
    CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    int errors=0;
    for(int i=0;i<12;i++){
        prefs.glassMode=i==9||i==10;
        if(prefs.glassMode){
            prefs.glassTintRgb=i==9?0x454e64:0xffb6d3;
            prefs.tintPercent=i==9?28:52;
            prefs.fontRgb=i==9?0xf5f5f7:0x211927;
            configureMaterial();
            if(OpticalGlass::allocate(S(fullWidth*1.025),S(fullHeight*1.025))){
                for(int y=0;y<OpticalGlass::sh;y++)for(int x=0;x<OpticalGlass::sw;x++){
                    double u=x/(double)OpticalGlass::sw,v=y/(double)OpticalGlass::sh;
                    double wave=.5+.5*sin(12*u+5*v+1.8*sin(v*4));
                    BYTE r=(BYTE)(33+54*u+38*wave), g=(BYTE)(44+64*(1-v)+20*wave);
                    BYTE b=(BYTE)(95+100*u+21*wave);
                    OpticalGlass::source[y*OpticalGlass::sw+x]={b,g,r,255};
                }
                OpticalGlass::render();
            }
        }else{
            OpticalGlass::release();
            prefs.backgroundRgb=i==11?0x204b4a:0x18181f;
            prefs.fontRgb=0xf5f5f7;
        }
        selected=i==4?workstationPage:i<9?i:3;
        paint(dc);GdiFlush();
        wchar_t name[100];swprintf_s(name,L"tests\\previews\\%02d.png",i);
        Bitmap image(bmp,0);if(image.Save(name,&png,0)!=Ok)errors++;
    }
    SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);ReleaseDC(0,screen);
    LauncherIcons::releaseAll();
    for(int i=0;i<5;i++)DeleteObject(fonts[i]);return errors?1:0;
}
static bool showSettings(int page);
static void setupTooltips(){
    if(tooltips)DestroyWindow(tooltips);INITCOMMONCONTROLSEX init={sizeof(init),ICC_WIN95_CLASSES};InitCommonControlsEx(&init);
    tooltips=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,0,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,0,0,0,0,win,0,GetModuleHandle(0),0);
    SendMessage(tooltips,TTM_SETDELAYTIME,TTDT_INITIAL,450);SendMessage(tooltips,TTM_SETMAXTIPWIDTH,0,400);
    const auto toolbarTip=[](UINT_PTR id,const RECT& bounds,const wchar_t* name){
        TOOLINFO tool={sizeof(tool)};
        tool.hwnd=win;tool.uFlags=TTF_SUBCLASS;tool.uId=id;
        tool.rect={S(bounds.left),S(bounds.top),S(bounds.right),S(bounds.bottom)};
        tool.lpszText=(wchar_t*)name;
        SendMessage(tooltips,TTM_ADDTOOL,0,(LPARAM)&tool);
    };
    for(int i=0;i<primaryTabCount;i++)toolbarTip(i+1,tileBounds(i),tabNames[i]);
    const ToolbarControl auxiliary[]={pinControl,mailControl,appearanceControl,clearAppsControl};
    for(const auto& control:auxiliary)
        toolbarTip(control.id,control.bounds,control.name);
    TOOLINFO tip={sizeof(tip)};
    tip.hwnd=win;tip.uFlags=TTF_SUBCLASS;
    tip.uId=musicTitleTip;tip.lpszText=nowPlaying.title;
    SendMessage(tooltips,TTM_ADDTOOL,0,(LPARAM)&tip);
    tip.uId=quickNoteTip;
    tip.lpszText=(wchar_t*)L"Quick note";
    SendMessage(tooltips,TTM_ADDTOOL,0,(LPARAM)&tip);
    updatePageTooltips();
}
static Connectors::Config connectorConfig(){Connectors::Config c;c.gmail=prefs.gmailEnabled;c.android=prefs.androidEnabled;c.iphone=prefs.iphoneEnabled;c.unreadOnly=prefs.gmailUnreadOnly;c.suspended=paused;c.mailSeconds=prefs.gmailRefresh;c.phoneSeconds=prefs.phoneRefresh;wcscpy_s(c.clientId,prefs.gmailClientId[0]?prefs.gmailClientId:PublisherGoogle::clientId);wcscpy_s(c.kdePath,prefs.kdeCliPath);wcscpy_s(c.deviceId,prefs.androidDeviceId);wcscpy_s(c.profile,configFile);return c;}
static void configureConnectors(){Connectors::configure(connectorConfig());}
#include "settings.hpp"
static void workstationNotice(const wchar_t* message){
    say(message);
    if(!expanded){
        tray.uFlags=NIF_INFO;
        wcscpy_s(tray.szInfoTitle,L"Workstations");
        wcsncpy_s(tray.szInfo,message,_TRUNCATE);
        Shell_NotifyIcon(NIM_MODIFY,&tray);
    }
}
static void runWorkstation(int index){
    if(index<0||index>=workstationCount||!prefs.workstations[index].name[0])return;
    const auto& station=prefs.workstations[index];
    int failed=0;
    bool hasItems=station.spotify[0]!=0;
    for(const auto& app:station.apps)if(app.path[0]){
        hasItems=true;
        if(!LaunchTargets::shortcut(win,app.path))failed++;
    }
    for(const auto& tab:station.tabs)if(tab[0])hasItems=true;
    auto tabs=LaunchTargets::openTabs(win,station.tabs,true);
    failed+=tabs.failed;
    bool spotifyOpened=false;
    if(station.spotify[0]&&!(SpotifyPlayback::linked(configFile)&&SpotifyPlayback::start(win,configFile,station.spotify))){
        spotifyOpened=LaunchTargets::openSpotifyItem(win,station.spotify);
        if(!spotifyOpened)failed++;
    }
    if(!hasItems)workstationNotice(L"Add apps, tabs, or Spotify music to this workstation.");
    else if(failed){wchar_t status[100];swprintf_s(status,L"%d workstation item%s couldn’t open.",failed,failed==1?L"":L"s");workstationNotice(status);}
    else if(spotifyOpened)workstationNotice(tabs.fallback?L"Tabs opened in your default browser. Music opened in Spotify.":L"Music opened in Spotify. Press Play to start it.");
    else if(tabs.fallback)workstationNotice(L"Chrome was unavailable. Tabs opened in your default browser.");
}
static void quickOpen(){
    LauncherDialog::Dialog dialog={prefs.workstations};
    INT_PTR result=LauncherDialog::show(win,dialog);
    if(result==LauncherDialog::extraId){
        bool empty=true;
        for(const auto& station:prefs.workstations)if(station.name[0])empty=false;
        showWorkstationSettings(-1,empty);
    }else if(result==IDOK)runWorkstation(dialog.index);
}
static void pauseFor(unsigned minutes){
    if(minutes){
        if(focus&&!paused)pausedFocusRemaining=deadline>GetTickCount64()?deadline-GetTickCount64():0;
        paused=true;pausedUntil=GetTickCount64()+(ULONGLONG)minutes*60000;
        if(frameClock)CancelWaitableTimer(frameClock);swiping=swipeCandidate=false;pinned=expanded=tracking=false;if(frameTimer)timeEndPeriod(1);moving=tabMoving=hoverMoving=frameTimer=false;
        openness=fromOpen=toOpen=0;hover=previousHover=pressed=-1;
        for(int i=1;i<=5;i++)KillTimer(win,i);
        ShowWindow(win,SW_HIDE);if(glassWindow)ShowWindow(glassWindow,SW_HIDE);releaseBuffer();SetTimer(win,6,minutes*60000,0);
        tray.uFlags=NIF_TIP|NIF_INFO;
        swprintf_s(tray.szTip,L"%s paused • Win+Alt+N to resume",appName());
        _snwprintf_s(tray.szInfoTitle,_countof(tray.szInfoTitle),_TRUNCATE,L"%s paused",appName());
        swprintf_s(tray.szInfo,L"Hidden for %u minutes. Press Win+Alt+N to resume now.",minutes);
        Shell_NotifyIcon(NIM_MODIFY,&tray);
    }else{
        if(!paused)return;
        paused=false;pausedUntil=0;KillTimer(win,6);
        if(focus){deadline=GetTickCount64()+pausedFocusRemaining;SetTimer(win,1,1000,0);}
        place();ShowWindow(win,SW_SHOWNOACTIVATE);excludeOverlayFromShell(win);
        tray.uFlags=NIF_TIP;swprintf_s(tray.szTip,L"%s • hover at top center • Win+Alt+N to pause",appName());
        Shell_NotifyIcon(NIM_MODIFY,&tray);repaint();
    }
    configureConnectors();
}
static void menu(){wchar_t resume[128],quit[128];swprintf_s(resume,L"Resume %s   (Win+Alt+N)",appName());swprintf_s(quit,L"Quit %s",appName());POINT p;GetCursorPos(&p);HMENU m=CreatePopupMenu();if(paused)AppendMenu(m,MF_STRING,6,resume);else{
    HMENU pauses=CreatePopupMenu();AppendMenu(pauses,MF_STRING,11,L"5 minutes");AppendMenu(pauses,MF_STRING,12,L"15 minutes");AppendMenu(pauses,MF_STRING,13,L"30 minutes   (Win+Alt+N)");AppendMenu(pauses,MF_STRING,14,L"1 hour");AppendMenu(m,MF_POPUP,(UINT_PTR)pauses,L"Pause for");}
    AppendMenu(m,MF_STRING|(paused?MF_GRAYED:0),1,pinned?L"Unpin island":L"Keep open");AppendMenu(m,MF_STRING,5,L"Settings…");AppendMenu(m,MF_STRING,7,L"Quick Open…");AppendMenu(m,MF_STRING,2,L"Clear clipboard history");AppendMenu(m,MF_STRING,3,L"Clear file shelf");AppendMenu(m,MF_SEPARATOR,0,0);AppendMenu(m,MF_STRING,4,quit);SetForegroundWindow(win);int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,win,0);DestroyMenu(m);if(id==6)pauseFor(0);if(id>=11&&id<=14){unsigned lengths[]={5,15,30,60};pauseFor(lengths[id-11]);}if(id==1){pinned=!pinned;expand(pinned);repaint();}if(id==2){SecureZeroMemory(clips,sizeof(clips));clipCount=0;scrollOffset=0;repaint();}if(id==3){SecureZeroMemory(files,sizeof(files));fileCount=0;scrollOffset=0;repaint();}if(id==4)DestroyWindow(win);if(id==5)showSettings(0);if(id==7)quickOpen();}
static void activate(int id){
    if(id>=appHitBase&&id<appHitBase+appLimit){
        if(!LaunchTargets::shortcut(win,prefs.apps[id-appHitBase].path))say(L"Windows couldn’t open that shortcut.");
        return;
    }
    if(primaryIndex(id)>=0){choose(id);if(id==7)showSettings(0);return;}switch(id){case 100:NativeMedia::control(0);break;case 101:NativeMedia::control(1);break;case 102:NativeMedia::control(2);break;case 103:media(VK_VOLUME_DOWN);break;case 104:media(VK_VOLUME_UP);break;case 201:SecureZeroMemory(files,sizeof(files));fileCount=0;scrollOffset=0;say(L"Shelf cleared. Your files are untouched.");break;case 301:SecureZeroMemory(clips,sizeof(clips));clipCount=0;scrollOffset=0;say(L"Clipboard history cleared.");break;case 510:showWorkstationSettings(-1,true);break;
case 511:showWorkstationSettings();break;
case 506:showSettings(3);break;
case 507:if(showSettings(3))SendMessage(settingsWindow,WM_COMMAND,420,0);break;
case 508:scrollApps(-1);break;
case 509:scrollApps(1);break;
case 601:inBreak=false;focus=!focus;focusTotalSeconds=prefs.focusMinutes*60;deadline=GetTickCount64()+(ULONGLONG)prefs.focusMinutes*60000;SetTimer(win,1,1000,0);repaint();break;case 701:animations=!animations;saveSettings();if(!animations){openness=toOpen;moving=tabMoving=hoverMoving=false;tabReveal=hoverReveal=1;KillTimer(win,3);if(frameClock)CancelWaitableTimer(frameClock);if(frameTimer)timeEndPeriod(1);frameTimer=false;place();}repaint();break;case 702:docked=!docked;saveSettings();place();repaint();break;case 703:case 900:pinned=!pinned;repaint();break;case 705:showSettings(0);break;case 706:showSettings(4);break;case 704:DestroyWindow(win);break;case 801:case 802:{
    bool glass=id==801;
    if(prefs.glassMode!=glass){prefs.glassMode=glass;OpticalGlass::release();saveSettings();place();repaint();}
    break;
}
case 803:showSettings(0);break;case 901:choose(8);break;
case 902:connectorFeed=0;scrollOffset=0;choose(9);repaint();break;
case 707:case 903:{
    wchar_t result[150]={};
    BackgroundApps::clear(win,result);
    if(result[0])say(result);
    break;
}
case 950:case 951:case 954:connectorFeed=id==954?2:id-950;scrollOffset=0;repaint();break;
case 952:showSettings(5+connectorFeed);break;
case 953:if(connectorFeed!=2){Connectors::refresh();say(L"Refreshing enabled connectors…");}break;
case 955:say(DiscordVoice::toggleMute());break;
case 956:if(!DiscordVoice::openApp(win))say(L"Install the Discord desktop app first.");break;
default:if(id>=720&&id<720+workstationCount)runWorkstation(id-720);if(id>=730&&id<730+workstationCount)showWorkstationSettings(id-730);if(id>=960&&id<980){int i=id-960;if(!connectorFeed&&i<connectorView.mailCount&&Connectors::mailId(connectorView.mail[i].id)){wchar_t url[180],mail[80];ConnectorNet::wide(connectorView.mail[i].id,mail,80);swprintf_s(url,L"https://mail.google.com/mail/u/0/#inbox/%s",mail);open(url);}else if(connectorFeed&&i<connectorView.phoneCount){auto& p=connectorView.phone[i];wchar_t detail[1100];swprintf_s(detail,L"%s\r\n\r\n%s",p.title,p.body);MessageBox(win,detail,p.app,MB_OK|MB_ICONINFORMATION);}}if(id>=210&&id<222&&id-210<fileCount){wchar_t args[MAX_PATH+30];swprintf_s(args,L"/select,\"%s\"",files[id-210]);ShellExecute(win,L"open",L"explorer.exe",args,0,SW_SHOWNORMAL);}if(id>=310&&id<322)copyClip(id-310);}}
static void tickAnimation(){double clock=profileNow();if(moving){if(frameLast){double interval=clock-frameLast;frameSum+=interval;frameCount++;if(interval>frameWorst)frameWorst=interval;}frameLast=clock;}ULONGLONG now=GetTickCount64();if(moving){double t=(now-motionStart)/(expanded?(double)prefs.animationMs:prefs.animationMs*.733);double spring=t>=1?1:1-exp(-9*t)*(cos(7*t)+9.0/7*sin(7*t));openness=lerp(fromOpen,toOpen,spring);if(t>=1){openness=toOpen;moving=false;if(!expanded)releaseBuffer();}place();}if(tabMoving){double t=(now-tabStart)/190.0;tabReveal=lerp(slideFrom,slideGoal,ease(t));if(t>=1){tabMoving=false;tabReveal=slideGoal;repaint();}}if(hoverMoving){double t=(now-hoverStart)/120.0;hoverReveal=ease(t);if(t>=1){hoverMoving=false;previousHover=-1;}}animationTimer();if(tabMoving||hoverMoving||(expanded&&selected==0&&((prefs.waveformEnabled&&nowPlaying.status==4)||titleOverflow)&&now-lastMusicPaint>=33)){lastMusicPaint=now;repaint();}else if(moving)InvalidateRect(win,0,FALSE);}
static LRESULT CALLBACK proc(HWND h,UINT msg,WPARAM wp,LPARAM lp){switch(msg){
case WM_MOUSEMOVE:{if(paused)return 0;if(swipeCandidate&&(wp&MK_LBUTTON)){moveSwipe((int)(GET_X_LPARAM(lp)/F(1)),(int)(GET_Y_LPARAM(lp)/F(1)));if(swiping)return 0;}if(pressed==105){setVolumeAt((int)(GET_X_LPARAM(lp)/F(1)));return 0;}
if(!tracking){TRACKMOUSEEVENT t={sizeof(t),TME_LEAVE,h,0};TrackMouseEvent(&t);tracking=true;}KillTimer(h,2);
if(prefs.hoverEnabled&&!expanded&&!moving)SetTimer(h,5,prefs.hoverDelay,0);
int id=hitAt((int)(MulDiv(GET_X_LPARAM(lp),96,dpi)/uiScale),(int)(MulDiv(GET_Y_LPARAM(lp),96,dpi)/uiScale));if(id!=hover){previousHover=hover;hover=id;hoverReveal=animations?0:1;hoverStart=GetTickCount64();hoverMoving=animations;animationTimer();repaint();}SetCursor(LoadCursor(0,id>=0?IDC_HAND:IDC_ARROW));return 0;}
case WM_MOUSELEAVE:if(!expanded&&!moving)OpticalGlass::release();tracking=false;KillTimer(h,5);previousHover=hover;hover=-1;hoverReveal=animations?0:1;hoverStart=GetTickCount64();hoverMoving=animations;animationTimer();if(!pinned&&!swipeCandidate&&pressed<0)SetTimer(h,2,prefs.closeDelay,0);repaint();return 0;
case WM_TIMER:if(wp==6){pauseFor(0);return 0;}if(wp==3)tickAnimation();else if(wp==5){KillTimer(h,5);if(tracking)expand(true);}else if(wp==4){KillTimer(h,4);status[0]=0;repaint();}else if(wp==2){KillTimer(h,2);POINT p;GetCursorPos(&p);RECT r;GetWindowRect(h,&r);ScreenToClient(win,&p);HRGN hit=CreateRectRgn(0,0,0,0);GetWindowRgn(win,hit);bool inside=PtInRegion(hit,p.x,p.y)!=FALSE;DeleteObject(hit);if(!pinned&&!inside&&GetCapture()!=h)expand(false);}else if(wp==1){if(focus&&GetTickCount64()>=deadline){bool finishingBreak=inBreak;bool startingBreak=prefs.automaticBreak&&!inBreak;focus=startingBreak;inBreak=startingBreak;if(startingBreak)focusTotalSeconds=prefs.breakMinutes*60;if(startingBreak)deadline=GetTickCount64()+(ULONGLONG)prefs.breakMinutes*60000;say(startingBreak?L"Focus complete. Your break has started.":L"Session complete.");tray.uFlags=NIF_INFO;wcscpy_s(tray.szInfoTitle,finishingBreak?L"Break complete":L"Focus complete");swprintf_s(tray.szInfo,startingBreak?L"Your %d minute focus session is complete. A %d minute break has started.":L"Your session is complete. Take a moment before the next one.",prefs.focusMinutes,prefs.breakMinutes);Shell_NotifyIcon(NIM_MODIFY,&tray);if(!expanded&&!focus)KillTimer(h,1);}if(expanded){
    if(prefs.glassMode&&!moving&&glassForeground!=GetForegroundWindow()){
        RECT glass;GetWindowRect(win,&glass);
        syncGlass(glass.left,glass.top,glass.right-glass.left,glass.bottom-glass.top);
    }
    if(selected==0){NativeMedia::refresh();readVolume(false);}if(selected==3)sampleCpu();repaint();}}return 0;
case WM_LBUTTONDOWN:pressed=hitAt((int)(MulDiv(GET_X_LPARAM(lp),96,dpi)/uiScale),(int)(MulDiv(GET_Y_LPARAM(lp),96,dpi)/uiScale));if(prefs.swipeEnabled&&expanded&&!moving&&!tabMoving&&GET_Y_LPARAM(lp)>=S(pageTop)&&pressed!=105&&pressed!=106){beginSwipe((int)(GET_X_LPARAM(lp)/F(1)),(int)(GET_Y_LPARAM(lp)/F(1)));SetCapture(h);}else if(pressed>=0)SetCapture(h);if(pressed==105)setVolumeAt((int)(GET_X_LPARAM(lp)/F(1)));repaint();return 0;
case WM_LBUTTONUP:{if(swiping){finishSwipe();pressed=-1;if(GetCapture()==h)ReleaseCapture();return 0;}swipeCandidate=false;int id=hitAt((int)(MulDiv(GET_X_LPARAM(lp),96,dpi)/uiScale),(int)(MulDiv(GET_Y_LPARAM(lp),96,dpi)/uiScale));bool wasPressed=pressed==id;if(pressed==105){setVolumeAt((int)(GET_X_LPARAM(lp)/F(1)));id=-1;}if(pressed==106&&nowPlaying.duration>0){double fraction=((GET_X_LPARAM(lp)/F(1))-34)/612;if(fraction<0)fraction=0;if(fraction>1)fraction=1;NativeMedia::seek((LONGLONG)(fraction*nowPlaying.duration));id=-1;}pressed=-1;if(GetCapture()==h)ReleaseCapture();if(!pinned)SetTimer(h,2,prefs.closeDelay,0);if(!expanded)expand(true);else if(id>=0&&wasPressed)activate(id);repaint();return 0;}
case WM_CAPTURECHANGED:if(swiping)finishSwipe(true);swipeCandidate=false;pressed=-1;return 0;
case WM_CANCELMODE:finishSwipe(true);if(GetCapture()==h)ReleaseCapture();return 0;
case WM_GESTURE:{if(!prefs.swipeEnabled)return DefWindowProc(h,msg,wp,lp);GESTUREINFO info={sizeof(info)};if(!GetGestureInfo((HGESTUREINFO)lp,&info))return DefWindowProc(h,msg,wp,lp);if(info.dwID!=GID_PAN)return DefWindowProc(h,msg,wp,lp);POINT point={info.ptsLocation.x,info.ptsLocation.y};ScreenToClient(h,&point);int x=(int)(point.x/F(1)),y=(int)(point.y/F(1));if(info.dwFlags&GF_BEGIN){int id=hitAt(x,y);if(y<pageTop||id==105||id==106){CloseGestureInfoHandle((HGESTUREINFO)lp);return 0;}beginSwipe(x,y);}else moveSwipe(x,y);if(info.dwFlags&GF_END)finishSwipe();CloseGestureInfoHandle((HGESTUREINFO)lp);return 0;}
case WM_MOUSEHWHEEL:{if(!prefs.swipeEnabled)return 0;if(!expanded||moving||paused)return 0;ULONGLONG now=GetTickCount64();if(now-lastWheelEvent>300)horizontalWheel=0;lastWheelEvent=now;horizontalWheel+=GET_WHEEL_DELTA_WPARAM(wp);if(abs(horizontalWheel)>=60&&now-lastWheel>=190){int direction=horizontalWheel>0?1:-1;choose(nextPrimaryPage(selected,direction),direction);horizontalWheel=0;lastWheel=now;}return 0;}
case WM_MOUSEWHEEL:{if(prefs.swipeEnabled&&(GET_KEYSTATE_WPARAM(wp)&MK_SHIFT)){int direction=GET_WHEEL_DELTA_WPARAM(wp)<0?1:-1;choose(nextPrimaryPage(selected,direction),direction);return 0;}
    if(selected==5){
        if(expanded&&!moving&&!tabMoving&&!swiping){
            appWheel+=GET_WHEEL_DELTA_WPARAM(wp);
            int rows=appWheel/WHEEL_DELTA;
            appWheel%=WHEEL_DELTA;
            if(rows)scrollApps(-rows);
        }
        return 0;
    }
    int count=selected==1?fileCount:selected==2?clipCount:selected==9?(connectorFeed?connectorView.phoneCount:connectorView.mailCount):0;scrollOffset+=GET_WHEEL_DELTA_WPARAM(wp)<0?1:-1;if(scrollOffset<0)scrollOffset=0;int rows=selected==9?2:3;if(scrollOffset>count-rows)scrollOffset=count>rows?count-rows:0;repaint();return 0;}
case WM_RBUTTONUP:menu();return 0;
case WM_DROPFILES:{HDROP drop=(HDROP)wp;UINT count=DragQueryFile(drop,0xffffffff,0,0);for(UINT i=0;i<count&&fileCount<12;i++){UINT len=DragQueryFile(drop,i,0,0);if(len<MAX_PATH&&DragQueryFile(drop,i,files[fileCount],MAX_PATH))fileCount++;}DragFinish(drop);scrollOffset=0;expand(true);choose(1);repaint();return 0;}
case WM_CLIPBOARDUPDATE:captureClipboard();return 0;
case WM_HOTKEY:if(wp==3){pauseFor(paused?0:30);return 0;}if(!paused&&wp==2)toggleIsland();return 0;
case WM_APP+1:if(lp==WM_RBUTTONUP)menu();if(lp==WM_LBUTTONUP){if(paused)pauseFor(0);else toggleIsland();}return 0;
case WM_APP+90:{auto* update=(Connectors::View*)lp;if(update){connectorView=*update;SecureZeroMemory(update,sizeof(*update));HeapFree(GetProcessHeap(),0,update);if(settingsWindow)updateConnectorStatus();if(expanded&&selected==9)repaint();}return 0;}
case SpotifyPlayback::finishedMessage:
    if(wp){updateSpotifySettings();if(settingsWindow&&settingsPage==workstationSettingsPage)settingsError((const wchar_t*)lp);}
    else workstationNotice((const wchar_t*)lp);
    return 0;
case WM_APP+91:return (prefs.gmailEnabled?1:0)|(prefs.androidEnabled?2:0)|(prefs.iphoneEnabled?4:0);
case WM_APP+92:return connectorView.mailCount;case WM_APP+93:return connectorView.phoneCount;
case WM_APP+80:return settingsPage;case WM_APP+81:return focus;case WM_APP+82:return focus&&deadline>GetTickCount64()?(LRESULT)(deadline-GetTickCount64()):0;
case WM_APP+61:showSettings(0);return 0;
case WM_APP+60:if(paused)pauseFor(0);toggleIsland();return 0;
case WM_APP+2:return (moving?1:0)|(tabMoving?2:0)|(hoverMoving?4:0)|((frameTimer&&!(expanded&&selected==0&&nowPlaying.status==4))?8:0);
case WM_APP+3:return selected;
case WM_APP+4:return paused;
case WM_APP+6:return (LRESULT)OpticalGlass::renderMs;
case WM_APP+7:return OpticalGlass::surface!=0;
case WM_APP+8:return expanded;
case WM_APP+83:return pinned;
case WM_APP+30:{
    auto* data=(NativeMedia::Snapshot*)lp;if(wcscmp(nowPlaying.title,data->title))titleStarted=GetTickCount64();nowPlaying=*data;mediaReceived=GetTickCount64();nowPlaying.artwork=0;animationTimer();
    if(data->artChanged){if(albumArt){delete albumArt;albumArt=0;}if(data->artwork){IStream* stream=0;if(SUCCEEDED(CreateStreamOnHGlobal(data->artwork,TRUE,&stream))){data->artwork=0;Bitmap* image=Bitmap::FromStream(stream);if(image&&image->GetLastStatus()==Ok&&image->GetWidth()<=2048&&image->GetHeight()<=2048){albumArt=new Bitmap(128,128,PixelFormat32bppPARGB);Graphics canvas(albumArt);canvas.SetInterpolationMode(InterpolationModeHighQualityBicubic);canvas.DrawImage(image,0,0,128,128);}delete image;stream->Release();}}}
    if(data->artwork)GlobalFree(data->artwork);HeapFree(GetProcessHeap(),0,data);if(expanded&&selected==0)repaint();return 0;}
case WM_APP+51:return swipeCandidate;case WM_APP+52:return swiping;
case WM_APP+44:return volumeKnown;case WM_APP+45:return (LRESULT)(masterVolume*1000);case WM_APP+46:return (LRESULT)(songPosition()/10000);case WM_APP+47:return titleOverflow;
case WM_APP+40:return nowPlaying.available;case WM_APP+41:return nowPlaying.status;case WM_APP+42:return nowPlaying.title[0]!=0;case WM_APP+43:return albumArt!=0;
case WM_APP+20:InterlockedExchange(&framePending,0);if(frameTimer)tickAnimation();return 0;
case WM_APP+10:return frameCount?(LRESULT)(1000*frameSum/frameCount):0;
case WM_APP+11:return (LRESULT)(1000*frameWorst);
case WM_APP+12:return frameCount;
case WM_APP+13:return (LRESULT)(1000*paintWorst);
case WM_APP+5:if(paused)SetTimer(win,6,100,0);return 0;
case WM_KEYDOWN:if(wp==VK_ESCAPE){finishSwipe(true);pinned=false;expand(false);}else if(wp==VK_LEFT)choose(nextPrimaryPage(selected,-1),-1);else if(wp==VK_RIGHT)choose(nextPrimaryPage(selected,1),1);return 0;
case WM_DISPLAYCHANGE:case WM_SETTINGCHANGE:OpticalGlass::release();place();repaint();return 0;
case WM_DPICHANGED:dpi=HIWORD(wp);createFonts();setupTooltips();OpticalGlass::release();releaseBuffer();place();repaint();return 0;
case WM_ERASEBKGND:return 1;
case WM_PAINT:{double start=profileNow();PAINTSTRUCT ps;BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);int w=r.right,hgt=r.bottom;
    if(!backDC||w>bufferWidth||hgt>bufferHeight){
        releaseBuffer();backDC=CreateCompatibleDC(0);bufferWidth=w;bufferHeight=hgt;
        BITMAPINFO dib={};dib.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);dib.bmiHeader.biWidth=w;dib.bmiHeader.biHeight=-hgt;dib.bmiHeader.biPlanes=1;dib.bmiHeader.biBitCount=32;dib.bmiHeader.biCompression=BI_RGB;
        backBitmap=CreateDIBSection(backDC,&dib,DIB_RGB_COLORS,(void**)&scenePixels,0,0);oldBitmap=SelectObject(backDC,backBitmap);cachedPixels=(DWORD*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,w*hgt*4);contentDirty=true;
    }
    if(backBitmap&&contentDirty){
        paint(backDC);GdiFlush();
        for(int y=0;y<hgt;y++){
            DWORD* row=scenePixels+y*bufferWidth;
            for(int x=0;x<w;x++)row[x]|=0xff000000;
        }
        if(cachedPixels)memcpy(cachedPixels,scenePixels,bufferWidth*hgt*4);contentDirty=false;
    }
    if(backBitmap)presentScene();EndPaint(h,&ps);double cost=profileNow()-start;if(cost>paintWorst)paintWorst=cost;return 0;
}
case WM_DESTROY:LauncherIcons::releaseAll();SpotifyPlayback::stop();Connectors::stop();if(settingsWindow)DestroyWindow(settingsWindow);if(outputVolume){outputVolume->Release();outputVolume=0;}NativeMedia::stop();if(albumArt){delete albumArt;albumArt=0;}if(frameStop)SetEvent(frameStop);if(frameThread){WaitForSingleObject(frameThread,1000);CloseHandle(frameThread);frameThread=0;}if(frameClock){CancelWaitableTimer(frameClock);CloseHandle(frameClock);frameClock=0;}if(frameStop){CloseHandle(frameStop);frameStop=0;}OpticalGlass::release();if(frameTimer){timeEndPeriod(1);frameTimer=false;}if(glassWindow){SetWindowLongPtr(win,GWLP_HWNDPARENT,0);HWND oldGlass=glassWindow;glassWindow=0;DestroyWindow(oldGlass);}Shell_NotifyIcon(NIM_DELETE,&tray);RemoveClipboardFormatListener(h);UnregisterHotKey(h,2);UnregisterHotKey(h,3);for(int i=1;i<=6;i++)KillTimer(h,i);releaseBuffer();PostQuitMessage(0);return 0;
default:if(msg==taskbarMessage){excludeOverlayFromShell(win);excludeOverlayFromShell(glassWindow);tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;Shell_NotifyIcon(NIM_ADD,&tray);}return DefWindowProc(h,msg,wp,lp);}}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR args,int){defaultPreferences(prefs);QueryPerformanceFrequency(&profileFrequency);singleton=CreateMutex(0,TRUE,L"Local\\WinPixel.Native.Singleton");if(GetLastError()==ERROR_ALREADY_EXISTS){
    if(wcsstr(args,L"--toggle")||wcsstr(args,L"--show")||wcsstr(args,L"--settings")){HWND running=0;for(int i=0;i<80&&!running;i++){running=FindWindow(L"WinPixelIsland",0);if(!running)Sleep(25);}if(running){DWORD_PTR result=0;SendMessageTimeout(running,wcsstr(args,L"--settings")?WM_APP+61:WM_APP+60,0,0,SMTO_ABORTIFHUNG,1500,&result);}}
    CloseHandle(singleton);return 0;}SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(0,COINIT_APARTMENTTHREADED);GdiplusStartupInput input;GdiplusStartup(&graphicsToken,&input,0);saveEnabled=!wcsstr(args,L"--test")&&!wcsstr(args,L"--render");if(saveEnabled)loadSettings();if(wcsstr(args,L"--render")){int result=renderPreviews();GdiplusShutdown(graphicsToken);CoUninitialize();CloseHandle(singleton);return result;}WNDCLASS wc={};wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=L"WinPixelIsland";wc.hCursor=LoadCursor(0,IDC_ARROW);RegisterClass(&wc);bool showOnLaunch=wcsstr(args,L"--toggle")||wcsstr(args,L"--show");bool preview=wcsstr(args,L"--preview")!=0;if(preview||showOnLaunch){expanded=true;openness=fromOpen=toOpen=1;}win=CreateWindowEx(WS_EX_LAYERED|WS_EX_TOPMOST|(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE),wc.lpszClassName,appName(),WS_POPUP,0,0,smallWidth,smallHeight,0,0,instance,0);WNDCLASS glassClass={};glassClass.lpfnWndProc=glassProc;glassClass.hInstance=instance;glassClass.lpszClassName=L"WinPixelGlass";glassClass.hCursor=LoadCursor(0,IDC_ARROW);RegisterClass(&glassClass);
    glassWindow=CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TOPMOST,L"WinPixelGlass",L"BezelBrew glass",WS_POPUP,0,0,1,1,0,0,instance,0);SetWindowLongPtr(win,GWLP_HWNDPARENT,(LONG_PTR)glassWindow);DragAcceptFiles(glassWindow,TRUE);
    frameStop=CreateEvent(0,TRUE,FALSE,0);frameClock=CreateWaitableTimerEx(0,0,2,TIMER_ALL_ACCESS);if(frameClock)frameThread=CreateThread(0,65536,frameWorker,0,0,0);if(!frameThread&&frameClock){CloseHandle(frameClock);frameClock=0;}GESTURECONFIG gesture={GID_PAN,GC_PAN|GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY,GC_PAN_WITH_SINGLE_FINGER_VERTICALLY};SetGestureConfig(win,0,1,&gesture,sizeof(gesture));NativeMedia::start(win);Connectors::start(win,connectorConfig());dpi=GetDpiForWindow(win);createFonts();setupTooltips();place();if(preview||showOnLaunch){SetTimer(win,1,1000,0);NativeMedia::refresh();readVolume(true);}DragAcceptFiles(win,TRUE);AddClipboardFormatListener(win);// Ctrl+Alt+N is owned by the Windows launch shortcut, so it also works after Quit.
bool pauseRegistered=RegisterHotKey(win,3,MOD_WIN|MOD_ALT|MOD_NOREPEAT,'N');tray.hWnd=win;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=WM_APP+1;tray.hIcon=brandIcon(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON));swprintf_s(tray.szTip,L"%s • right-click for menu",appName());Shell_NotifyIcon(NIM_ADD,&tray);taskbarMessage=RegisterWindowMessage(L"TaskbarCreated");ShowWindow(win,SW_SHOWNOACTIVATE);excludeOverlayFromShell(win);excludeOverlayFromShell(glassWindow);if(!pauseRegistered)say(L"Win+Alt+N is in use. Pause from the tray menu.");if(wcsstr(args,L"--settings"))showSettings(0);MSG msg;while(GetMessage(&msg,0,0,0)>0){if(settingsWindow&&IsDialogMessage(settingsWindow,&msg))continue;TranslateMessage(&msg);DispatchMessage(&msg);}for(int i=0;i<5;i++)DeleteObject(fonts[i]);GdiplusShutdown(graphicsToken);CoUninitialize();CloseHandle(singleton);return 0;}
