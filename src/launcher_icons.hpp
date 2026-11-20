#pragma once
#include <commoncontrols.h>
#include <appmodel.h>
#include <shlwapi.h>
#include <xmllite.h>

namespace LauncherIcons {
struct Entry {
    wchar_t target[MAX_PATH]={};
    HICON icon=nullptr;
    Bitmap* image=nullptr;
};
static Entry entries[appLimit];

static HICON shellIcon(const wchar_t* item,UINT flags=0){
    SHFILEINFO info={};
    if(SHGetFileInfo(item,FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_SYSICONINDEX|flags)){
        IImageList* images=nullptr;
        if(SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE,IID_PPV_ARGS(&images)))){
            HICON icon=nullptr;
            images->GetIcon(info.iIcon,ILD_TRANSPARENT,&icon);
            images->Release();
            if(icon)return icon;
        }
    }
    return SHGetFileInfo(item,FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_ICON|SHGFI_LARGEICON|flags)
        ?info.hIcon:nullptr;
}
static HICON appIcon(const wchar_t* target){
    PIDLIST_ABSOLUTE id=nullptr;
    if(FAILED(SHParseDisplayName(target,nullptr,&id,0,nullptr)))return nullptr;
    HICON icon=shellIcon((const wchar_t*)id,SHGFI_PIDL);
    CoTaskMemFree(id);
    return icon;
}
static const wchar_t* shellApp(const wchar_t* target){
    if(!_wcsnicmp(target,L"shell:AppsFolder\\",17))return target;
    struct Alias {const wchar_t* target;const wchar_t* app;};
    const Alias aliases[]={
        {L"explorer.exe",L"shell:AppsFolder\\Microsoft.Windows.Explorer"},
        {L"ms-settings:",L"shell:AppsFolder\\windows.immersivecontrolpanel_cw5n1h2txyewy!microsoft.windows.immersivecontrolpanel"},
        {L"calc.exe",L"shell:AppsFolder\\Microsoft.WindowsCalculator_8wekyb3d8bbwe!App"},
        {L"notepad.exe",L"shell:AppsFolder\\Microsoft.WindowsNotepad_8wekyb3d8bbwe!App"}
    };
    for(const auto& alias:aliases)if(!_wcsicmp(target,alias.target))return alias.app;
    return nullptr;
}
static bool manifestLogo(const wchar_t* manifest,const wchar_t* application,wchar_t (&relative)[MAX_PATH]){
    IStream* stream=nullptr;
    if(FAILED(SHCreateStreamOnFileEx(manifest,STGM_READ|STGM_SHARE_DENY_NONE,0,FALSE,nullptr,&stream)))return false;
    IXmlReader* reader=nullptr;
    bool found=false;
    if(SUCCEEDED(CreateXmlReader(IID_PPV_ARGS(&reader),nullptr))){
        reader->SetProperty(XmlReaderProperty_DtdProcessing,DtdProcessing_Prohibit);
        if(SUCCEEDED(reader->SetInput(stream))){
            XmlNodeType node;
            bool matches=false;
            while(reader->Read(&node)==S_OK){
                if(node!=XmlNodeType_Element&&node!=XmlNodeType_EndElement)continue;
                const wchar_t* element=nullptr;
                if(FAILED(reader->GetLocalName(&element,nullptr)))continue;
                if(!wcscmp(element,L"Application")){
                    matches=false;
                    if(node==XmlNodeType_Element&&reader->MoveToAttributeByName(L"Id",nullptr)==S_OK){
                        const wchar_t* id=nullptr;
                        matches=reader->GetValue(&id,nullptr)==S_OK&&id&&!_wcsicmp(id,application);
                        reader->MoveToElement();
                    }
                }else if(matches&&node==XmlNodeType_Element&&!wcscmp(element,L"VisualElements")&&
                         reader->MoveToAttributeByName(L"Square44x44Logo",nullptr)==S_OK){
                    const wchar_t* value=nullptr;
                    if(reader->GetValue(&value,nullptr)==S_OK&&value&&value[0]&&value[0]!=L'\\'&&value[0]!=L'/'&&
                       !wcschr(value,L':')&&!wcsstr(value,L"..")&&wcslen(value)<ARRAYSIZE(relative)){
                        wcscpy_s(relative,value);
                        found=true;
                    }
                    break;
                }
            }
        }
        reader->Release();
    }
    stream->Release();
    return found;
}
static Bitmap* logoImage(const wchar_t* root,const wchar_t* relative){
    const wchar_t* extension=wcsrchr(relative,L'.');
    if(!extension)return nullptr;
    const wchar_t* qualifiers[]={L".targetsize-64_altform-unplated",L".targetsize-72_altform-unplated",
        L".targetsize-96_altform-unplated",L".targetsize-48_altform-unplated",
        L".targetsize-64",L".targetsize-48",L".scale-200",L".scale-100",L""};
    for(const auto* qualifier:qualifiers){
        wchar_t path[MAX_PATH*2];
        if(swprintf_s(path,L"%s\\%.*s%s%s",root,(int)(extension-relative),relative,qualifier,extension)<0||
           GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)continue;
        auto* image=Bitmap::FromFile(path);
        if(image&&image->GetLastStatus()==Ok)return image;
        delete image;
    }
    return nullptr;
}
static Bitmap* packageImage(const wchar_t* target){
    const wchar_t* familyStart=target+17;
    const wchar_t* separator=wcschr(familyStart,L'!');
    if(!separator||separator==familyStart||!separator[1])return nullptr;
    wchar_t family[MAX_PATH];
    size_t length=separator-familyStart;
    if(length>=ARRAYSIZE(family))return nullptr;
    wcsncpy_s(family,familyStart,length);
    PWSTR names[32]={};
    wchar_t nameBuffer[MAX_PATH*ARRAYSIZE(names)];
    UINT32 count=ARRAYSIZE(names),capacity=ARRAYSIZE(nameBuffer);
    if(GetPackagesByPackageFamily(family,&count,names,&capacity,nameBuffer)!=ERROR_SUCCESS)return nullptr;
    for(UINT32 i=0;i<count;i++){
        wchar_t root[MAX_PATH*2],manifest[MAX_PATH*2],relative[MAX_PATH];
        UINT32 size=ARRAYSIZE(root);
        if(GetPackagePathByFullName(names[i],&size,root)!=ERROR_SUCCESS||
           swprintf_s(manifest,L"%s\\AppxManifest.xml",root)<0||
           !manifestLogo(manifest,separator+1,relative))continue;
        if(Bitmap* image=logoImage(root,relative))return image;
    }
    return nullptr;
}
static HICON resolve(const wchar_t* target){
    if(!_wcsnicmp(target,L"shell:AppsFolder\\",17))return appIcon(target);
    if(!_wcsnicmp(target,L"https://",8)||!_wcsnicmp(target,L"http://",7)){
        wchar_t chrome[MAX_PATH];
        if(LaunchTargets::chromePath(chrome)){
            if(HICON icon=shellIcon(chrome))return icon;
        }
        return shellIcon(L".url",SHGFI_USEFILEATTRIBUTES);
    }
    wchar_t path[MAX_PATH];
    if(!_wcsnicmp(target,L"ms-settings:",12)){
        UINT length=GetWindowsDirectory(path,ARRAYSIZE(path));
        if(length&&length<ARRAYSIZE(path)&&
           !wcscat_s(path,L"\\ImmersiveControlPanel\\SystemSettings.exe"))return shellIcon(path);
        return nullptr;
    }
    if(target[0]==L'\\'&&target[1]==L'\\')return shellIcon(target,SHGFI_USEFILEATTRIBUTES);
    if(wcspbrk(target,L"\\/"))return shellIcon(target);
    DWORD length=SearchPath(nullptr,target,nullptr,ARRAYSIZE(path),path,nullptr);
    return length&&length<ARRAYSIZE(path)?shellIcon(path):nullptr;
}
static void release(Entry& entry){
    delete entry.image;
    if(entry.icon)DestroyIcon(entry.icon);
    entry={};
}
static Bitmap* transparentImage(HICON icon){
    ICONINFO info={};
    if(!GetIconInfo(icon,&info))return nullptr;
    BITMAP native={};
    const HBITMAP image=info.hbmColor?info.hbmColor:info.hbmMask;
    const bool measured=GetObject(image,sizeof(native),&native)==sizeof(native);
    if(info.hbmColor)DeleteObject(info.hbmColor);
    if(info.hbmMask)DeleteObject(info.hbmMask);
    const int w=native.bmWidth,h=info.hbmColor?native.bmHeight:native.bmHeight/2;
    if(!measured||w<=0||h<=0)return nullptr;

    // Recover coverage from two backgrounds. This also handles legacy AND masks.
    BITMAPINFO format={};
    format.bmiHeader={sizeof(BITMAPINFOHEADER),w,-h,1,32,BI_RGB};
    DWORD* pixels=nullptr;
    HDC dc=CreateCompatibleDC(nullptr);
    HBITMAP canvas=dc?CreateDIBSection(dc,&format,DIB_RGB_COLORS,(void**)&pixels,nullptr,0):nullptr;
    HGDIOBJ old=canvas?SelectObject(dc,canvas):nullptr;
    Bitmap* result=nullptr;
    if(old&&old!=HGDI_ERROR){
        auto* bitmap=new Bitmap(w,h,PixelFormat32bppPARGB);
        BitmapData data={};
        const Rect region(0,0,w,h);
        if(bitmap->LockBits(&region,ImageLockModeWrite,PixelFormat32bppPARGB,&data)==Ok){
            ZeroMemory(pixels,(size_t)w*h*sizeof(DWORD));
            bool drawn=DrawIconEx(dc,0,0,icon,w,h,0,nullptr,DI_NORMAL)!=FALSE;
            GdiFlush();
            for(int y=0;y<h;y++)memcpy((BYTE*)data.Scan0+y*data.Stride,pixels+y*w,w*sizeof(DWORD));
            memset(pixels,255,(size_t)w*h*sizeof(DWORD));
            drawn=DrawIconEx(dc,0,0,icon,w,h,0,nullptr,DI_NORMAL)&&drawn;
            GdiFlush();
            for(int y=0;y<h;y++){
                DWORD* row=(DWORD*)((BYTE*)data.Scan0+y*data.Stride);
                for(int x=0;x<w;x++){
                    DWORD black=row[x],white=pixels[y*w+x];
                    int transparent=(int)(white&255)-(int)(black&255);
                    const BYTE alpha=(BYTE)(255-(transparent>0?transparent:0));
                    row[x]=((DWORD)alpha<<24)|(black&0xffffff);
                }
            }
            bitmap->UnlockBits(&data);
            if(drawn)result=bitmap;
        }
        if(!result)delete bitmap;
        SelectObject(dc,old);
    }
    if(canvas)DeleteObject(canvas);
    if(dc)DeleteDC(dc);
    return result;
}
static Bitmap* image(Entry& entry,const wchar_t* target){
    if(wcscmp(entry.target,target)){
        release(entry);
        wcscpy_s(entry.target,target);
        if(!target[0])return nullptr;
        if(const wchar_t* app=shellApp(target)){
            entry.image=packageImage(app);
            if(entry.image)return entry.image;
        }
        entry.icon=resolve(target);
        if(!entry.icon)entry.icon=CopyIcon(LoadIcon(nullptr,IDI_APPLICATION));
        if(entry.icon)entry.image=transparentImage(entry.icon);
    }
    return entry.image;
}
static Bitmap* image(int slot,const wchar_t* target){return image(entries[slot],target);}
static void releaseAll(){for(auto& entry:entries)release(entry);}
}
