"""Guided setup using synthetic devices; no browser, real phones, or Google login."""
import os,time,ctypes as c,subprocess,json,shutil
from pathlib import Path
from ctypes import wintypes as w
root=Path(__file__).resolve().parents[1];area=root/'tools'/'tmp'/'connector-setup-test';area.mkdir(parents=True,exist_ok=True)
profile=area/'profile.ini';profile.write_text('[Appearance]\nAnimations=0\n',encoding='utf-8')
installation=area/'programs'/'KDE Connect'/'bin';installation.mkdir(parents=True,exist_ok=True)
shutil.copyfile(root/'tests'/'kde-fixture.exe',installation/'kdeconnect-cli.exe')
profile.write_text('[Appearance]\nAnimations=0\n[Connectors]\nKdeCliPath='+str(installation/'kdeconnect-cli.exe')+'\n',encoding='utf-8')
state=area/'state.txt';state.write_text('many')
u=c.WinDLL('user32');u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.IsWindowEnabled.argtypes=[w.HWND];u.IsWindowVisible.argtypes=[w.HWND]
assert not u.FindWindowW('WinPixelIsland',None),'Quit the app before testing'
env=os.environ.copy();env['WINPIXEL_CONFIG']=str(profile);env['BEZELBREW_KDE_FIXTURE_STATE']=str(state)
p=subprocess.Popen([str(root/'dist'/'BezelBrew.exe'),'--settings'],env=env);main=window=None

def wait_for(predicate,description):
 for _ in range(160):
  result=predicate()
  if result:return result
  time.sleep(.04)
 raise AssertionError(description)

def item(i):
 h=u.GetDlgItem(window,i);assert h,f'Missing control {i}';return h

def text(i):
 value=c.create_unicode_buffer(4096);u.SendMessageW(item(i),0xd,4096,c.addressof(value));return value.value

def click(i):u.SendMessageW(window,0x111,i,0)
def page(i):click(8100+i)
def scan(mode):
 state.write_text(mode);started=time.perf_counter();click(631);elapsed=time.perf_counter()-started
 assert elapsed<.2,f'Phone scan blocked the UI for {elapsed:.3f}s'
 wait_for(lambda:u.IsWindowEnabled(item(631)),'Phone discovery did not finish')
try:
 main=wait_for(lambda:u.FindWindowW('WinPixelIsland',None),'Main window missing');window=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings missing');wait_for(lambda:u.IsWindowVisible(window),'Settings not visible')
 page(5);assert not u.GetDlgItem(window,603) and not u.GetDlgItem(window,604),'Developer Google setup leaked into default flow'
 assert not u.IsWindowEnabled(item(606)),'Google sign-in should be unavailable without publisher configuration'
 assert 'app publisher' in text(609) and 'not available' in text(609),'Missing publisher setup was not explained'
 click(606);assert u.SendMessageW(main,0x805b,0,0)==0,'Unavailable Google sign-in enabled the connector'
 click(630);assert item(603) and item(604),'Advanced Google setup was removed';click(630);assert not u.GetDlgItem(window,604)
 page(6);assert not u.GetDlgItem(window,611) and not u.GetDlgItem(window,613),'Path/ID fields leaked into guided Android setup'
 assert text(621)=='PC app found','Installed KDE Connect was not detected automatically'
 assert not u.IsWindowEnabled(item(616)),'Connect should wait for an explicit phone choice'
 scan('many');assert u.SendMessageW(item(640),0x146,0,0)==3,'Friendly device list did not load'
 assert u.SendMessageW(item(640),0x147,0,0)==0,'Multiple phones should require a choice'
 value=c.create_unicode_buffer(256);u.SendMessageW(item(640),0x148,2,c.addressof(value));assert value.value=='Spare test phone','Picker did not show device names'
 u.SendMessageW(item(640),0x14e,2,0);u.SendMessageW(window,0x111,640|(1<<16),0)
 assert u.IsWindowEnabled(item(616)),'Choosing a phone did not enable Connect'
 click(616);wait_for(lambda:u.SendMessageW(main,0x805d,0,0)==1,'The selected second phone was not connected')
 raw=profile.read_text(encoding='utf-16');assert 'AndroidDeviceId=test-phone-456' in raw and 'androidEnabled=1' in raw,'Connect did not save the selected phone automatically'
 assert 'Phone connected' in text(614),'Connection status not shown'
 click(634);assert 'Notification sync' in text(614),'Troubleshooting did not explain notification permission'
 click(635);wait_for(lambda:u.SendMessageW(main,0x805d,0,0)==0,'Disconnect did not clear notifications')
 assert u.SendMessageW(main,0x805b,0,0)==0
 scan('none');assert u.SendMessageW(item(640),0x146,0,0)==1,'CLI stderr was incorrectly treated as a phone'
 assert not u.IsWindowEnabled(item(616)) and 'No paired phone found' in text(614)
 scan('single');assert u.SendMessageW(item(640),0x147,0,0)==1 and u.IsWindowEnabled(item(616)),'Single phone was not selected automatically'
 scan('fail');assert not u.IsWindowEnabled(item(616)) and 'Open KDE Connect' in text(614),'Companion failure was not actionable'
 assert text(8106)=='Phone' and item(8410) and item(8411)
 click(8411);assert item(617) and item(622) and item(623)
 assert 'ANCS' not in text(618),'Bluetooth protocol jargon leaked into normal setup'
 assert u.SendMessageW(main,0x805b,0,0)==0,'Opening guided pages silently enabled a connector'
 click(8410);assert item(631) and not u.GetDlgItem(window,617),'Android did not return under Phone'
 page(7);assert text(8107)=='Discord' and item(8412) and item(8413)
 result={'developerFieldsHidden':True,'missingPublisherSignInHonest':True,'configuredCompanionRecognized':True,'scanDoesNotBlockUi':True,'phoneNamesInPicker':True,'multiplePhonesRequireChoice':True,'selectedPhoneSavedByConnect':True,'disconnectClearsFeed':True,'emptyListDoesNotParseStderr':True,'singlePhoneAutoSelected':True,'actionableConnectionErrors':True,'plainLanguageIphoneSteps':True,'realAccountsOrDevicesUsed':False}
 (root/'tests'/'connector-setup-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
finally:
 if p.poll() is None:u.PostMessageW(main,0x10,0,0);p.wait(timeout=10)
