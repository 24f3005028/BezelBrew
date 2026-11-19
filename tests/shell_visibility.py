"""Exercise hidden overlay windows, preview launch, rename, and activation identity."""
import ctypes as c,subprocess,time,os,json
from pathlib import Path
from ctypes import wintypes as w
root=Path(__file__).resolve().parents[1];exe=root/'dist'/'BezelBrew.exe';profile=root/'tests'/'shell-profile.ini'
profile.write_text('[Appearance]\nAnimations=0\n',encoding='utf-8')
u=c.WinDLL('user32');u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetWindowLongPtrW.argtypes=[w.HWND,c.c_int];u.GetWindowLongPtrW.restype=c.c_ssize_t
u.GetWindow.argtypes=[w.HWND,w.UINT];u.GetWindow.restype=w.HWND
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int];u.IsWindowVisible.argtypes=[w.HWND]
assert not u.FindWindowW('WinPixelIsland',None),'Quit existing instance first'
p=None

def wait_for(predicate,message):
 for _ in range(100):
  result=predicate()
  if result:return result
  time.sleep(.05)
 raise AssertionError(message)

def title(h):
 text=c.create_unicode_buffer(256);u.GetWindowTextW(h,text,256);return text.value

def launch(*args):
 global p,h
 env=os.environ.copy();env['WINPIXEL_CONFIG']=str(profile)
 p=subprocess.Popen([str(exe),*args],env=env)
 h=wait_for(lambda:u.FindWindowW('WinPixelIsland',None),'Island missing')
 wait_for(lambda:u.IsWindowVisible(h),'Island not ready')

def hidden_windows():
 owner=u.GetWindow(h,4);assert owner,'Overlay should keep a hidden owner'
 assert not u.IsWindowVisible(owner),'Helper owner leaked onto desktop'
 for window in [h,owner]:
  style=u.GetWindowLongPtrW(window,-20)
  assert style&0x80,'Tool-window exclusion missing'
  assert not style&0x40000,'APPWINDOW forces overlay into switcher/taskbar'
  assert style&0x8000000,'Overlay should not take focus automatically'

def close():
 if p and p.poll() is None:u.PostMessageW(h,0x10,0,0);p.wait(timeout=10)
try:
 launch('--preview');hidden_windows();assert title(h)=='BezelBrew','Default brand was not applied';close()
 launch('--settings');hidden_windows()
 settings=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings missing')
 item=lambda i:u.GetDlgItem(settings,i)
 assert title(settings)=='BezelBrew Settings'
 text=c.create_unicode_buffer('BezelBrew Personal');u.SendMessageW(item(113),0xc,0,c.addressof(text));u.SendMessageW(settings,0x111,9002,0)
 assert title(h)=='BezelBrew Personal' and title(settings)=='BezelBrew Personal Settings','Rename not applied'
 hidden_windows();u.SendMessageW(settings,0x111,2,0)
 # Existing --toggle links must still find the renamed instance by class, not caption.
 for expected in [1,0,1]:
  subprocess.run([str(root/'dist'/'WinPixel.exe'),'--toggle'],check=True,timeout=5)
  assert bool(u.SendMessageW(h,0x8008,0,0))==bool(expected),'Rename broke launch toggling'
  hidden_windows()
 u.SendMessageW(h,0x312,3,0);assert not u.IsWindowVisible(h)
 u.SendMessageW(h,0x312,3,0);hidden_windows();close()
 launch();assert title(h)=='BezelBrew Personal','Rename did not persist';hidden_windows();close()
 result={'previewExcluded':True,'normalExcluded':True,'hiddenOwner':True,'displayRenameApplied':True,'renamePersists':True,'legacyShortcutStillTogglesRenamedInstance':True,'pauseResumeRemainsExcluded':True,'shellUiVisualCheck':False}
 (root/'tests'/'shell-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
finally:close()
