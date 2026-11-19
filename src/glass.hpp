#pragma once
// Bounded, opening-time optical glass. No capture thread or idle polling.
namespace OpticalGlass {
struct Pixel { unsigned char b,g,r,a; };
static Pixel *source=0,*scratch=0,*surface=0;
static int width=0,height=0,sw=0,sh=0,originX=0,originY=0;
static BITMAPINFO info={};
static double renderMs=0,tint=.28,cornerPixels=0;
static int blurPasses=1,paneWidth=0,paneHeight=0;
static Pixel tintColor={100,78,69,255};
static bool lightInk=true,flatTop=false;
static void release(){if(source)HeapFree(GetProcessHeap(),0,source);if(scratch)HeapFree(GetProcessHeap(),0,scratch);if(surface)HeapFree(GetProcessHeap(),0,surface);source=scratch=surface=0;width=height=sw=sh=0;}
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static unsigned char channel(double v){return (unsigned char)clamp((int)v,0,255);}
static Pixel sample(double x,double y){x=x<0?0:x>sw-1?sw-1:x;y=y<0?0:y>sh-1?sh-1:y;int ix=(int)x,iy=(int)y,jx=clamp(ix+1,0,sw-1),jy=clamp(iy+1,0,sh-1);double fx=x-ix,fy=y-iy;Pixel a=source[iy*sw+ix],b=source[iy*sw+jx],c=source[jy*sw+ix],d=source[jy*sw+jx],out;for(int k=0;k<3;k++)((unsigned char*)&out)[k]=channel((1-fy)*(((unsigned char*)&a)[k]*(1-fx)+((unsigned char*)&b)[k]*fx)+fy*(((unsigned char*)&c)[k]*(1-fx)+((unsigned char*)&d)[k]*fx));out.a=255;return out;}
static void blur(){for(int pass=0;pass<blurPasses;pass++){for(int y=0;y<sh;y++)for(int x=0;x<sw;x++){int sums[3]={};for(int j=-2;j<=2;j++){Pixel p=source[y*sw+clamp(x+j,0,sw-1)];sums[0]+=p.b;sums[1]+=p.g;sums[2]+=p.r;}scratch[y*sw+x]={(BYTE)(sums[0]/5),(BYTE)(sums[1]/5),(BYTE)(sums[2]/5),255};}for(int y=0;y<sh;y++)for(int x=0;x<sw;x++){int sums[3]={};for(int j=-2;j<=2;j++){Pixel p=scratch[clamp(y+j,0,sh-1)*sw+x];sums[0]+=p.b;sums[1]+=p.g;sums[2]+=p.r;}source[y*sw+x]={(BYTE)(sums[0]/5),(BYTE)(sums[1]/5),(BYTE)(sums[2]/5),255};}}}
static bool allocate(int w,int h){
    release();
    if(w<=0||h<=0)return false;
    width=w;height=h;sw=(w+1)/2;sh=(h+1)/2;
    source=(Pixel*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sw*sh*4);scratch=(Pixel*)HeapAlloc(GetProcessHeap(),0,sw*sh*4);surface=(Pixel*)HeapAlloc(GetProcessHeap(),0,w*h*4);
    if(!source||!scratch||!surface){release();return false;}
    return true;
}
static void render(){
    blur();
    int w=width,h=height;
    double paneW=paneWidth>0?paneWidth:w,paneH=paneHeight>0?paneHeight:h;
    double scale=paneW/680.0,radius=fmin(cornerPixels,paneH*.5);
    double edges[1024],rims[1024],shadows[1024];
    for(int i=0;i<1024;i++){
        double d=i*.5;
        edges[i]=exp(-d/(12*scale));
        rims[i]=exp(-pow((d-1.6*scale)/fmax(.65,1.3*scale),2));
        shadows[i]=exp(-pow((d-8*scale)/(4*scale),2));
    }
    for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){
        double px=xx+.5-w*.5,py=yy+.5-paneH*.5;
        double localRadius=flatTop&&py<0?0:radius;
        double qx=fabs(px)-(paneW*.5-localRadius),qy=fabs(py)-(paneH*.5-localRadius);
        double ox=fmax(0,qx),oy=fmax(0,qy),length=sqrt(ox*ox+oy*oy);
        double distance=localRadius-length-fmin(0,fmax(qx,qy));
        double nx=0,ny=0;
        if(length>.001){nx=ox/length;ny=oy/length;}
        else if(qx>qy)nx=1;
        else ny=1;
        nx*=px<0?-1:1;
        ny*=py<0?-1:1;
        int lookup=clamp((int)(fabs(distance)*2),0,1023);
        double envelope=edges[lookup],bend=19*scale*envelope;
        double sx=(xx-px*.027-nx*bend)/2,sy=(yy-py*.027-ny*bend)/2;
        Pixel p=sample(sx,sy),red=sample(sx+nx*.85*envelope,sy+ny*.85*envelope),blue=sample(sx-nx*.85*envelope,sy-ny*.85*envelope);
        double facing=fmax(0,-nx*.4-ny*.72+.38);
        double specular=rims[lookup]*(34+91*facing)-shadows[lookup]*9;
        double top=fmax(0,1-yy/paneH);
        double sheen=14*top*top+7*(1-xx/(double)w)*top;
        double wash=lightInk?9:242;
        double luminance=p.r*.2126+p.g*.7152+p.b*.0722;
        double opposing=(lightInk?luminance:255-luminance);
        double washStrength=.19+.37*fmin(1.0,fmax(0.0,(opposing-105)/130));
        auto shade=[&](double sample,int pigment){
            double tinted=sample+(pigment-sample)*tint;
            return channel(tinted+(wash-tinted)*washStrength+specular+sheen);
        };
        surface[yy*w+xx]={shade(blue.b,tintColor.b),shade(p.g,tintColor.g),shade(red.r,tintColor.r),255};
    }
    info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
}
static bool capture(int x,int y,int w,int h){
    LARGE_INTEGER a,b,f;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&f);
    if(!allocate(w,h))return false;
    originX=x;originY=y;
    BITMAPINFO mini={};mini.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);mini.bmiHeader.biWidth=w;mini.bmiHeader.biHeight=-h;mini.bmiHeader.biPlanes=1;mini.bmiHeader.biBitCount=32;mini.bmiHeader.biCompression=BI_RGB;
    HDC screen=GetDC(0),memory=CreateCompatibleDC(screen);void* bits=0;HBITMAP image=CreateDIBSection(screen,&mini,DIB_RGB_COLORS,&bits,0,0);HGDIOBJ old=SelectObject(memory,image);SetStretchBltMode(memory,HALFTONE);
    bool ok=image&&bits&&BitBlt(memory,0,0,w,h,screen,x,y,SRCCOPY|CAPTUREBLT);
    GdiFlush();
    if(ok){
        auto* input=(Pixel*)bits;
        for(int iy=0;iy<sh;iy++)for(int ix=0;ix<sw;ix++){
            int x0=ix*2,x1=clamp(x0+1,0,w-1),y0=iy*2,y1=clamp(y0+1,0,h-1);
            Pixel a=input[y0*w+x0],b=input[y0*w+x1],c=input[y1*w+x0],d=input[y1*w+x1];
            source[iy*sw+ix]={(BYTE)((a.b+b.b+c.b+d.b)/4),(BYTE)((a.g+b.g+c.g+d.g)/4),
                             (BYTE)((a.r+b.r+c.r+d.r)/4),255};
        }
    }
    SelectObject(memory,old);DeleteObject(image);DeleteDC(memory);ReleaseDC(0,screen);
    if(!ok){release();return false;}
    render();
    QueryPerformanceCounter(&b);renderMs=1000.0*(b.QuadPart-a.QuadPart)/f.QuadPart;
    return true;
}
static void paint(HDC dc){if(surface)SetDIBitsToDevice(dc,0,0,width,height,0,0,0,height,surface,&info,DIB_RGB_COLORS);}
}
