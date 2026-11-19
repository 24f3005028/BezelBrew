#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "../src/connectors.hpp"
static int checks=0;
#define REQUIRE(x) do {checks++;if(!(x)){printf("Failed at line %d\n",__LINE__);return 1;}}while(0)
static wchar_t fixtureRoot[MAX_PATH];
static DWORD WINAPI fixtureEnvironment(LPCWSTR name,LPWSTR output,DWORD capacity){if(_wcsicmp(name,L"ProgramFiles"))return 0;DWORD length=(DWORD)wcslen(fixtureRoot);if(length>=capacity)return length+1;wcscpy_s(output,capacity,fixtureRoot);return length;}
int main(int argc,char** argv){
 using ConnectorJson::Document;
 Document mail("{\"id\":\"18abc\",\"payload\":{\"headers\":[{\"name\":\"Subject\",\"value\":\"Hello \\u4e16\\u754c\"},{\"name\":\"From\",\"value\":\"Example <test@example.invalid>\"}]}}");
 REQUIRE(mail.valid);wchar_t out[32];REQUIRE(mail.string(mail.get(0,"id"),out,32)&&!wcscmp(out,L"18abc"));
 Connectors::Mail result={};REQUIRE(Connectors::parseMail(mail.text,result));REQUIRE(!wcscmp(result.title,L"Hello \u4e16\u754c"));
 REQUIRE(!Document("{\"a\":1,}").valid);REQUIRE(!Document("[1,]").valid);REQUIRE(!Document("{\"a\":01}").valid);REQUIRE(!Document("[true]evil").valid);REQUIRE(!Document("\"bad\\q\"").valid);
 REQUIRE(!Connectors::parseMail("{\"id\":\"../evil\"}",result));
 char output[256];REQUIRE(ConnectorNet::challenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk",output,sizeof(output)));REQUIRE(!strcmp(output,"E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
 REQUIRE(ConnectorNet::encode("a&b c/",output,sizeof(output))&&!strcmp(output,"a%26b%20c%2F"));REQUIRE(ConnectorNet::query("state=hello&code=a%2Bb","code",output,sizeof(output))&&!strcmp(output,"a+b"));REQUIRE(!ConnectorNet::query("state=a&state=b","state",output,sizeof(output)));REQUIRE(!ConnectorNet::query("code=a%00b","code",output,sizeof(output)));
 REQUIRE(ConnectorNet::sameSecret("same","same")&&!ConnectorNet::sameSecret("same","other"));
 REQUIRE(!Connectors::deviceId(L"foo --pair")&&Connectors::deviceId(L"abc_1-2"));
 Connectors::Phone phone[8]={};int count=Connectors::parseAndroid("- Mail: A new message\n- Chat: Hello there\n",phone,8);REQUIRE(count==2&&!wcscmp(phone[0].app,L"Mail")&&!wcscmp(phone[1].title,L"Hello there"));
 Connectors::PhoneChoice choices[16]={};int available=Connectors::Setup::choices("test-phone-123 My phone\ninvalid;id Unsafe\ntest-phone-123 Duplicate\ntest-phone-456 Spare phone\r\n",choices,16);REQUIRE(available==2);REQUIRE(!wcscmp(choices[0].name,L"My phone")&&!wcscmp(choices[1].id,L"test-phone-456"));
 REQUIRE(Connectors::Setup::choices("No devices found\n",choices,16)==0);REQUIRE(Connectors::Setup::choices("test-phone-123 My phone\ntest-phone-456 Spare\n",choices,1)==1);
 if(argc>1){ConnectorNet::wide(argv[1],fixtureRoot,MAX_PATH);wchar_t detected[MAX_PATH];REQUIRE(Connectors::Setup::detect(detected,L"",fixtureEnvironment));REQUIRE(Connectors::Setup::executable(detected));REQUIRE(wcsstr(detected,L"KDE Connect\\bin\\kdeconnect-cli.exe")!=0);}
 BYTE attrs[]={0,0x12,0,0,0,0,3,0,'a','p','p',1,5,0,'H','e','l','l','o',3,4,0,'B','o','d','y'};IPhoneAncs::Event event={};
 for(int i=0;i<(int)sizeof(attrs);i++)REQUIRE(IPhoneAncs::parseAttributes(attrs,i,0x12,event)==0);
 REQUIRE(IPhoneAncs::parseAttributes(attrs,sizeof(attrs),0x12,event)==1&&!strcmp(event.title,"Hello")&&!strcmp(event.body,"Body"));REQUIRE(IPhoneAncs::parseAttributes(attrs,sizeof(attrs),0x13,event)==-1);
 attrs[11]=9;REQUIRE(IPhoneAncs::parseAttributes(attrs,sizeof(attrs),0x12,event)==-1);
 printf("%d connector protocol checks passed. No accounts or devices used.\n",checks);return 0;
}
