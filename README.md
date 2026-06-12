# wshowkeys

Keystroke and mouse-pointer overlay for **Wayland** (Hyprland), made for
teaching and presenting: keypresses are drawn in a large, high-contrast
overlay, and the mouse pointer can be highlighted with a bright blob so the
audience never loses it — even on busy, dark application UIs.

Wayland only. X11 is not supported. Requires a compositor with
`wlr_layer_shell_v1` (Hyprland, sway, etc.); the monitor-targeting,
focus-following, and pointer-highlight features use Hyprland IPC.

Fork lineage: https://git.sr.ht/~sircmpwn/wshowkeys →
https://github.com/DreamMaoMao/wshowkeys

## Features

- On-screen keypresses, with modifier combinations (`Ctrl+`, `Super+`, …)
  and a repeat counter for held keys
- `-p`: bright amber blob tracking the mouse pointer (Hyprland)
- `-o`: pin the overlay to one monitor — by Hyprland monitor ID (`0`, `1`,
  `2`, … as shown by `hyprctl monitors`) or by name (`DP-1`, `eDP-1`) — and
  show it **only while that monitor is focused**
- Toggle: running `wshowkeys` while another instance is active terminates
  it instead. Bind one key to turn it on and off.

## Install

Dependencies: `cairo` `libinput` `pango` `libudev` `wayland` `xkbcommon`
(build: `meson` `ninja`)

```sh
meson setup build
ninja -C build
sudo ninja -C build install
sudo chmod a+s "$(command -v wshowkeys)"
```

wshowkeys must be setuid: it needs root to read input events. Root is
dropped immediately after startup.

## Hyprland

Add one bind to `~/.config/hypr/hyprland.conf` — press it to enable,
press it again to disable:

```conf
# SUPER+F8: toggle keystroke + pointer overlay for presenting
bind = SUPER, F8, exec, wshowkeys -a bottom -m 80 -t 1500 -o 0 -p
```

This shows keys at the bottom of monitor 0, highlights the pointer, and
hides everything whenever another monitor has focus. The defaults are
tuned for visibility on dark editor/3D-viewport style screens; tweak
colors per the options below if needed.

## Usage

```
wshowkeys [-b|-f|-s #RRGGBB[AA]] [-F font] [-t timeout]
          [-a top|left|right|bottom] [-m margin] [-l lenmax]
          [-o output] [-p]
```

- `-b #RRGGBB[AA]`: background color (default `#000000D9`)
- `-f #RRGGBB[AA]`: foreground color (default `#FFFFFFFF`)
- `-s #RRGGBB[AA]`: color for special keys (default `#FFD000FF`)
- `-F font`: font (Pango format, default `Sans Bold 40`)
- `-t timeout`: time in ms before old keystrokes are cleared
- `-a top|left|right|bottom`: anchor edge; may be given twice
- `-m margin`: margin in pixels from the nearest edge
- `-l lenmax`: maximum displayed length of the key line
- `-o output`: monitor to show on — Hyprland ID (`hyprctl monitors`) or
  name. The overlay only appears while this monitor is focused.
- `-p`: highlight the mouse pointer with a tracking blob (Hyprland)
