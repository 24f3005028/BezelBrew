import ctypes as c,json
from ctypes import wintypes as w
u=c.WinDLL('user32');u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND;u.IsWindowVisible.argtypes=[w.HWND];u.GetWindowRect.argtypes=[w.HWND,c.POINTER(w.RECT)];u.GetWindowLongW.argtypes=[w.HWND,c.c_int]
for name in ['WinPixelIsland','WinPixelSettings']:
 h=u.FindWindowW(name,None);r=w.RECT();u.GetWindowRect(h,c.byref(r));print(name,h,bool(u.IsWindowVisible(h)),[r.left,r.top,r.right,r.bottom],hex(u.GetWindowLongW(h,-16)&0xffffffff))
