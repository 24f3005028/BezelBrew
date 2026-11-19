import ctypes as c,json
from ctypes import wintypes as w
u=c.WinDLL('user32');d=c.WinDLL('dwmapi')
u.GetWindowLongPtrW.argtypes=[w.HWND,c.c_int];u.GetWindowLongPtrW.restype=c.c_ssize_t
u.GetWindow.argtypes=[w.HWND,w.UINT];u.GetWindow.restype=w.HWND
u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int];u.GetClassNameW.argtypes=[w.HWND,w.LPWSTR,c.c_int]
u.IsWindowVisible.argtypes=[w.HWND];u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
rows=[]
@c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
def collect(h,_):
 cls=c.create_unicode_buffer(128);u.GetClassNameW(h,cls,128)
 if cls.value.startswith('WinPixel'):
  title=c.create_unicode_buffer(256);u.GetWindowTextW(h,title,256)
  rows.append({'class':cls.value,'title':title.value,'visible':bool(u.IsWindowVisible(h)),'style':hex(u.GetWindowLongPtrW(h,-16)&0xffffffff),'exStyle':hex(u.GetWindowLongPtrW(h,-20)&0xffffffff),'owner':u.GetWindow(h,4)})
 return True
u.EnumWindows(collect,0);print(json.dumps(rows,indent=2))
