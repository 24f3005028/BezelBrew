# Compact live-media build: performance and verification

Measured on this PC, 2026-11-15, optimized native Windows x64 executable.

| Metric | Result |
| --- | ---: |
| Expanded window at 100% display scaling, including reveal margin | 383 × 160 px |
| Hover strip at 100% display scaling | 88 × 2 px |
| Average application animation update interval | 16.306 ms |
| Worst sampled interval | 34.543 ms |
| Intervals sampled | 145 |
| Worst foreground paint | 9.312 ms |
| Optical material generation | 23 ms |
| Hidden working set after media and UI use | 25.94 MiB |
| Private committed memory | 3.79 MiB |
| CPU time over 10 seconds hidden idle | 0 ms |
| Executable size | 107,008 bytes |

The approximately 375 × 156 logical-pixel content scales with Windows DPI.
Compared to the prior 680 × 284 content, it is 45% smaller in each dimension.
Fonts use separately chosen sizes so compactness does not simply shrink all
text into miniature lettering.

The native Windows media integration was checked against the real Spotify
session: available session, Playing (status 4), nonempty title, and decoded
artwork. The live UI was visually inspected for matching art, play/pause icon,
artist/title and timeline. Tests do not change the user's media playback.
Timeline updates use the provider's UTC timestamp while playing. Accuracy and
available metadata depend on what the source app publishes to Windows.

The worker refreshes on opening and once per second while the Music panel is
visible. Hidden workers wait without polling. Native WinRT system DLLs account
for additional resident/shared memory compared to the earlier UI-only build.
No working-set trims are forced. These readings are one local sample, not
peak-memory guarantees or measured display-present FPS.

The smoke test also covers compact navigation hit targets, theme selection,
optical allocation/release, reversal/collapse, stopped animation state,
manual/timed pause and resume, singleton behavior, and clean exit.

Optical glass samples on opening and when the foreground window changes while
the island is open. The sample is held in RAM and freed on hiding.
This is an original approximation, not Apple's proprietary compositor.

## Readability and control revision

Latest local smoke run: average transition interval 16.134 ms, worst 22.298 ms
over 145 intervals; material generation 19 ms; hidden working set 28.72 MiB,
private committed 4.04 MiB, and 0 ms CPU over 10 seconds hidden idle.
The run also confirms that the actual Windows output-volume endpoint was read.
The clock can continue while visible Music is playing/scrolling; a stopped
transition does not mean the visible waveform stops animating.

The media screen and actual volume thumb were checked live against Spotify.
Compact preview renders were inspected for Widgets, Controls, Settings and
Themes, including the bottom cards and captions. Native small-icon rendering
and text now occur on the real material rather than an opaque color key.
The offline music preview uses a clearly isolated sample-title fixture.

## Swipe navigation checks

Live mouse drags were visually checked in both directions. Smoke checks now
cover left/right swipes, short-drag snap-back, cancellation, horizontal wheel,
Shift-wheel, precision-wheel accumulation, and exclusion of the seek bar from
page gestures. No physical touchscreen test was performed. The top navigation
and glass background remain stationary while page content slides.

## Hover target regression, 2026-11-18

The old target failed real pointer hit tests despite a visible HWND and valid
window region. Increasing minimum opacity from 1 to 8 and widening/tallening
the resting target fixed it. A 125%-scale monitor passed 24 physical-coordinate
hit samples across its 275 × 8 target. Real pointer entry opened the panel.
The DPI-aware smoke run passed; application update intervals averaged 16.235 ms
with a 22.062 ms worst interval. Hidden CPU remained 0 ms over ten seconds.
