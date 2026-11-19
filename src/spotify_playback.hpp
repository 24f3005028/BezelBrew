#pragma once

namespace SpotifyPlayback {
#define STRINGIFY_SPOTIFY_INNER(value) #value
#define STRINGIFY_SPOTIFY(value) STRINGIFY_SPOTIFY_INNER(value)
#ifdef BEZELBREW_SPOTIFY_CLIENT_ID
static constexpr char clientId[]=STRINGIFY_SPOTIFY(BEZELBREW_SPOTIFY_CLIENT_ID);
#else
static constexpr char clientId[]="";
#endif
#undef STRINGIFY_SPOTIFY
#undef STRINGIFY_SPOTIFY_INNER

constexpr UINT finishedMessage=WM_APP+94;
static HANDLE worker=nullptr, stopEvent=nullptr;
static LONG working=0;
struct Job {
    HWND window;
    bool signIn;
    wchar_t credential[132],track[23];
};
static bool configured(){return clientId[0]!=0;}
static bool busy(){return InterlockedCompareExchange(&working,0,0)!=0;}
static bool stopping(){return WaitForSingleObject(stopEvent,0)==WAIT_OBJECT_0;}
static bool credentialName(const wchar_t* profile,wchar_t (&name)[132]){
    char profileUtf8[MAX_PATH*4],key[MAX_PATH*4+64],hash[64];
    ConnectorNet::utf8(profile,profileUtf8,ARRAYSIZE(profileUtf8));
    if(!*profileUtf8)return false;
    sprintf_s(key,"%s|%s",clientId,profileUtf8);
    if(!ConnectorNet::challenge(key,hash,ARRAYSIZE(hash)))return false;
    wchar_t suffix[64];
    ConnectorNet::wide(hash,suffix,ARRAYSIZE(suffix));
    swprintf_s(name,L"WinPixel/Spotify/%s",suffix);
    return true;
}
static bool linked(const wchar_t* profile){
    wchar_t name[132];
    PCREDENTIAL credential=nullptr;
    if(!configured()||!credentialName(profile,name)||!CredRead(name,CRED_TYPE_GENERIC,0,&credential))return false;
    CredFree(credential);
    return true;
}
static void disconnect(const wchar_t* profile){
    wchar_t name[132];
    if(!busy()&&credentialName(profile,name))CredDelete(name,CRED_TYPE_GENERIC,0);
}
static bool token(const char* form,const wchar_t* credential,char (&access)[4096],bool requireRefresh){
    ConnectorNet::Response response;
    if(!ConnectorNet::request(L"accounts.spotify.com",L"/api/token",L"POST",form,nullptr,response,stopEvent)||
       response.status!=200||!response.body)return false;
    ConnectorJson::Document doc(response.body);
    char refresh[2560]={};
    bool ok=doc.ascii(doc.get(0,"access_token"),access,ARRAYSIZE(access))&&access[0];
    if(ok&&doc.ascii(doc.get(0,"refresh_token"),refresh,ARRAYSIZE(refresh))&&refresh[0])
        ok=!stopping()&&ConnectorNet::writeCredential(credential,refresh,L"BezelBrew Spotify");
    else if(requireRefresh)ok=false;
    SecureZeroMemory(refresh,sizeof(refresh));
    return ok;
}
static bool receiveCode(SOCKET listener,const char* state,char (&code)[2048]){
    ULONGLONG deadline=GetTickCount64()+120000;
    while(!stopping()&&GetTickCount64()<deadline){
        fd_set sockets;
        FD_ZERO(&sockets);FD_SET(listener,&sockets);
        timeval wait={0,250000};
        int ready=select(0,&sockets,nullptr,nullptr,&wait);
        if(ready==SOCKET_ERROR)return false;
        if(!ready)continue;
        SOCKET client=accept(listener,nullptr,nullptr);
        if(client==INVALID_SOCKET)continue;
        DWORD timeout=500;
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(timeout));
        char request[6144]={};
        int used=0;
        ULONGLONG requestDeadline=GetTickCount64()+1500;
        while(used<(int)sizeof(request)-1&&!stopping()&&GetTickCount64()<requestDeadline){
            int got=recv(client,request+used,(int)sizeof(request)-1-used,0);
            if(got<=0)break;
            used+=got;
            if(strstr(request,"\r\n\r\n"))break;
        }
        constexpr char prefix[]="GET /spotify/callback?";
        char* end=strstr(request," HTTP/1.");
        char returnedState[96]={};
        char error[96]={};
        bool matched=false;
        if(end&&!strncmp(request,prefix,sizeof(prefix)-1)){
            *end=0;
            const char* fields=request+sizeof(prefix)-1;
            matched=ConnectorNet::query(fields,"state",returnedState,ARRAYSIZE(returnedState))&&
                    ConnectorNet::sameSecret(state,returnedState);
            if(matched){
                if(!ConnectorNet::query(fields,"code",code,ARRAYSIZE(code)))code[0]=0;
                if(!ConnectorNet::query(fields,"error",error,ARRAYSIZE(error)))error[0]=0;
            }
        }
        bool complete=matched&&((code[0]&&!error[0])||error[0]);
        const char* reply=complete?
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nCache-Control: no-store\r\nConnection: close\r\n\r\nYou may close this tab and return to BezelBrew.":
            "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nCache-Control: no-store\r\nConnection: close\r\n\r\nInvalid Spotify sign-in response.";
        send(client,reply,(int)strlen(reply),0);
        closesocket(client);
        SecureZeroMemory(request,sizeof(request));
        if(complete)return !error[0];
        SecureZeroMemory(code,sizeof(code));
    }
    return false;
}
static bool authorize(const Job& job){
    WSADATA winsock;
    if(WSAStartup(MAKEWORD(2,2),&winsock))return false;
    SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in address={};
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    int length=sizeof(address);
    BOOL exclusive=TRUE;
    bool listening=listener!=INVALID_SOCKET&&setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive))==0&&
        bind(listener,(sockaddr*)&address,sizeof(address))==0&&listen(listener,2)==0&&
        getsockname(listener,(sockaddr*)&address,&length)==0;
    char verifier[64]={},challenge[64]={},state[64]={},code[2048]={};
    char redirect[96],encodedRedirect[288];
    bool received=false,ok=false;
    if(listening&&ConnectorNet::randomToken(verifier,ARRAYSIZE(verifier))&&
       ConnectorNet::challenge(verifier,challenge,ARRAYSIZE(challenge))&&
       ConnectorNet::randomToken(state,ARRAYSIZE(state))){
        sprintf_s(redirect,"http://127.0.0.1:%u/spotify/callback",ntohs(address.sin_port));
        ConnectorNet::encode(redirect,encodedRedirect,ARRAYSIZE(encodedRedirect));
        char url[760];
        sprintf_s(url,"https://accounts.spotify.com/authorize?response_type=code&client_id=%s&scope=user-modify-playback-state&redirect_uri=%s&code_challenge_method=S256&code_challenge=%s&state=%s",
                  clientId,encodedRedirect,challenge,state);
        wchar_t browser[760];
        ConnectorNet::wide(url,browser,ARRAYSIZE(browser));
        if(!stopping()&&LaunchTargets::open(job.window,browser))received=receiveCode(listener,state,code);
    }
    if(listener!=INVALID_SOCKET)closesocket(listener);
    WSACleanup();
    if(received&&!stopping()){
        char encodedCode[6144],form[6800],access[4096]={};
        if(ConnectorNet::encode(code,encodedCode,ARRAYSIZE(encodedCode))){
            sprintf_s(form,"grant_type=authorization_code&client_id=%s&code=%s&redirect_uri=%s&code_verifier=%s",
                      clientId,encodedCode,encodedRedirect,verifier);
            ok=token(form,job.credential,access,true);
        }
        SecureZeroMemory(encodedCode,sizeof(encodedCode));
        SecureZeroMemory(form,sizeof(form));
        SecureZeroMemory(access,sizeof(access));
    }
    SecureZeroMemory(verifier,sizeof(verifier));
    SecureZeroMemory(state,sizeof(state));
    SecureZeroMemory(code,sizeof(code));
    return ok;
}
static const wchar_t* play(const Job& job){
    char refresh[2560]={},encoded[7680],form[7900],access[4096]={};
    DWORD status=0;
    if(ConnectorNet::readCredential(job.credential,refresh,ARRAYSIZE(refresh))&&
       ConnectorNet::encode(refresh,encoded,ARRAYSIZE(encoded))){
        sprintf_s(form,"grant_type=refresh_token&client_id=%s&refresh_token=%s",clientId,encoded);
        if(token(form,job.credential,access,false)&&!stopping()){
            char track[23],json[64];
            ConnectorNet::utf8(job.track,track,ARRAYSIZE(track));
            sprintf_s(json,"{\"uris\":[\"spotify:track:%s\"]}",track);
            ConnectorNet::Response response;
            ConnectorNet::request(L"api.spotify.com",L"/v1/me/player/play",L"PUT",json,access,response,stopEvent,L"application/json");
            status=response.status;
        }
    }
    SecureZeroMemory(refresh,sizeof(refresh));
    SecureZeroMemory(encoded,sizeof(encoded));
    SecureZeroMemory(form,sizeof(form));
    SecureZeroMemory(access,sizeof(access));
    if(status==204)return L"Spotify playback started.";
    if(stopping())return L"";
    if(!LaunchTargets::openTrack(job.window,job.track))return L"Spotify couldn’t play or open the track.";
    return status==404?L"Track opened. Start an active Spotify device for autoplay.":
                       L"Track opened. Spotify couldn’t start playback automatically.";
}
static DWORD WINAPI run(void* data){
    Job job=*(Job*)data;
    HeapFree(GetProcessHeap(),0,data);
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const wchar_t* message=job.signIn?
        (authorize(job)?L"Spotify connected.":L"Spotify sign-in did not complete."):play(job);
    if(SUCCEEDED(com))CoUninitialize();
    bool notify=!stopping();
    InterlockedExchange(&working,0);
    if(notify)PostMessage(job.window,finishedMessage,job.signIn,(LPARAM)message);
    return 0;
}
static bool start(HWND window,const wchar_t* profile,const wchar_t* track=nullptr){
    if(!configured()||busy())return false;
    if(worker){CloseHandle(worker);worker=nullptr;}
    if(!stopEvent)stopEvent=CreateEvent(nullptr,TRUE,FALSE,nullptr);
    if(!stopEvent)return false;
    auto* job=(Job*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Job));
    if(!job)return false;
    if(!credentialName(profile,job->credential)){HeapFree(GetProcessHeap(),0,job);return false;}
    job->window=window;
    job->signIn=track==nullptr;
    if(track)wcscpy_s(job->track,track);
    InterlockedExchange(&working,1);
    worker=CreateThread(nullptr,0,run,job,0,nullptr);
    if(worker)return true;
    InterlockedExchange(&working,0);
    HeapFree(GetProcessHeap(),0,job);
    return false;
}
static void stop(){
    if(stopEvent)SetEvent(stopEvent);
    if(worker){WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=nullptr;}
    if(stopEvent){CloseHandle(stopEvent);stopEvent=nullptr;}
}
}
