"""Settings integration test using an isolated profile; never edits the real profile."""
import os,time,ctypes as c,subprocess,json
from pathlib import Path
from ctypes import wintypes as w
root=Path(__file__).resolve().parents[1];profile=root/'tests'/'settings-profile.ini'
profile.write_text('[Appearance]\nTheme=2\nAnimations=1\n[Behavior]\ndefaultTab=4\n',encoding='utf-8')
u=c.WinDLL('user32');u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p];u.SetProcessDpiAwarenessContext(c.c_void_p(-4));u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.SetWindowTextW.argtypes=[w.HWND,w.LPCWSTR];u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int]
u.IsWindowVisible.argtypes=[w.HWND];u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
class Header(c.Structure):_fields_=[('hwndFrom',w.HWND),('idFrom',c.c_size_t),('code',w.UINT)]
assert not u.FindWindowW('WinPixelIsland',None),'Quit WinPixel before testing'
env=os.environ.copy();env['WINPIXEL_CONFIG']=str(profile)
p=None;window=None

def wait_window(cls):
 for _ in range(120):
  h=u.FindWindowW(cls,None)
  if h:return h
  time.sleep(.1)
 raise AssertionError(f'{cls} did not appear')
def launch():
 global p,window,main
 p=subprocess.Popen([str(root/'dist'/'WinPixel.exe'),'--settings'],env=env)
 main=wait_window('WinPixelIsland');window=wait_window('WinPixelSettings')
 for _ in range(40):
  if u.IsWindowVisible(window):break
  time.sleep(.05)
 assert u.IsWindowVisible(window),'Settings did not open visibly'
def close_app():
 u.PostMessageW(main,0x10,0,0);p.wait(timeout=7)
def item(i):
 h=u.GetDlgItem(window,i);assert h,f'Control {i} is missing';return h
def set_text(i,value):
 buffer=c.create_unicode_buffer(str(value));u.SendMessageW(item(i),0x0C,0,c.addressof(buffer))
def text(i):
 result=c.create_unicode_buffer(4096);u.SendMessageW(item(i),0x0D,4096,c.addressof(result));return result.value
def tab(page):
 h=item(8001)
 u.SendMessageW(h,0x1330,page,0) # TCM_SETCURFOCUS emits notifications inside the app
 assert u.SendMessageW(h,0x130b,0,0)==page,'Tab selection failed'
def click(i):
 u.SendMessageW(window,0x111,i,0)
 if i==9002:print('Apply status:',text(8002),'page',u.SendMessageW(main,0x8050,0,0),'hasAccent',bool(u.GetDlgItem(window,102)),'hasDelay',bool(u.GetDlgItem(window,207)),flush=True)
def saved():return profile.read_text(encoding='utf-16')
try:
 launch()
 assert u.SendMessageW(main,0x8003,0,0)==3,'The retired starting section did not fall back to Widgets'
 assert text(114)=='241721','Legacy Rosé appearance did not migrate to a solid color'
 assert not u.GetDlgItem(window,116) and not u.GetDlgItem(window,107),'Solid showed glass controls'
 click(8200);assert not u.GetDlgItem(window,114),'Glass showed the solid-background control'
 set_text(116,'FFB6D3')
 click(8201);set_text(102,'18A999');set_text(114,'234567');set_text(115,'FDF8F3')
 u.SendMessageW(item(103),0x405,1,120);u.SendMessageW(item(104),0x405,1,105);u.SendMessageW(item(105),0x405,1,22)
 click(9002);assert 'sizePercent=120' in saved() and 'textPercent=105' in saved()
 assert all(entry in saved() for entry in ('glassMode=0','backgroundRgb=2311527','fontRgb=16644339','glassTintRgb=16758483'))
 set_text(8301,'Work Black');click(8302)
 assert 'Name1=@Work Black' in saved(),'Theme was not saved'
 click(8200);set_text(116,'3157DB');set_text(8301,'Coding Blue');click(8302)
 assert 'Name2=@Coding Blue' in saved(),'Second theme was not saved'
 def select_theme(row):
  u.SendMessageW(item(8300),0x14e,row,0)
  u.SendMessageW(window,0x111,8300|(1<<16),item(8300))
 select_theme(1)
 assert text(114)=='234567' and not u.GetDlgItem(window,116),'Loading Solid displayed incorrect controls'
 click(9002)

 tab(1);set_text(207,'99999');click(9002)
 assert 'hoverDelay=99999' not in saved(),'Invalid hover delay was saved'
 set_text(207,140);set_text(208,350);set_text(205,260);set_text(206,10);click(9002)
 assert 'hoverDelay=140' in saved() and 'hoverWidth=260' in saved()
 tab(2);assert u.SendMessageW(item(301),0x146,0,0)==8,'Starting section still includes Quick Controls'
 u.SendMessageW(item(301),0x14e,7,0);set_text(306,45);set_text(307,7);u.SendMessageW(item(308),0xf1,1,0);click(9002)
 assert 'focusMinutes=45' in saved() and 'automaticBreak=1' in saved()
 tab(3);set_text(401,'My browser');set_text(402,'https://example.com');click(9002)
 assert 'My browser' in saved() and 'https://example.com' in saved()
 tab(4);note='Quick note 世界\r\nSecond line \\ path';set_text(501,note);click(9002)
 assert '世界' in saved(),'Unicode text did not persist'
 close_app();launch()
 assert u.SendMessageW(main,0x8003,0,0)==10 and 'defaultTab=8' in saved(),'Workstations starting section did not persist'
 assert u.SendMessageW(item(103),0x400,0,0)==120,'Size did not restore'
 assert [text(i) for i in (114,115)]==['234567','FDF8F3'],'Custom colors did not restore'
 assert u.SendMessageW(item(8300),0x146,0,0)==3,'Saved themes did not survive restart'
 select_theme(2)
 assert text(116)=='3157DB' and not u.GetDlgItem(window,114),'Loading saved Glass did not restore tint'
 set_text(116,'FFB6D3');click(8302)
 select_theme(1)
 click(8200)
 assert text(116)=='FFB6D3','Switching material did not restore the saved inactive color'
 click(8201)
 tab(1);assert text(207)=='140' and text(205)=='260','Behavior did not restore'
 tab(2);assert text(306)=='45' and text(307)=='7','Focus settings did not restore'
 tab(3);assert text(401)=='My browser' and text(402)=='https://example.com','Launchers did not restore'
 tab(4);assert text(501).replace('\r\n','\n')==note.replace('\r\n','\n'),'Multiline note did not restore'
 set_text(501,'unsaved change');click(2);assert not u.FindWindowW('WinPixelSettings',None),'Cancel did not close settings'
 u.SendMessageW(main,0x803d,0,0);window=wait_window('WinPixelSettings');tab(4)
 assert text(501).replace('\r\n','\n')==note.replace('\r\n','\n'),'Cancel changed the saved note'
 # Confirm the customized duration drives a real focus session.
 click(2);u.SendMessageW(main,0x803c,0,0);time.sleep(.4)
 for _ in range(2):u.SendMessageW(main,0x100,0x25,0)
 time.sleep(.3)
 u.GetDpiForWindow.argtypes=[w.HWND]
 scale=.55*1.2*u.GetDpiForWindow(main)/96
 point=(int(132*scale)<<16)|int(520*scale)
 u.SendMessageW(main,0x201,1,point);u.SendMessageW(main,0x202,0,point)
 assert u.SendMessageW(main,0x8051,0,0)==1,'Focus session did not start'
 remaining=u.SendMessageW(main,0x8052,0,0)
 assert 44*60000<remaining<=45*60000,'Custom focus duration was not used'
 result={'focusDurationUsed':True,'visibleSettings' :True,'appearanceApplied':True,'invalidInputRejected':True,'behaviorPersistence':True,'customFocusAndBreak':True,'launcherPersistence':True,'unicodeMultilineNote':True,'cancelDiscardsDraft':True,'profileIsolation':True}
 (root/'tests'/'settings-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
finally:
 if p and p.poll() is None:close_app()
