import ctypes as c,json
from ctypes import wintypes as w
u=c.WinDLL('user32',use_last_error=True);g=c.WinDLL('gdi32')
u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p];u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR];u.FindWindowW.restype=w.HWND
u.GetWindowRect.argtypes=[w.HWND,c.POINTER(w.RECT)];u.WindowFromPoint.argtypes=[w.POINT];u.WindowFromPoint.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.IsWindowVisible.argtypes=[w.HWND]
u.GetWindowRgn.argtypes=[w.HWND,w.HRGN];g.CreateRectRgn.restype=w.HRGN;g.PtInRegion.argtypes=[w.HRGN,c.c_int,c.c_int];g.GetRgnBox.argtypes=[w.HRGN,c.POINTER(w.RECT)];g.DeleteObject.argtypes=[w.HGDIOBJ]
h=u.FindWindowW('WinPixelIsland',None);r=w.RECT();u.GetWindowRect(h,c.byref(r));region=g.CreateRectRgn(0,0,0,0);kind=u.GetWindowRgn(h,region);box=w.RECT();g.GetRgnBox(region,c.byref(box))
print(json.dumps({'window':h,'rect':[r.left,r.top,r.right,r.bottom],'visible':bool(u.IsWindowVisible(h)),'paused':u.SendMessageW(h,0x8004,0,0),'expanded':u.SendMessageW(h,0x8008,0,0),'regionKind':kind,'regionBox':[box.left,box.top,box.right,box.bottom],'insideCenter':g.PtInRegion(region,(r.right-r.left)//2,1),'hits':[u.WindowFromPoint(w.POINT((r.left+r.right)//2,r.top+i))==h for i in range(max(0,r.bottom-r.top))]}))
g.DeleteObject(region)
