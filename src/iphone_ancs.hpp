#pragma once
#include <setupapi.h>
#include <bluetoothleapis.h>
namespace IPhoneAncs {
struct Event {DWORD uid;bool removed;char app[256],title[256],body[512];};
static HANDLE device=INVALID_HANDLE_VALUE,wake=0;static HMODULE bluetooth=0;static SRWLOCK lock=SRWLOCK_INIT;
static BLUETOOTH_GATT_EVENT_HANDLE registration=0;static BTH_LE_GATT_CHARACTERISTIC source={},dataSource={},control={};
static BTH_LE_GATT_DESCRIPTOR sourceDescriptor={},dataDescriptor={};
static Event events[16];static int eventCount=0;static DWORD requests[32],removed[32],pendingUid=0;static int requestCount=0,removedCount=0;
static BYTE fragments[4096];static int fragmentLength=0;static bool busy=false;static ULONGLONG pendingAt=0;
static const GUID serviceId={0x7905f431,0xb5ce,0x4e99,{0xa4,0x0f,0x4b,0x1e,0x12,0x2d,0,0xd0}};
static const GUID sourceId={0x9fbf120d,0x6301,0x42d9,{0x8c,0x58,0x25,0xe6,0x99,0xa2,0x1d,0xbd}};
static const GUID dataId={0x22eac6e9,0x24d6,0x4bb5,{0xbe,0x44,0xb3,0x6a,0xce,0x7c,0x7b,0xfb}};
static const GUID controlId={0x69d1d8f3,0x45e1,0x49a8,{0x98,0x21,0x9b,0xbd,0xfd,0xaa,0xd9,0xd9}};
static const GUID deviceInterface={0x781aee18,0x7733,0x4ce4,{0xad,0xd0,0x91,0xf4,0x1c,0x67,0xb5,0x92}};
static DWORD little32(const BYTE* p){return p[0]|((DWORD)p[1]<<8)|((DWORD)p[2]<<16)|((DWORD)p[3]<<24);}
static bool uuid(BTH_LE_UUID value,const GUID& expected){return !value.IsShortUuid&&IsEqualGUID(value.Value.LongUuid,expected);}
static void enqueue(const Event& event){if(eventCount==16){memmove(events,events+1,15*sizeof(Event));eventCount=15;}events[eventCount++]=event;}
static bool deleted(DWORD uid){for(int i=0;i<removedCount;i++)if(removed[i]==uid)return true;return false;}
// Parses fragmented ANCS attribute tuples. Returns 0 for incomplete, 1 complete, -1 malformed.
static int parseAttributes(const BYTE* bytes,int length,DWORD expected,Event& result){if(length<5)return 0;if(bytes[0]!=0||little32(bytes+1)!=expected)return -1;int at=5;BYTE wanted[]={0,1,3};result={};result.uid=expected;for(BYTE id:wanted){if(length-at<3)return 0;int attribute=bytes[at++],size=bytes[at]|(bytes[at+1]<<8);at+=2;if(attribute!=id||size>2048)return -1;if(length-at<size)return 0;char* out=id==0?result.app:id==1?result.title:result.body;int capacity=id==3?512:256;int count=size<capacity-1?size:capacity-1;memcpy(out,bytes+at,count);out[count]=0;at+=size;}return at==length?1:-1;}
static void CALLBACK changed(BTH_LE_GATT_EVENT_TYPE,void* parameter,void*){
    auto* event=(BLUETOOTH_GATT_VALUE_CHANGED_EVENT*)parameter;if(!event||!event->CharacteristicValue)return;auto* value=event->CharacteristicValue;if(value->DataSize>4096)return;
    AcquireSRWLockExclusive(&lock);
    if(event->ChangedAttributeHandle==source.CharacteristicValueHandle&&value->DataSize>=8){DWORD uid=little32(value->Data+4);if(value->Data[0]==2){if(removedCount==32){memmove(removed,removed+1,31*sizeof(DWORD));removedCount=31;}removed[removedCount++]=uid;Event gone={};gone.uid=uid;gone.removed=true;enqueue(gone);}else if(value->Data[0]<=1){for(int i=0;i<removedCount;i++)if(removed[i]==uid){removed[i]=removed[--removedCount];break;}if(requestCount<32)requests[requestCount++]=uid;}}
    else if(event->ChangedAttributeHandle==dataSource.CharacteristicValueHandle&&busy){if(fragmentLength+(int)value->DataSize>4096){busy=false;fragmentLength=0;}else{memcpy(fragments+fragmentLength,value->Data,value->DataSize);fragmentLength+=value->DataSize;Event item={};int parsed=parseAttributes(fragments,fragmentLength,pendingUid,item);if(parsed){busy=false;fragmentLength=0;if(parsed>0&&!deleted(item.uid))enqueue(item);}}}
    ReleaseSRWLockExclusive(&lock);if(wake)SetEvent(wake);
}
static bool setSubscription(const BTH_LE_GATT_DESCRIPTOR& descriptor,bool enabled){auto set=(decltype(&::BluetoothGATTSetDescriptorValue))GetProcAddress(bluetooth,"BluetoothGATTSetDescriptorValue");if(!set)return false;BTH_LE_GATT_DESCRIPTOR_VALUE value={};value.DescriptorType=ClientCharacteristicConfiguration;value.ClientCharacteristicConfiguration.IsSubscribeToNotification=enabled;auto copy=descriptor;return SUCCEEDED(set(device,&copy,&value,BLUETOOTH_GATT_FLAG_CONNECTION_ENCRYPTED));}
static void close(){if(bluetooth&&registration){auto unreg=(decltype(&::BluetoothGATTUnregisterEvent))GetProcAddress(bluetooth,"BluetoothGATTUnregisterEvent");if(unreg)unreg(registration,0);registration=0;}if(device!=INVALID_HANDLE_VALUE){setSubscription(sourceDescriptor,false);setSubscription(dataDescriptor,false);CloseHandle(device);device=INVALID_HANDLE_VALUE;}AcquireSRWLockExclusive(&lock);SecureZeroMemory(events,sizeof(events));SecureZeroMemory(fragments,sizeof(fragments));eventCount=requestCount=removedCount=fragmentLength=0;busy=false;ReleaseSRWLockExclusive(&lock);}
static bool connect(HANDLE signal,wchar_t* status,int capacity){
    close();wake=signal;if(!bluetooth)bluetooth=LoadLibrary(L"BluetoothApis.dll");HMODULE setup=LoadLibrary(L"setupapi.dll");if(!bluetooth||!setup){swprintf_s(status,capacity,L"Bluetooth GATT is unavailable on this Windows installation.");if(setup)FreeLibrary(setup);return false;}
#define LOAD(api,library) auto api=(decltype(&::api))GetProcAddress(library,#api)
    LOAD(SetupDiGetClassDevsW,setup);LOAD(SetupDiEnumDeviceInterfaces,setup);LOAD(SetupDiGetDeviceInterfaceDetailW,setup);LOAD(SetupDiDestroyDeviceInfoList,setup);LOAD(BluetoothGATTGetServices,bluetooth);LOAD(BluetoothGATTGetCharacteristics,bluetooth);LOAD(BluetoothGATTGetDescriptors,bluetooth);LOAD(BluetoothGATTRegisterEvent,bluetooth);
    if(!SetupDiGetClassDevsW||!SetupDiEnumDeviceInterfaces||!SetupDiGetDeviceInterfaceDetailW||!SetupDiDestroyDeviceInfoList||!BluetoothGATTGetServices||!BluetoothGATTGetCharacteristics||!BluetoothGATTGetDescriptors||!BluetoothGATTRegisterEvent){FreeLibrary(setup);swprintf_s(status,capacity,L"This Windows Bluetooth driver lacks required GATT APIs.");return false;}
    HDEVINFO devices=SetupDiGetClassDevsW(&deviceInterface,0,0,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);bool found=false;
    if(devices!=INVALID_HANDLE_VALUE)for(DWORD index=0;index<64&&!found;index++){SP_DEVICE_INTERFACE_DATA info={sizeof(info)};if(!SetupDiEnumDeviceInterfaces(devices,0,&deviceInterface,index,&info))break;DWORD needed=0;SetupDiGetDeviceInterfaceDetailW(devices,&info,0,0,&needed,0);if(needed>8192||needed<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))continue;auto* path=(SP_DEVICE_INTERFACE_DETAIL_DATA_W*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,needed);if(!path)continue;path->cbSize=sizeof(*path);if(!SetupDiGetDeviceInterfaceDetailW(devices,&info,path,needed,0,0)){HeapFree(GetProcessHeap(),0,path);continue;}HANDLE candidate=CreateFile(path->DevicePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);HeapFree(GetProcessHeap(),0,path);if(candidate==INVALID_HANDLE_VALUE)continue;
        BTH_LE_GATT_SERVICE services[64];USHORT count=0;HRESULT result=BluetoothGATTGetServices(candidate,64,services,&count,BLUETOOTH_GATT_FLAG_NONE);if(SUCCEEDED(result))for(int i=0;i<count&&i<64;i++)if(uuid(services[i].ServiceUuid,serviceId)){
            BTH_LE_GATT_CHARACTERISTIC chars[16];USHORT length=0;if(FAILED(BluetoothGATTGetCharacteristics(candidate,&services[i],16,chars,&length,0)))continue;source={};dataSource={};control={};for(int n=0;n<length&&n<16;n++){if(uuid(chars[n].CharacteristicUuid,sourceId))source=chars[n];if(uuid(chars[n].CharacteristicUuid,dataId))dataSource=chars[n];if(uuid(chars[n].CharacteristicUuid,controlId))control=chars[n];}
            if(!source.CharacteristicValueHandle||!dataSource.CharacteristicValueHandle||!control.CharacteristicValueHandle)continue;device=candidate;
            auto descriptor=[&](BTH_LE_GATT_CHARACTERISTIC& characteristic,BTH_LE_GATT_DESCRIPTOR& destination){BTH_LE_GATT_DESCRIPTOR list[16];USHORT n=0;if(FAILED(BluetoothGATTGetDescriptors(device,&characteristic,16,list,&n,0)))return false;for(int d=0;d<n&&d<16;d++)if(list[d].DescriptorType==ClientCharacteristicConfiguration){destination=list[d];return true;}return false;};
            struct Registration {USHORT count;BTH_LE_GATT_CHARACTERISTIC values[2];} registrationInfo={2,{source,dataSource}};
            if(descriptor(source,sourceDescriptor)&&descriptor(dataSource,dataDescriptor)&&SUCCEEDED(BluetoothGATTRegisterEvent(device,CharacteristicValueChangedEvent,&registrationInfo,changed,0,&registration,0))&&setSubscription(dataDescriptor,true)&&setSubscription(sourceDescriptor,true)){found=true;swprintf_s(status,capacity,L"Bluetooth notifications are ready. Send a notification to your iPhone to test the connection.");break;}close();candidate=INVALID_HANDLE_VALUE;device=INVALID_HANDLE_VALUE;break;
        }
        if(!found&&candidate!=INVALID_HANDLE_VALUE)CloseHandle(candidate);
    }
    if(devices!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(devices);FreeLibrary(setup);if(!found)swprintf_s(status,capacity,L"Windows is not sharing iPhone notifications yet. Pair in Bluetooth, allow Share System Notifications on your iPhone, then try again.");return found;
#undef LOAD
}
static int drain(Event* output,int capacity){AcquireSRWLockExclusive(&lock);int count=eventCount<capacity?eventCount:capacity;memcpy(output,events,count*sizeof(Event));if(count<eventCount)memmove(events,events+count,(eventCount-count)*sizeof(Event));eventCount-=count;ReleaseSRWLockExclusive(&lock);return count;}
static void pump(){if(device==INVALID_HANDLE_VALUE)return;DWORD uid=0;bool send=false;AcquireSRWLockExclusive(&lock);if(busy&&GetTickCount64()-pendingAt>3000){busy=false;fragmentLength=0;}if(!busy&&requestCount){uid=requests[0];memmove(requests,requests+1,(--requestCount)*sizeof(DWORD));pendingUid=uid;pendingAt=GetTickCount64();busy=true;fragmentLength=0;send=true;}ReleaseSRWLockExclusive(&lock);if(!send)return;BYTE memory[sizeof(BTH_LE_GATT_CHARACTERISTIC_VALUE)+16]={};auto* value=(BTH_LE_GATT_CHARACTERISTIC_VALUE*)memory;BYTE command[]={0,(BYTE)uid,(BYTE)(uid>>8),(BYTE)(uid>>16),(BYTE)(uid>>24),0,1,160,0,3,0,2};value->DataSize=sizeof(command);memcpy(value->Data,command,sizeof(command));auto set=(decltype(&::BluetoothGATTSetCharacteristicValue))GetProcAddress(bluetooth,"BluetoothGATTSetCharacteristicValue");if(!set||FAILED(set(device,&control,value,0,BLUETOOTH_GATT_FLAG_CONNECTION_ENCRYPTED))){AcquireSRWLockExclusive(&lock);busy=false;fragmentLength=0;ReleaseSRWLockExclusive(&lock);}}
static bool pending(){AcquireSRWLockShared(&lock);bool pending=busy||requestCount;ReleaseSRWLockShared(&lock);return pending;}
}
