"""Dark settings rendering, sidebar interactions, small-screen layout and cleanup."""
import ctypes as c,os,subprocess,time,json
from pathlib import Path
from ctypes import wintypes as w
root=Path(__file__).resolve().parents[1];profile=root/'tests'/'skin-profile.ini'
profile.write_text('[Appearance]\nTheme=3\nAnimations=1\n',encoding='utf-8')
u=c.WinDLL('user32');g=c.WinDLL('gdi32');k=c.WinDLL('kernel32')
u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p];u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.FindWindowExW.argtypes=[w.HWND,w.HWND,w.LPCWSTR,w.LPCWSTR];u.FindWindowExW.restype=w.HWND
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.IsWindowVisible.argtypes=[w.HWND];u.GetClientRect.argtypes=[w.HWND,c.POINTER(w.RECT)];u.GetWindowRect.argtypes=[w.HWND,c.POINTER(w.RECT)]
u.GetDpiForWindow.argtypes=[w.HWND]
u.GetDC.argtypes=[w.HWND];u.GetDC.restype=w.HDC;u.ReleaseDC.argtypes=[w.HWND,w.HDC]
u.PrintWindow.argtypes=[w.HWND,w.HDC,w.UINT]
u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int]
u.GetGuiResources.argtypes=[w.HANDLE,w.DWORD]
u.SetWindowPos.argtypes=[w.HWND,w.HWND,c.c_int,c.c_int,c.c_int,c.c_int,w.UINT]
g.CreateCompatibleDC.argtypes=[w.HDC];g.CreateCompatibleDC.restype=w.HDC
g.CreateCompatibleBitmap.argtypes=[w.HDC,c.c_int,c.c_int];g.CreateCompatibleBitmap.restype=w.HBITMAP
g.SelectObject.argtypes=[w.HDC,w.HGDIOBJ];g.SelectObject.restype=w.HGDIOBJ
g.GetPixel.argtypes=[w.HDC,c.c_int,c.c_int];g.GetPixel.restype=w.DWORD
g.DeleteObject.argtypes=[w.HGDIOBJ];g.DeleteDC.argtypes=[w.HDC]
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE;k.CloseHandle.argtypes=[w.HANDLE]
assert not u.FindWindowW('WinPixelIsland',None),'Quit BezelBrew before testing'
env=os.environ.copy();env['WINPIXEL_CONFIG']=str(profile);p=subprocess.Popen([str(root/'dist'/'BezelBrew.exe'),'--settings'],env=env)
main=window=None;process=None

def wait_for(predicate,label):
 for _ in range(100):
  result=predicate()
  if result:return result
  time.sleep(.05)
 raise AssertionError(label)

def item(i):
 h=u.GetDlgItem(window,i);assert h,f'Missing control {i}';return h

def click(i):u.SendMessageW(item(i),0xf5,0,0) # native BM_CLICK, including keyboard/accessibility behavior

def control_text(window,ident):
 h=u.GetDlgItem(window,ident);value=c.create_unicode_buffer(64)
 u.SendMessageW(h,0xD,len(value),c.addressof(value))
 return value.value

def open_picker(button=181,title='Accent color'):
 u.PostMessageW(window,0x111,button,0)
 def owned_picker():
  dialog=None
  while True:
   dialog=u.FindWindowExW(None,dialog,'#32770',title)
   if not dialog:return None
   pid=w.DWORD()
   u.GetWindowThreadProcessId(dialog,c.byref(pid))
   if pid.value==p.pid:return dialog
 dialog=wait_for(owned_picker,'Accent picker did not open')
 wait_for(lambda:u.GetDlgItem(dialog,2001),'Accent picker did not finish loading')
 wait_for(lambda:u.IsWindowVisible(dialog),'Accent picker did not become visible')
 return dialog

def bounds(h):
 r=w.RECT();u.GetWindowRect(h,c.byref(r));return r

def intersection(a,b):return max(a.left,b.left)<min(a.right,b.right) and max(a.top,b.top)<min(a.bottom,b.bottom)

def pixel(h,x,y):
 r=w.RECT();u.GetClientRect(h,c.byref(r));screen=u.GetDC(None);dc=g.CreateCompatibleDC(screen);bitmap=g.CreateCompatibleBitmap(screen,r.right,r.bottom);old=g.SelectObject(dc,bitmap)
 try:
  assert u.PrintWindow(h,dc,1),'Control did not render'
  color=g.GetPixel(dc,x,y);return color&255,(color>>8)&255,(color>>16)&255
 finally:g.SelectObject(dc,old);g.DeleteObject(bitmap);g.DeleteDC(dc);u.ReleaseDC(None,screen)
try:
 main=wait_for(lambda:u.FindWindowW('WinPixelIsland',None),'Main missing');window=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings missing')
 wait_for(lambda:u.IsWindowVisible(window),'Settings not visible');time.sleep(.25);scale=u.GetDpiForWindow(window)/96
 assert not u.IsWindowVisible(item(8001)),'Old tab frame should not cover the new form'
 assert all(u.IsWindowVisible(item(8100+i)) for i in range(9)),'Sidebar item missing'
 # Visible labels must not overlap any material card (the old empty-label regression).
 children=[]
 @c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
 def collect(h,_):
  if u.IsWindowVisible(h):children.append(h)
  return True
 u.EnumChildWindows(window,collect,0)
 u.GetClassNameW.argtypes=[w.HWND,w.LPWSTR,c.c_int]
 for h in children:
  cls=c.create_unicode_buffer(80);u.GetClassNameW(h,cls,80)
  if cls.value=='Static':
   for i in range(2):assert not intersection(bounds(h),bounds(item(8200+i))),'Label overlaps a material card'
 red,green,blue=pixel(item(103),int(30*scale),int(16*scale));assert blue>red+40,'Slider does not render the blue filled track'
 assert u.IsWindowVisible(item(9002)) and u.IsWindowVisible(item(113)),'Primary action or name input missing'
 assert not u.GetDlgItem(window,8202),'A legacy preset is still visible'
 click(8201);click(9002)
 assert 'glassMode=0' in profile.read_text(encoding='utf-16'),'Solid choice did not save'
 click(8200);click(9002)
 assert 'glassMode=1' in profile.read_text(encoding='utf-16'),'Glass choice did not save'
 initial=control_text(window,102)
 picker=open_picker()
 assert control_text(picker,2001)==initial,'Picker did not open on the current accent'
 plane=u.GetDlgItem(picker,2003)
 def point(x,y):return (int(y*scale)<<16)|int(x*scale)
 u.SendMessageW(plane,0x201,1,point(25,20))
 for x in range(30,300,5):
  u.SendMessageW(plane,0x200,1,point(x,95))
 dragged=control_text(picker,2001)
 assert dragged!=initial,'Dragging did not update the selected color'
 u.SendMessageW(plane,0x202,0,point(295,95))
 assert control_text(picker,2001)==dragged,'Releasing the drag changed the selected color'
 u.SendMessageW(picker,0x111,2012,0) # Mint preset
 assert control_text(picker,2001)=='49D5BB','Preset did not update the hex field'
 u.SendMessageW(picker,0x111,2,0)
 assert control_text(window,102)==initial,'Cancel changed the accent draft'
 picker=open_picker()
 hex_value=c.create_unicode_buffer('12AB34')
 u.SendMessageW(u.GetDlgItem(picker,2001),0xC,0,c.addressof(hex_value))
 u.SendMessageW(picker,0x111,1,0)
 assert control_text(window,102)=='12AB34','Use color did not update the accent draft'
 click(9002);assert 'accentRgb=1223476' in profile.read_text(encoding='utf-16'),'Accent did not save'
 picker=open_picker(184,'Glass tint color')
 u.SendMessageW(picker,0x111,2016,0) # Light pink
 u.SendMessageW(picker,0x111,1,0)
 assert control_text(window,116)=='FFB6D3','Tint picker did not update its own field'
 assert control_text(window,102)=='12AB34','Tint picker changed the accent'
 click(8101);assert u.SendMessageW(main,0x8050,0,0)==1,'Sidebar navigation failed'
 u.SendMessageW(item(8101),0x100,0x28,0);assert u.SendMessageW(main,0x8050,0,0)==2,'Sidebar arrow-key navigation failed'
 click(8100);base=u.SendMessageW(item(103),0x400,0,0);u.SendMessageW(item(103),0x100,0x27,0);u.SendMessageW(item(103),0x101,0x27,0)
 assert u.SendMessageW(item(103),0x400,0,0)>base,'Styled slider lost keyboard input'
 track=item(104)
 u.SendMessageW(track,0x405,1,107)
 bounds104=w.RECT();u.GetClientRect(track,c.byref(bounds104))
 mid=bounds104.right*22//45
 y=(bounds104.bottom//2)<<16
 u.SendMessageW(track,0x201,1,y|mid)
 end=bounds104.right*4//5
 for x in range(mid,end+1,3):u.SendMessageW(track,0x200,1,y|x)
 u.SendMessageW(track,0x202,0,y|end)
 dragged=u.SendMessageW(track,0x400,0,0)
 assert dragged>107 and control_text(window,1104)==f'{dragged}%','Slider drag did not update its value and readout'
 original=bounds(window);client=w.RECT();u.GetClientRect(window,c.byref(client));nonclient=(original.bottom-original.top)-client.bottom
 u.SetWindowPos(window,None,0,0,original.right-original.left,int(630*scale)+nonclient,0x16);time.sleep(.1)
 assert not u.IsWindowVisible(item(109)),'Short form should clip overflow above footer'
 for _ in range(3):u.SendMessageW(window,0x20a,((-120)&0xffff)<<16,0)
 assert u.IsWindowVisible(item(109)),'Short form does not scroll to the last setting'
 assert not intersection(bounds(item(109)),bounds(item(8002))),'Scrolled setting overlaps footer'
 u.SetWindowPos(window,None,0,0,original.right-original.left,original.bottom-original.top,0x16)
 assert u.IsWindowVisible(item(8200)) and u.IsWindowVisible(item(109)),'Restoring height did not restore the full form'
 process=k.OpenProcess(0x400,False,p.pid);assert process
 # Warm up close/reopen, then check bounded GDI lifetime across repeated use.
 u.SendMessageW(window,0x111,2,0);time.sleep(.1);before=u.GetGuiResources(process,0)
 for _ in range(6):
  u.SendMessageW(main,0x803d,0,0);window=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Reopen failed');u.SendMessageW(window,0x111,2,0)
 after=u.GetGuiResources(process,0);assert after<=before+6,f'Settings GDI leak: {before} -> {after}'
 result={'blueSliderRendered':True,'materialCardsUnobstructed':True,'sidebarClickAndKeyboard':True,'sliderKeyboard':True,'materialCardPersistence':True,'smallScreenScrolling':True,'footerDoesNotOverlap':True,'closedGdiObjectsBefore':before,'closedGdiObjectsAfterSixReopens':after}
 (root/'tests'/'skin-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
finally:
 if process:k.CloseHandle(process)
 if p.poll() is None:u.PostMessageW(main,0x10,0,0);p.wait(timeout=10)
