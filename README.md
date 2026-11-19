# BezelBrew

BezelBrew is the new name for this desktop island. Build with `./build.ps1` and
launch `dist/BezelBrew.exe`. `dist/WinPixel.exe` remains an identical compatibility
entry point for older links. Existing `WinPixel.ini` settings and stored account
credentials are preserved.

The island and its hidden helper use tool-window/no-activate styles in every
launch mode, including `--preview`, and explicitly remove any shell taskbar tab.
They stay out of Alt+Tab and the taskbar while remaining available by hover,
tray menu, and Ctrl+Alt+N. The separate Settings window remains a normal window
while it is open.

You can change the displayed name under Settings > Appearance > Display name.
Renaming no longer affects single-instance activation, which uses a stable
internal window class. `install-startup.ps1` migrates this project's owned
WinPixel shortcuts to BezelBrew and preserves Ctrl+Alt+N and start at sign-in.

`tests/shell_visibility.py` checks real overlay/owner styles in normal and preview
launches, persisted Unicode-capable display names, legacy launch compatibility,
and pause/resume. It does not inspect the shell switcher visually.

---

# WinPixel

A free, MIT-licensed, native Windows 10/11 x64 utility. C++/Win32 and the
Windows GDI+ renderer; no webview, .NET runtime, account, telemetry or server.

## Run

Download the Windows x64 ZIP from the GitHub release, extract it to a folder you
can write to, and run `BezelBrew.exe`. For a local build, run
`dist/BezelBrew.exe`. BezelBrew saves `WinPixel.ini` beside the executable.
Quit any running instance from its tray menu before switching builds.

**Rest your pointer at the very top-center edge of your display.** The island
stays invisible until you hover. It expands in
about 300 ms and disappears when you move away (180 ms grace, 220 ms close).
A roughly 220-by-7-pixel, nearly invisible strip at the top center detects hover
at 100% display scaling. Its mouse-hit opacity is kept above zero.

- The island toolbar opens Music, Shelf, Clipboard, Widgets, Launcher, Focus,
  Settings, Workstations, Connectors, and Appearance. Hover over an icon to
  see its name. Use the droplet icon to switch between Glass and Solid.
  Settings > Appearance lets you customize the text color and the active
  material's background or tint.
- Appearance settings: smooth animations, top-edge notch shape, or floating
  rounded shape. Keep open is optional and off by default.
- In Settings > Appearance, name the current look and select **Save theme**.
  Choose a saved theme and select **Save changes** to equip it. Use **Export**
  and **Import** to share a `.bbtheme` file containing appearance settings.
- Right-click the open island or system-tray icon for clear/quit actions.
- **Win+Alt+N pauses for 30 minutes; press again to resume immediately.**
  The tray menu also offers 5, 15, 30, or 60 minutes. Pausing hides the hover
  zone and suspends clipboard capture and active focus-session countdowns.
- Ctrl+Alt+N opens/hides the island and launches it after Quit (requires the launch shortcut below).
- Animation and theme choices are saved in `dist/WinPixel.ini`. Per-user startup and the launch hotkey can be installed with `install-startup.ps1`.

## Available tools

**Launchers and Workstations.** In Settings > Launchers, choose an installed app,
enter a website, or browse to an existing Windows or Chrome shortcut. You can
save up to six Workstations in Settings > Workstations, each with four apps or shortcuts, four web
addresses, and an optional Spotify track. Run, add, and edit them from the island’s
Workstations tab. The tray also has a Quick Open menu item. Web addresses open together in a
new Chrome window when Chrome is installed, or in the default browser otherwise.

Spotify track links open in Spotify by default. To enable automatic playback
in a publisher build, register `http://127.0.0.1/spotify/callback` in the Spotify
app dashboard and build with `./build.ps1 -SpotifyClientId <public-client-id>`.
Allowed Spotify Premium users can then connect in the Workstations editor.
Playback targets their active Spotify device. Refresh tokens are stored per
BezelBrew profile in Windows Credential Manager.

**Music:** reads the actual current Windows media session: title, artist,
playback state, position/duration, and available album artwork. The play/pause
icon follows the reported state, and previous/next/play-pause target that same
session. The worker refreshes once per second while the music panel is open,
and on opening. It sleeps while hidden. Artwork is bounded to 1 MiB encoded,
reduced to a 128-pixel image, and kept only in memory. Apps must publish their
session to Windows; unsupported apps show "No active player". Clicking the timeline seeks within the current track. Live lyrics are not implemented.

**Shelf:** drop up to 12 files; click one to reveal its original location in
Explorer. No copying/moving/deleting the original. Scroll for more rows.

**Clipboard:** up to 12 recent text entries, limited to 2,047 characters each.
Click to copy; scroll for more; clear with the trash button or context menu.
Captures text while running, including potentially sensitive copied text.
Memory only; history is never saved to disk.

**Widgets:** local clock, CPU utilization, and physical RAM utilization.
**Launcher:** Explorer, Windows Settings, Calculator, Notepad.
**Focus:** a 25-minute session with a Windows completion notification.

## Lightweight by design

The animation timer exists only during transitions. It starts once and stops
when the transition settles; pointer movement does not restart frame timing.
The drawing buffer is released after hiding. Idle clipboard monitoring is
event-driven. Clock/system readings update once per second only while open.
There are no forced working-set trims or continuous background animations.

The current UI measured **28.72 MiB resident RAM**, **4.04 MiB private committed
memory**, and **0 ms CPU during ten seconds collapsed idle** on this PC. The
native executable is 107,008 bytes. See `PERFORMANCE.md` for scope and limitations.

## Build and verify

The island is now about 375 × 156 device-independent pixels (plus a small
animation margin), with font sizes chosen separately to remain legible.

Requires portable Zig 0.15.2 (compiler only; not needed to run the application).

```powershell
python -m pip install ziglang==0.15.2 --target tools
.\build.ps1
```

A compatible external Zig can be passed with `build.ps1 -Zig <path>`.
Quit WinPixel before replacing the executable or running the smoke test.

```powershell
python tests/smoke.py
.\dist\WinPixel.exe --render
```

`--render` writes deterministic screenshots of the actual native paint path to
`tests/previews`. `--preview` opens the island pinned for visual QA; ordinary
launches stay hidden until hover. `--test` ignores saved appearance settings.

## Scope and attribution

The UI uses the familiar iOS Dynamic Island form, icon navigation, rounded
surfaces, and a black media card. Public NotchPixel screenshots were consulted
as design references: https://notchpixel.com/press. Reference screenshots are
not bundled into the app. WinPixel is independent, with its own name and vectors;
it is not an Apple or NotchPixel product or a complete pixel-for-pixel replica.

Phone transfers, phone remote, lyrics, OCR/PDF tools, per-app audio, notification
mirroring, GPU/temperature/FPS overlays, and embedded web pages are not included.
Keyboard navigation and screen-reader coverage need further work. Placement
follows the current window's monitor, without separate per-monitor islands.

MIT license; see `LICENSE`.

## Optical glass renderer

Glass uses the optical renderer in `src/glass.hpp`. On opening,
it samples only the island-sized region behind the UI into local memory,
blurs it with a separable multipass filter, and applies a rounded-lens edge
refraction field, subtle chromatic dispersion, inner shading, and Fresnel-style
specular highlights. The renderer blends in the selected tint and applies
contrast shading based on the selected text color. Cards and buttons let the
backdrop show through, while text and icons stay opaque.

**The background is a snapshot refreshed on each opening.** Changes behind the
panel are not continuously captured while it stays open. This bounds memory and
avoids idle capture/GPU activity. Samples are never written to disk or sent
anywhere, and all material buffers are freed on collapse, pause, or exit.

This is an independent approximation of optical glass, not Apple's proprietary
Liquid Glass compositor. The native transition uses a stable backing surface,
an animated reveal region, and short-lived high-resolution frame timing.

## Frame pacing fix

Glass and controls are now composed into one cached, premultiplied-alpha
window. Reveal masks animate the cached surface; the app no longer reshapes
two windows and redraws every vector for every reveal frame. A high-resolution
waitable clock posts coalesced frame messages during transitions. Its small
worker thread sleeps on kernel events and the clock is canceled while idle.
Scripted transitions averaged 16.488 ms between updates (about 60/s), with a
59.259 ms worst interval. This measures application updates, not guaranteed
display-present FPS. Optical material generation still costs roughly 56 ms
on the tested PC and occurs during the hover dwell.

## Native media implementation

`src/media.hpp` calls the Windows.Media.Control WinRT ABI directly, using the
interface definitions published by Microsoft in windows-rs. No managed runtime
or SDK package is needed. WinRT work and bounded asynchronous waits run on a
separate event-driven worker, not the rendering thread. Playback position is
extrapolated from the reported UTC update timestamp while playing and clamped
to the published duration.

API references:
- https://learn.microsoft.com/en-us/uwp/api/windows.media.control.globalsystemmediatransportcontrolssessionmanager
- https://learn.microsoft.com/en-us/uwp/api/windows.media.control.globalsystemmediatransportcontrolssessiontimelineproperties.position

## Readability and controls polish

The music layout gives the title/artist their own wide rows. Long titles scroll
after a pause instead of staying truncated. Played progress uses a smoothly
joined animated wave while playing and a still bar when paused, with elapsed
and remaining time labels. Position interpolates between native session updates.
The volume bar reads and changes the default Windows multimedia endpoint;
click or drag it to set the level. It shows the actual level and mute state.

The lighter glass uses one half-resolution blur pass. Text and vectors now draw
directly over the optical material, eliminating the old color-key fringes.
Vector coordinates retain subpixel precision; strokes have a minimum physical
pixel width. Larger navigation glyphs use native hover tooltips, with no cut-off
inline captions. Offline fixtures cover all panels at actual compact dimensions.
Music animation runs only while visible, playing, or scrolling a long title;
there is no animation clock activity while hidden.

## Swipe between sections

Drag the panel content left to advance, or right to go back. The page follows
the drag, then slides into place; short or canceled drags return to the
original page. Music, Shelf, Clipboard, Widgets, Launcher, Focus, Workstations and
Settings form a circular sequence. The toolbar remains fixed.

Horizontal touchpad scrolling and Shift + mouse wheel also change sections.
Small high-resolution wheel deltas accumulate instead of being dropped.
Ordinary vertical scrolling still scrolls file and clipboard entries. Native
Windows horizontal touch-pan handling is included; physical touchscreen
behavior has not been tested on this machine.

Start swipes in the content area. The song seek bar and volume slider retain
their own drag behavior. A drag across a button cancels its click action once
it becomes a page gesture. Escape/capture loss cancels a pending swipe.

## Start automatically and launch by keyboard

Run `.\install-startup.ps1` after building. This creates two shortcuts for your
Windows account only: a Startup shortcut that runs WinPixel hidden at sign-in,
and a Start Menu shortcut with Ctrl+Alt+N and the `--toggle` argument.
Explorer owns this hotkey, so it works even after choosing Quit in WinPixel.
The executable does not register that same combination, avoiding a conflict.

If WinPixel is already running, `--toggle` sends a bounded activation message
to the single existing instance. A new launch opens it pinned without needing
a hover. Running paused instances resume before opening. The Win+Alt+N
30-minute pause shortcut remains unchanged.

To remove both Windows shortcuts, run `.\install-startup.ps1 -Remove`.
Moving the project/executable requires rerunning the installer to update paths.
The installer refuses to replace an unrelated shortcut with the same name.
There is no service or extra resident launcher process.

Verified locally on 2026-11-17: real Ctrl+Alt+N keypress launched from Quit;
activation checks covered open/hide, paused resume and fresh pinned launch.
The startup shortcut target and arguments were read back after installation;
a Windows restart/sign-in was not performed as part of the test.

## Hover activation repair (2026-11-18)

The former two-physical-pixel strip used global alpha 1 in per-pixel composition,
which was not reliably mouse-hit-testable. The resting alpha is now 8 and the
activation area is wider/taller without enlarging the expanded island.
`tests/check_hover_zone.py` checks real WindowFromPoint hits across the target
using physical, per-monitor-DPI-aware coordinates. Smoke tests also verify the
hit target before sending synthetic events, so they catch this failure.
Real pointer entry into the strip was verified to open the island without a
click or shortcut. The test monitor at 125% scaling has a 275-by-8-physical-pixel
activation target.

## Mail and phone connectors

The envelope button opens a compact mail/phone feed. Settings has guided Gmail,
Android and iPhone pages. Connector actions save their necessary preferences;
users do not need an extra Apply step after Sign in or Connect this phone.

- Gmail: Sign in with Google using the release's publisher Desktop OAuth identity.
  A build without that identity explains that the publisher must configure it;
  it does not ask ordinary users to create developer accounts. See
  [publisher instructions](packaging/GOOGLE_SIGN_IN.md). Restricted-scope Google
  verification still applies before a public rollout. User refresh tokens use
  Windows Credential Manager. Personal client import remains under Advanced.
- Android: guided companion installation and pairing, detection of standard KDE
  Connect installations/running apps, asynchronous discovery, and a phone-name
  picker. Multiple devices require a choice; a single device is preselected.
  Connect saves the selected ID and enables sync. Disconnect clears the feed
  without deleting the KDE Connect pairing. Manual paths/IDs are Advanced only.
- iPhone: Bluetooth pairing, iPhone permission instructions and Check connection.
  The adapter is still a hardware-unverified beta. Phone Link is a clearly
  separate fallback; its private feed is not imported into BezelBrew.

See the [user setup guide](docs/CONNECTORS.txt). No accounts are connected merely
by opening a setup page. Discovery is explicit and does not add idle polling.
The Pause shortcut suspends connectors. Headers and notifications stay in memory.

Validation includes protocol/PKCE tests, isolated worker tests using synthetic
KDE devices, guided setup with multiple/empty/failing device lists, preservation
of Advanced options, and publisher-build input checks. Real account consent,
Google project approval and real-phone compatibility are not validated by fixtures.

## Settings design (2026-11-18)

The dedicated Settings window now matches the island's dark iOS styling: a
rounded sidebar, blue accent, vector icons, theme cards, a live theme/accent/
corner preview, dark inputs, custom sliders, and animated switches. Native
controls retain keyboard navigation, editable text and accessibility semantics.
The old tab frame is hidden, avoiding its overlap with labels and sliders.

The form scrolls when the window is shorter, keeping settings above its fixed
footer. Windows 11 uses a dark caption and rounded corners; the painted dark
surface also works when newer DWM material attributes are unavailable. No
WebView or continuous desktop capture is added. Per-control animation timers
stop after 160 ms. The settings drawing buffer and fonts are released on close.

`tests/settings_skin.py` covers native slider rendering, unobstructed theme
cards, sidebar click/arrow navigation, keyboard slider input, theme saving,
short-screen scrolling, footer separation and GDI cleanup across six reopens.
The existing settings and connector tests cover persistence and validation.
