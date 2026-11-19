import ctypes as c,json
from ctypes import wintypes as w
u=c.WinDLL('user32',use_last_error=True)
u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p]
u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetWindowRect.argtypes=[w.HWND,c.POINTER(w.RECT)]
u.WindowFromPoint.argtypes=[w.POINT];u.WindowFromPoint.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
h=u.FindWindowW('WinPixelIsland',None);assert h,'WinPixel is not running'
assert not u.SendMessageW(h,0x8004,0,0),'WinPixel is intentionally paused'
assert not u.SendMessageW(h,0x8008,0,0),'Hide the island before testing its hover strip'
r=w.RECT();u.GetWindowRect(h,c.byref(r))
width,height=r.right-r.left,r.bottom-r.top
samples=[u.WindowFromPoint(w.POINT(r.left+int(width*f),r.top+y))==h for f in (.25,.5,.75) for y in range(height)]
assert all(samples),'The hover zone is transparent to pointer hit-testing'
print(json.dumps({'physicalZone':[width,height],'hitTestSamples':len(samples),'allPassed':all(samples)}))
