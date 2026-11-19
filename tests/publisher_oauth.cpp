#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "../src/connectors.hpp"
int main(){
    if(!PublisherGoogle::configured())return 1;
    Connectors::Config config;wcscpy_s(config.clientId,PublisherGoogle::clientId);
    char credential[4096]={};bool ok=Connectors::clientSecret(config,credential,sizeof(credential));
    ok=ok&&!strcmp(credential,"test-public-desktop-credential");SecureZeroMemory(credential,sizeof(credential));
    if(!ok)return 2;puts("Publisher Desktop identity works without per-user client import. No sign-in or network requests performed.");return 0;
}
