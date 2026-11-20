"""Connector UI/worker integration using only an isolated profile and local fake CLI."""
import os,time,ctypes as c,subprocess,json
from pathlib import Path
from ctypes import wintypes as w
root=Path(__file__).resolve().parents[1];profile=root/'tests'/'connector-profile.ini'
profile.write_text('[Appearance]\nTheme=0\nAnimations=0\n',encoding='utf-8')
u=c.WinDLL('user32');u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p];u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.GetDpiForWindow.argtypes=[w.HWND]
assert not u.FindWindowW('WinPixelIsland',None),'Quit WinPixel before testing'
p=None

def wait_for(predicate,description):
 for _ in range(100):
  result=predicate()
  if result:return result
  time.sleep(.06)
 raise AssertionError(description)

def launch():
 global p,main,window
 env=os.environ.copy();env['WINPIXEL_CONFIG']=str(profile)
 p=subprocess.Popen([str(root/'dist'/'WinPixel.exe'),'--settings'],env=env)
 main=wait_for(lambda:u.FindWindowW('WinPixelIsland',None),'Main window missing')
 window=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings missing')

def item(i):
 h=u.GetDlgItem(window,i);assert h,f'Control {i} missing';return h

def tab(i):u.SendMessageW(item(8001),0x1330,i,0)
def text(i):
 out=c.create_unicode_buffer(4096);u.SendMessageW(item(i),0xd,4096,c.addressof(out));return out.value

def set_text(i,value):
 out=c.create_unicode_buffer(str(value));u.SendMessageW(item(i),0xc,0,c.addressof(out))
def click(i):u.SendMessageW(window,0x111,i,0)
def check(i,value):u.SendMessageW(item(i),0xf1,int(value),0)
def close():
 if p and p.poll() is None:u.PostMessageW(main,0x10,0,0);p.wait(timeout=10)
try:
 launch();assert u.SendMessageW(main,0x805b,0,0)==0,'Connectors should default off'
 tab(5);click(630);assert item(604) and item(606) and item(609)
 click(606);assert 'app publisher' in text(8002),'Missing client setup was not explained'
 assert u.SendMessageW(main,0x805b,0,0)==0,'Failed Connect enabled Gmail'
 set_text(605,10);click(9002);assert 'between 60 and 900' in text(8002),'Bad refresh accepted'
 set_text(605,180);click(9002)
 tab(6);click(630);set_text(611,root/'tests'/'kde-fixture.exe');set_text(613,'bad --argument');click(9002)
 assert 'Device ID' in text(8002),'CLI argument injection was not blocked'
 set_text(613,'');check(610,True);click(9002)
 wait_for(lambda:u.SendMessageW(main,0x805d,0,0)==2,'Android worker did not load fixture notifications')
 assert 'Phone connected' in text(614),'Pairing status did not update'
 assert u.SendMessageW(main,0x805b,0,0)==2
 u.SendMessageW(main,0x803c,0,0);time.sleep(.1)
 scale=.55*u.GetDpiForWindow(main)/96
 def island_click(x,y):
  point=(int(y*scale)<<16)|int(x*scale)
  u.SendMessageW(main,0x201,1,point);u.SendMessageW(main,0x202,0,point)
 island_click(522,27);assert u.SendMessageW(main,0x8003,0,0)==9,'Mail tile did not open feed'
 island_click(175,124)
 check(610,False);click(9002)
 wait_for(lambda:u.SendMessageW(main,0x805d,0,0)==0,'Disabling Android did not clear notifications')
 click(8411);assert item(615) and item(617) and item(618);check(615,True)
 click(8410);assert item(610)
 click(8411);assert u.SendMessageW(item(615),0xf0,0,0)==1,'Switching phone setup lost the draft'
 click(2)
 assert u.SendMessageW(main,0x805b,0,0)==0,'Cancel applied iPhone draft'
 close();launch();tab(5);click(630);assert text(605)=='180','Connector refresh did not persist'
 tab(6);click(630);assert text(611)==str(root/'tests'/'kde-fixture.exe'),'CLI path did not persist'
 assert u.SendMessageW(item(610),0xf0,0,0)==0,'Disabled state did not persist'
 raw=profile.read_text(encoding='utf-16');assert 'refresh_token' not in raw and 'client_secret' not in raw
 report={'setupPages':True,'disabledByDefault':True,'missingOAuthClientExplained':True,'refreshValidation':True,'cliArgumentValidation':True,'androidFixtureWorker':True,'feedToolbar':True,'disableClearsFeed':True,'cancelDiscardsIphoneDraft':True,'connectorPersistence':True,'noTokensInIni':True,'realAccountsOrDevicesUsed':False}
 (root/'tests'/'connectors-result.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
finally:close()
