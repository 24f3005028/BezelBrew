#pragma once
#include <roapi.h>
#include <winstring.h>
#include <objidl.h>
// Native Windows.Media.Control ABI, per Microsoft's windows-rs definitions.
namespace NativeMedia {
static HMODULE runtime=LoadLibrary(L"combase.dll");
static auto RoInitialize=(HRESULT(WINAPI*)(RO_INIT_TYPE))GetProcAddress(runtime,"RoInitialize");
static auto RoUninitialize=(void(WINAPI*)())GetProcAddress(runtime,"RoUninitialize");
static auto RoGetActivationFactory=(HRESULT(WINAPI*)(HSTRING,REFIID,void**))GetProcAddress(runtime,"RoGetActivationFactory");
static auto WindowsCreateString=(HRESULT(WINAPI*)(PCNZWCH,UINT32,HSTRING*))GetProcAddress(runtime,"WindowsCreateString");
static auto WindowsDeleteString=(HRESULT(WINAPI*)(HSTRING))GetProcAddress(runtime,"WindowsDeleteString");
static auto WindowsGetStringRawBuffer=(PCWSTR(WINAPI*)(HSTRING,UINT32*))GetProcAddress(runtime,"WindowsGetStringRawBuffer");

struct Snapshot {wchar_t title[256],artist[256],app[160];int status;bool available,artChanged;LONGLONG position,duration;HGLOBAL artwork;};
static HANDLE request=0,stopEvent=0,thread=0;static HWND target=0;static LONG command=-1;static LONG64 seekTarget=0;
static const GUID staticsId={0x2050c4ee,0x11a0,0x57de,{0xae,0xd7,0xc9,0x7c,0x70,0x33,0x82,0x45}};
static const GUID asyncInfoId={0x00000036,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID randomStreamId={0x905a0fe1,0xbc53,0x11df,{0x8c,0x49,0,0x1e,0x4f,0xc6,0x86,0xda}};
template<int slot,typename... A> static HRESULT call(void* p,A... args){if(!p)return E_POINTER;auto fn=(HRESULT(WINAPI*)(void*,A...))(*(void***)p)[slot];return fn(p,args...);}
static void release(void* p){if(p)((IUnknown*)p)->Release();}
static HRESULT finish(void* op,void** result){if(!op)return E_POINTER;void* info=0;HRESULT hr=((IUnknown*)op)->QueryInterface(asyncInfoId,&info);if(FAILED(hr))return hr;int status=0;ULONGLONG until=GetTickCount64()+2500;while(SUCCEEDED(hr=call<7>(info,&status))&&status==0&&GetTickCount64()<until){if(WaitForSingleObject(stopEvent,10)==WAIT_OBJECT_0){hr=E_ABORT;break;}}if(SUCCEEDED(hr)&&status==1)hr=call<8>(op,result);else{call<9>(info);hr=E_ABORT;}release(info);return hr;}
static void getString(void* object,int slot,wchar_t* text,size_t capacity){HSTRING str=0;HRESULT hr=slot==6?call<6>(object,&str):slot==9?call<9>(object,&str):call<6>(object,&str);if(SUCCEEDED(hr)&&str){UINT32 len=0;const wchar_t* value=WindowsGetStringRawBuffer(str,&len);wcsncpy_s(text,capacity,value,_TRUNCATE);}if(str)WindowsDeleteString(str);}
static HGLOBAL thumbnail(void* properties){
    void *ref=0,*op=0,*stream=0,*random=0;IStream* adapter=0;HGLOBAL bytes=0;
    if(SUCCEEDED(call<15>(properties,&ref))&&ref&&SUCCEEDED(call<6>(ref,&op))&&SUCCEEDED(finish(op,&stream))&&stream&&SUCCEEDED(((IUnknown*)stream)->QueryInterface(randomStreamId,&random))){
        typedef HRESULT(WINAPI* Convert)(IUnknown*,REFIID,void**);HMODULE lib=LoadLibrary(L"shcore.dll");auto convert=lib?(Convert)GetProcAddress(lib,"CreateStreamOverRandomAccessStream"):0;
        if(convert&&SUCCEEDED(convert((IUnknown*)random,IID_IStream,(void**)&adapter))){STATSTG stat={};if(SUCCEEDED(adapter->Stat(&stat,STATFLAG_NONAME))&&stat.cbSize.QuadPart>0&&stat.cbSize.QuadPart<=1024*1024){bytes=GlobalAlloc(GMEM_MOVEABLE,(SIZE_T)stat.cbSize.QuadPart);if(bytes){void* dst=GlobalLock(bytes);ULONG count=0;HRESULT hr=dst?adapter->Read(dst,(ULONG)stat.cbSize.QuadPart,&count):E_OUTOFMEMORY;if(dst)GlobalUnlock(bytes);if(FAILED(hr)||count!=stat.cbSize.QuadPart){GlobalFree(bytes);bytes=0;}}}adapter->Release();}if(lib)FreeLibrary(lib);
    }
    release(random);release(stream);release(op);release(ref);return bytes;
}
static DWORD WINAPI run(void*){
    RoInitialize(RO_INIT_MULTITHREADED);void* manager=0;Snapshot last={};bool haveArt=false;
    HANDLE events[]={stopEvent,request};
    while(WaitForMultipleObjects(2,events,FALSE,INFINITE)==WAIT_OBJECT_0+1){
        if(!manager){HSTRING name=0;const wchar_t* cls=L"Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager";WindowsCreateString(cls,(UINT32)wcslen(cls),&name);void *factory=0,*op=0;if(SUCCEEDED(RoGetActivationFactory(name,staticsId,&factory))&&SUCCEEDED(call<6>(factory,&op)))finish(op,&manager);release(op);release(factory);WindowsDeleteString(name);}
        Snapshot* data=(Snapshot*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Snapshot));if(!data)continue;
        void* session=0;call<6>(manager,&session);
        if(session){
            data->available=true;getString(session,6,data->app,160);
            LONG action=InterlockedExchange(&command,-1);if(action>=0){void* actionOp=0;if(action==0)call<17>(session,&actionOp);else if(action==1)call<20>(session,&actionOp);else if(action==2)call<16>(session,&actionOp);else if(action==3){LONGLONG target=InterlockedCompareExchange64(&seekTarget,0,0);void* time=0;LONGLONG start=0;if(SUCCEEDED(call<8>(session,&time))){call<6>(time,&start);release(time);}call<24>(session,target+start,&actionOp);}release(actionOp);}
            void *playback=0,*timeline=0,*op=0,*properties=0;
            if(SUCCEEDED(call<9>(session,&playback))&&playback)call<7>(playback,&data->status);
            if(SUCCEEDED(call<8>(session,&timeline))&&timeline){LONGLONG start=0;call<6>(timeline,&start);call<7>(timeline,&data->duration);call<10>(timeline,&data->position);data->position-=start;data->duration-=start;
                LONGLONG updated=0;call<11>(timeline,&updated);FILETIME time;GetSystemTimeAsFileTime(&time);ULARGE_INTEGER utc;utc.LowPart=time.dwLowDateTime;utc.HighPart=time.dwHighDateTime;
                if(data->status==4&&updated>0&&utc.QuadPart>(ULONGLONG)updated&&utc.QuadPart-updated<864000000000ULL)data->position+=(LONGLONG)(utc.QuadPart-updated);
                if(data->position<0)data->position=0;if(data->duration>0&&data->position>data->duration)data->position=data->duration;}
            if(SUCCEEDED(call<7>(session,&op))&&SUCCEEDED(finish(op,&properties))&&properties){getString(properties,6,data->title,256);getString(properties,9,data->artist,256);data->artChanged=wcscmp(data->title,last.title)||wcscmp(data->artist,last.artist)||wcscmp(data->app,last.app)||!haveArt;if(data->artChanged){data->artwork=thumbnail(properties);haveArt=data->artwork!=0;}}
            release(properties);release(op);release(timeline);release(playback);release(session);
        }else{data->artChanged=last.available;haveArt=false;InterlockedExchange(&command,-1);}
        last=*data;last.artwork=0;
        if(WaitForSingleObject(stopEvent,0)==WAIT_OBJECT_0||!PostMessage(target,WM_APP+30,0,(LPARAM)data)){if(data->artwork)GlobalFree(data->artwork);HeapFree(GetProcessHeap(),0,data);}
    }
    release(manager);RoUninitialize();return 0;
}
static void start(HWND window){target=window;stopEvent=CreateEvent(0,TRUE,FALSE,0);request=CreateEvent(0,FALSE,FALSE,0);thread=CreateThread(0,65536,run,0,0,0);}
static void refresh(){if(request)SetEvent(request);}
static void control(int action){InterlockedExchange(&command,action);refresh();}
static void seek(LONGLONG position){InterlockedExchange64(&seekTarget,position);control(3);}
static void stop(){if(stopEvent)SetEvent(stopEvent);if(thread){WaitForSingleObject(thread,4000);CloseHandle(thread);}if(stopEvent)CloseHandle(stopEvent);if(request)CloseHandle(request);thread=request=stopEvent=0;}
}
