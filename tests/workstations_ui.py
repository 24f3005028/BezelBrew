"""Exercise installed apps, website shortcuts, saved Workstations, and Quick Open."""
import ctypes as c
from ctypes import wintypes as w
import os
from pathlib import Path
import subprocess
import time

root=Path(__file__).resolve().parents[1]
profile=root/'tests'/'workstation-profile.ini'
profile.write_text('[Appearance]\nAnimations=0\n',encoding='utf-8')
u=c.WinDLL('user32')
u.FindWindowW.argtypes=[w.LPCWSTR,w.LPCWSTR]
u.FindWindowW.restype=w.HWND
u.FindWindowExW.argtypes=[w.HWND,w.HWND,w.LPCWSTR,w.LPCWSTR]
u.FindWindowExW.restype=w.HWND
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
u.IsWindowVisible.argtypes=[w.HWND]
u.GetDlgItem.argtypes=[w.HWND,c.c_int]
u.GetDlgItem.restype=w.HWND
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.GetDpiForWindow.argtypes=[w.HWND]
u.SetProcessDpiAwarenessContext.argtypes=[c.c_void_p]
u.SetProcessDpiAwarenessContext(c.c_void_p(-4))
assert not u.FindWindowW('WinPixelIsland',None),'Quit BezelBrew before testing'

def wait_for(check,message):
    for _ in range(160):
        value=check()
        if value:
            return value
        time.sleep(.025)
    raise AssertionError(message)

def send(h,message,wp=0,lp=0):
    return u.SendMessageW(h,message,wp,lp)

def command(h,ident):
    send(h,0x111,ident)

def item(h,ident):
    return wait_for(lambda:u.GetDlgItem(h,ident),f'Missing control {ident}')

def set_text(h,ident,text):
    data=c.create_unicode_buffer(text)
    send(item(h,ident),0xC,0,c.addressof(data))

def text(h,ident):
    data=c.create_unicode_buffer(512)
    send(item(h,ident),0xD,len(data),c.addressof(data))
    return data.value

def owned_dialog(title):
    window=None
    while True:
        window=u.FindWindowExW(None,window,'#32770',title)
        if not window:
            return None
        pid=w.DWORD()
        u.GetWindowThreadProcessId(window,c.byref(pid))
        if pid.value==p.pid:
            return window

def dialog(title):
    return wait_for(lambda:owned_dialog(title),f'Missing dialog {title}')

def launch():
    process=subprocess.Popen([root/'dist'/'BezelBrew.exe','--settings'],env=dict(os.environ,WINPIXEL_CONFIG=str(profile)))
    main=wait_for(lambda:u.FindWindowW('WinPixelIsland',None),'Island missing')
    settings=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings missing')
    item(settings,8103)
    command(settings,8103)
    return process,main,settings

def quit_app(process,main):
    if process.poll() is None:
        u.PostMessageW(main,0x10,0,0)
        process.wait(timeout=15)

def choose_app(settings,ident):
    u.PostMessageW(settings,0x111,ident,0)
    picker=dialog('Choose an app')
    listing=item(picker,2301)
    count=wait_for(lambda:send(listing,0x18B),'Windows returned no installed apps')
    wait_for(lambda:u.IsWindowVisible(picker),'App picker did not become visible')
    set_text(picker,2302,'never-an-installed-app-bezelbrew-test')
    assert send(listing,0x18B)==0,'App search did not filter'
    assert not u.IsWindowVisible(listing) and u.IsWindowVisible(item(picker,2304)),'Empty search kept showing the app list'
    set_text(picker,2302,'')
    assert send(listing,0x18B)==count,'Clearing search did not restore apps'
    assert u.IsWindowVisible(listing) and not u.IsWindowVisible(item(picker,2304)),'Results kept showing the empty message'
    command(picker,1)
    wait_for(lambda:not owned_dialog('Choose an app'),'App picker did not close')

def island_click(main,x,y):
    scale=.55*u.GetDpiForWindow(main)/96
    point=(int(y*scale)<<16)|int(x*scale)
    send(main,0x201,1,point)
    u.PostMessageW(main,0x202,0,point)

def show_workstations(main,settings):
    command(settings,2)
    if not send(main,0x8008):
        send(main,0x803C)
    time.sleep(.15)
    island_click(main,418,27)
    wait_for(lambda:send(main,0x8003)==10,'Dedicated Workstations tab did not open')
    time.sleep(.1)

def editor():
    settings=wait_for(lambda:u.FindWindowW('WinPixelSettings',None),'Settings did not open')
    item(settings,444)
    assert send(main,0x8050)==9,'Editor did not open on the dedicated Settings page'
    return settings

def add_item(settings,choice):
    if not u.GetDlgItem(settings,choice):
        command(settings,443)
    item(settings,choice)

def pick_link(settings,control,title,value):
    u.PostMessageW(settings,0x111,control,0)
    picker=dialog(title)
    set_text(picker,2320,value)
    command(picker,1)
    wait_for(lambda:not owned_dialog(title),f'{title} did not accept the selection')

p,main,settings=launch()
try:
    command(settings,8700)
    choose_app(settings,430)
    assert text(settings,402).startswith('shell:AppsFolder\\'),'App picker did not store an installed app'
    command(settings,423)
    command(settings,420)
    command(settings,434)
    set_text(settings,402,'example.com/docs')
    command(settings,9002)
    assert 'https://example.com/docs' in profile.read_text(encoding='utf-16'),'Website shortcut did not normalize'
    assert text(settings,8109)=='Workstations' and not u.GetDlgItem(settings,441)
    show_workstations(main,settings)
    island_click(main,340,155)
    settings=editor()
    assert text(settings,444)=='Workstation 1','Empty plus did not create a draft'
    set_text(settings,444,'Office')
    assert not u.GetDlgItem(settings,9100) and not u.GetDlgItem(settings,9140),'Empty workstation showed unused slots'
    for _ in range(5):
        add_item(settings,470)
        choose_app(settings,470)
    for index in range(5):
        add_item(settings,471)
        pick_link(settings,471,'Website shortcut',f'example.com/{index}')
    add_item(settings,472)
    u.PostMessageW(settings,0x111,472,0)
    spotify=dialog('Choose Spotify music')
    assert text(spotify,2322)=='Open Spotify' and text(spotify,2323)=='Paste link'
    set_text(spotify,2320,'https://open.spotify.com/album/6rqhFgbbKwnb9MLmUQDhG6')
    command(spotify,1)
    assert 'track or playlist link' in text(spotify,2321),'An album was accepted as music'
    set_text(spotify,2320,'https://open.spotify.com/intl-en/playlist/6rqhFgbbKwnb9MLmUQDhG6?si=test')
    command(spotify,1)
    wait_for(lambda:not owned_dialog('Choose Spotify music'),'Playlist picker did not close')
    command(settings,9002)
    ini=profile.read_text(encoding='utf-16')
    assert all(value in ini for value in ('[Workstation1]','Name=Office','App1Target=shell:AppsFolder',
                                           'App5Target=shell:AppsFolder',
                                           'Tab1=https://example.com/0','Tab5=https://example.com/4',
                                           'SpotifyTrack=spotify:playlist:6rqhFgbbKwnb9MLmUQDhG6'))
    command(settings,441)
    set_text(settings,444,'Gaming')
    add_item(settings,471)
    pick_link(settings,471,'Website shortcut','https://example.net')
    command(settings,9002)
    quit_app(p,main)
    p,main,settings=launch()
    command(settings,8109)
    assert text(settings,444)=='Office' and text(settings,9144)=='example.com/4'
    assert 'Spotify playlist' in text(settings,456)
    pick_link(settings,456,'Choose Spotify music','spotify:track:6rqhFgbbKwnb9MLmUQDhG6')
    command(settings,9160)
    command(settings,9002)
    assert text(settings,9140)=='example.com/1' and not u.GetDlgItem(settings,9144)
    assert 'SpotifyTrack=spotify:track:6rqhFgbbKwnb9MLmUQDhG6' in profile.read_text(encoding='utf-16')
    show_workstations(main,settings)
    island_click(main,185,241)
    settings=editor()
    assert text(settings,444)=='Gaming','Edit did not select the corresponding workstation'
    command(settings,442)
    command(settings,9002)
    ini=profile.read_text(encoding='utf-16')
    assert '[Workstation2]' not in ini and 'Name=Office' in ini,'Removing Gaming changed the wrong workstation'
    quit_app(p,main)
    profile.write_text(ini.replace('SpotifyTrack=spotify:track:', 'SpotifyTrack='),encoding='utf-16')
    p,main,settings=launch()
    command(settings,8109)
    assert 'Spotify track' in text(settings,456),'Legacy Spotify track did not migrate'
    print('Workstation Add menu, five apps and websites, Spotify track and playlist picker, persistence, editing, and removal passed.')
finally:
    for title in ('Choose an app','Website shortcut','Choose Spotify music'):
        window=owned_dialog(title)
        if window:
            command(window,2)
    quit_app(p,main)
