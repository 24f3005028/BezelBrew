"""Verify command-line launch toggling and relaunch without another resident helper."""
import ctypes as c,subprocess,time,json
from ctypes import wintypes as w
from pathlib import Path
u=c.WinDLL('user32');u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
exe=Path(__file__).resolve().parents[1]/'dist'/'BezelBrew.exe'
def window():return u.FindWindowW('WinPixelIsland',None)
def expanded(h):return bool(u.SendMessageW(h,0x8008,0,0))
def launch():
    subprocess.Popen([str(exe),'--toggle']).wait(timeout=5)
    time.sleep(.45)
h=window();assert h,'The Windows shortcut did not launch the app'
assert expanded(h),'The Windows shortcut launched the app without opening it'
launch();assert window()==h and not expanded(h),'Running instance did not hide'
launch();assert window()==h and expanded(h),'Running instance did not open'
u.SendMessageW(h,0x312,3,0);assert u.SendMessageW(h,0x8004,0,0)==1
launch();assert expanded(h) and u.SendMessageW(h,0x8004,0,0)==0,'Shortcut failed to resume paused island'
u.PostMessageW(h,0x10,0,0)
for _ in range(50):
    if not window():break
    time.sleep(.1)
assert not window()
p=subprocess.Popen([str(exe),'--toggle'])
for _ in range(80):
    h=window()
    if h and expanded(h):break
    time.sleep(.1)
assert h and expanded(h),'Relaunch did not open the island'
result={'windowsShortcutLaunchedFromQuit':True,'toggleExistingInstance':True,'resumePaused':True,'relaunchPinnedOpen':True}
print(json.dumps(result,indent=2))
Path(__file__).with_name('activation-result.json').write_text(json.dumps(result,indent=2))
# Leave running in the default hover-only state.
u.SendMessageW(h,0x803c,0,0)
