"""Exercise native geometry and animation without changing media, mic or clipboard."""
import ctypes as c, subprocess, time, json
from ctypes import wintypes as w
from pathlib import Path
u=c.WinDLL('user32',use_last_error=True)
u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p]
u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
u.GetDpiForWindow.argtypes=[w.HWND]
u.WindowFromPoint.argtypes=[w.POINT];u.WindowFromPoint.restype=w.HWND
u.FindWindowW.restype=w.HWND
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR]
u.SendMessageW.restype=c.c_ssize_t
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.GetWindowRect.argtypes=[w.HWND,c.POINTER(w.RECT)]
u.IsWindowVisible.argtypes=[w.HWND]
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
root=Path(__file__).resolve().parents[1]
assert not u.FindWindowW('WinPixelIsland',None), 'Quit WinPixel before running the test'
p=subprocess.Popen([str(root/'dist'/'WinPixel.exe'),'--test'])
h=None
try:
    for _ in range(100):
        h=u.FindWindowW('WinPixelIsland',None)
        if h: break
        time.sleep(.1)
    assert h, 'Window did not appear'
    def size():
        r=w.RECT();u.GetWindowRect(h,c.byref(r));return (r.right-r.left,r.bottom-r.top)
    def click(x,y):
        scale=(expanded[0]/1.025)/680
        point=(int(y*scale)<<16)|int(x*scale)
        u.SendMessageW(h,0x201,1,point);u.SendMessageW(h,0x202,0,point)
        time.sleep(.4)
    time.sleep(.3)
    collapsed=size()
    bounds=w.RECT();u.GetWindowRect(h,c.byref(bounds))
    for offset in range(collapsed[1]):
        assert u.WindowFromPoint(w.POINT((bounds.left+bounds.right)//2,bounds.top+offset))==h, 'Invisible hover target does not receive real pointer input'

    u.SendMessageW(h,0x200,0,(1<<16)|80)
    u.SendMessageW(h,0x113,5,0)
    u.SendMessageW(h,0x312,2,0)
    time.sleep(.65)
    assert size()[0]>collapsed[0], 'Top-edge hover did not open'
    u.SendMessageW(h,0x2A3,0,0)
    u.SendMessageW(h,0x100,0x1B,0)
    time.sleep(.4)
    u.SendMessageW(h,0x312,2,0) # expand and pin, uses the real animation
    time.sleep(.08); intermediate=size()
    time.sleep(.55); expanded=size()
    material_ms=u.SendMessageW(h,0x8006,0,0)
    assert expanded[0] <= 500*u.GetDpiForWindow(h)/96, 'Compact overlay is too wide at test scale'
    time.sleep(.8)
    if u.SendMessageW(h,0x8028,0,0):
        assert u.SendMessageW(h,0x802A,0,0), 'Active session has no displayed title'
    assert u.SendMessageW(h,0x802C,0,0), 'System volume endpoint was not read'
    assert u.SendMessageW(h,0x8007,0,0)==1, 'Optical material did not render'
    assert collapsed[0]<intermediate[0]<=expanded[0], (collapsed,intermediate,expanded)
    assert expanded[1]>collapsed[1]
    def assert_animations_settle(page):
        deadline=time.monotonic()+3
        while True:
            flags=u.SendMessageW(h,0x8002,0,0)
            if page==0 and u.SendMessageW(h,0x802F,0,0):
                flags &= ~8  # An overflowing media title needs the frame timer.
            if not flags:
                return
            assert time.monotonic()<deadline, f'Animation timer flags on page {page}: {flags}'
            time.sleep(.1)

    assert_animations_settle(0)
    for index,tab in enumerate((0,1,2,3,5,6,7,10)):
        click(141+index*54,26)
        assert u.SendMessageW(h,0x8003,0,0)==tab, f'Tab {tab} did not activate'
        if tab==3:
            click(620,88)
            sheet=u.FindWindowW('WinPixelSettings',None)
            assert sheet and u.SendMessageW(h,0x8050,0,0)==4,'Quick Note icon did not open Notes'
            u.SendMessageW(sheet,0x10,0,0)
        if tab==7:
            sheet=u.FindWindowW('WinPixelSettings',None);assert sheet,'Full settings window did not open'
            u.SendMessageW(sheet,0x10,0,0)
        assert_animations_settle(tab)
    click(561,30)
    assert u.SendMessageW(h,0x8003,0,0)==9, 'Connectors toolbar button did not activate'
    click(617,29)
    assert u.SendMessageW(h,0x8003,0,0)==8
    click(450,170) # Solid material
    assert u.SendMessageW(h,0x8007,0,0)==0, 'Solid kept the optical background'
    click(170,170) # Glass material
    assert u.SendMessageW(h,0x8007,0,0)==1, 'Switching back to Glass did not capture the backdrop'
    click(141,26)
    assert u.SendMessageW(h,0x8003,0,0)==0
    def point(x,y):
        scale=(expanded[0]/1.025)/680
        return (int(y*scale)<<16)|int(x*scale)
    def swipe(x1,x2,y=118,cancel=False):
        u.SendMessageW(h,0x201,1,point(x1,y))
        u.SendMessageW(h,0x200,1,point(x2,y))
        assert u.SendMessageW(h,0x8034,0,0)==1, 'Drag did not enter swipe state'
        if cancel:u.SendMessageW(h,0x1F,0,0)
        else:u.SendMessageW(h,0x202,0,point(x2,y))
        time.sleep(.4)
    swipe(500,300)
    assert u.SendMessageW(h,0x8003,0,0)==1, 'Left swipe did not advance'
    swipe(260,470)
    assert u.SendMessageW(h,0x8003,0,0)==0, 'Right swipe did not go back'
    swipe(500,470)
    assert u.SendMessageW(h,0x8003,0,0)==0, 'Short swipe did not snap back'
    swipe(500,300,cancel=True)
    assert u.SendMessageW(h,0x8003,0,0)==0, 'Canceled swipe changed the page'
    u.SendMessageW(h,0x20E,120<<16,0);time.sleep(.4)
    assert u.SendMessageW(h,0x8003,0,0)==1, 'Horizontal wheel did not advance'
    u.SendMessageW(h,0x20A,(120<<16)|4,0);time.sleep(.4)
    assert u.SendMessageW(h,0x8003,0,0)==0, 'Shift wheel did not go back'
    for _ in range(4):
        u.SendMessageW(h,0x20E,15<<16,0);time.sleep(.03)
    time.sleep(.4)
    assert u.SendMessageW(h,0x8003,0,0)==1, 'High-resolution touchpad deltas did not accumulate'
    u.SendMessageW(h,0x20A,(120<<16)|4,0);time.sleep(.4)
    # Seeking is its own drag target, not a page gesture; cancel before seeking.
    if u.SendMessageW(h,0x8028,0,0):
        u.SendMessageW(h,0x201,1,point(350,215))
        assert u.SendMessageW(h,0x8033,0,0)==0, 'Seek bar incorrectly started a page swipe'
        u.SendMessageW(h,0x1F,0,0)
    # Repeat reversals midway; verify the final geometry and that animations stop.
    for _ in range(4):
        u.SendMessageW(h,0x312,2,0);time.sleep(.05)
        u.SendMessageW(h,0x312,2,0);time.sleep(.55)
    u.SendMessageW(h,0x312,2,0);time.sleep(.6)
    assert size()==collapsed
    assert u.SendMessageW(h,0x8007,0,0)==0, 'Optical buffers were not released'
    assert u.SendMessageW(h,0x8002,0,0)==0
    u.SendMessageW(h,0x312,3,0)
    assert u.SendMessageW(h,0x8004,0,0)==1, 'Pause shortcut did not pause'
    assert not u.IsWindowVisible(h), 'Paused hover zone is still visible'
    u.SendMessageW(h,0x200,0,(1<<16)|80);time.sleep(.15)
    assert not u.IsWindowVisible(h), 'Hover activated while paused'
    u.SendMessageW(h,0x312,3,0)
    assert u.SendMessageW(h,0x8004,0,0)==0 and u.IsWindowVisible(h)
    # Exercise the automatic-resume path with a short diagnostic timeout.
    u.SendMessageW(h,0x312,3,0);u.SendMessageW(h,0x8005,0,0);time.sleep(.3)
    assert u.SendMessageW(h,0x8004,0,0)==0, 'Automatic resume failed'
    duplicate=subprocess.Popen([str(root/'dist'/'WinPixel.exe'),'--test']);duplicate.wait(timeout=5)
    assert duplicate.returncode==0
    ps=f"$p=Get-Process -Id {p.pid}; $a=$p.TotalProcessorTime.TotalMilliseconds; Start-Sleep -Seconds 10; $p.Refresh(); [pscustomobject]@{{WorkingSetMiB=[math]::Round($p.WorkingSet64/1MB,2); PrivateMiB=[math]::Round($p.PrivateMemorySize64/1MB,2); CpuMsOver10s=$p.TotalProcessorTime.TotalMilliseconds-$a}} | ConvertTo-Json -Compress"
    print('Idle final state:', u.SendMessageW(h,0x8008,0,0), u.SendMessageW(h,0x8002,0,0), flush=True)
    metrics=json.loads(subprocess.check_output(['powershell','-NoProfile','-Command',ps],text=True))
    result={'collapsedPixels':collapsed,'intermediatePixels':intermediate,'expandedPixels':expanded,'media':{'sessionAvailable':bool(u.SendMessageW(h,0x8028,0,0)),'playbackStatus':u.SendMessageW(h,0x8029,0,0),'titleAvailable':bool(u.SendMessageW(h,0x802A,0,0)),'artworkAvailable':bool(u.SendMessageW(h,0x802B,0,0))},'frameProfile':{'averageIntervalMs':u.SendMessageW(h,0x800A,0,0)/1000,'worstIntervalMs':u.SendMessageW(h,0x800B,0,0)/1000,'intervalCount':u.SendMessageW(h,0x800C,0,0),'worstForegroundPaintMs':u.SendMessageW(h,0x800D,0,0)/1000},'opticalMaterialRenderMs':material_ms,'idle':metrics,'checks':['launch','animated expansion','navigation without Quick Controls','left/right swipes','short swipe snap-back','canceled swipe','horizontal wheel','precision wheel accumulation','shift wheel','seek gesture isolation','material picker','solid material','animation reversals','collapse','animation timer stopped','pause shortcut','hover disabled while paused','manual resume','automatic resume','single instance','clean exit']}
    u.PostMessageW(h,0x10,0,0);p.wait(timeout=5)
    assert p.returncode==0
    (root/'tests'/'result.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))
finally:
    if p.poll() is None:
        u.PostMessageW(h,0x10,0,0);p.wait(timeout=5)
