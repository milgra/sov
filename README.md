crash on empty workspace
check wob's package build
default anchor and margin from config

# how to create debug build

CC=clang meson build --buildtype=debug -Db_sanitize=address -Db_lundef=false

# sov — Sway Overview

[![Build Status](https://github.com/milgra/sov/workflows/test/badge.svg)](https://github.com/milgra/sov/actions)

![preview](https://github.com/milgra/sov/sov.png)

sway-overview is an application that shows thumbnails for all workspaces to make navigation in sway easier.

![alt text](screenshot2.png)
![alt text](screenshot1.png)

## Installation

### Compiling from source

Install dependencies:

- wayland
- wayland-protocols \*
- meson \*
- [scdoc](https://git.sr.ht/~sircmpwn/scdoc) (optional: man page) \*

\* _compile-time dependecy_

Run these commands:

```
git clone git@github.com:francma/wob.git
cd wob
meson build
ninja -C build
sudo ninja -C build install
```

### From packages

[![Packaging status](https://repology.org/badge/tiny-repos/wob.svg)](https://repology.org/project/wob/versions)

## Usage

Launch wob in a terminal, enter a value (positive integer), press return.

```
wob
```

### General case

You may manage a bar for audio volume, backlight intensity, or whatever, using a named pipe. Create a named pipe, e.g. /tmp/wobpipe, on your filesystem using.

```
mkfifo /tmp/wobpipe
```

Connect the named pipe to the standard input of an wob instance.

```
tail -f /tmp/wobpipe | wob
```

Set up your environment so that after updating audio volume, backlight intensity, or whatever, to a new value like 43, it writes that value into the pipe:

```
echo 43 > /tmp/wobpipe
```

Adapt this use-case to your workflow (scripts, callbacks, or keybindings handled by the window manager).

See [man page](https://github.com/francma/wob/blob/master/wob.1.scd) for styling and positioning options.

### Sway WM example

Add these lines to your Sway config file:

```
set $WOBSOCK $XDG_RUNTIME_DIR/wob.sock
exec rm -f $WOBSOCK && mkfifo $WOBSOCK && tail -f $WOBSOCK | wob
```

Volume using alsa:

```
bindsym XF86AudioRaiseVolume exec amixer sset Master 5%+ | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
bindsym XF86AudioLowerVolume exec amixer sset Master 5%- | sed -En 's/.*\[([0-9]+)%\].*/\1/p' | head -1 > $WOBSOCK
bindsym XF86AudioMute exec amixer sset Master toggle | sed -En '/\[on\]/ s/.*\[([0-9]+)%\].*/\1/ p; /\[off\]/ s/.*/0/p' | head -1 > $WOBSOCK
```

Volume using pulse audio:

```
bindsym XF86AudioRaiseVolume exec pamixer -ui 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioLowerVolume exec pamixer -ud 2 && pamixer --get-volume > $WOBSOCK
bindsym XF86AudioMute exec pamixer --toggle-mute && ( pamixer --get-mute && echo 0 > $WOBSOCK ) || pamixer --get-volume > $WOBSOCK
```

Brightness using [haikarainen/light](https://github.com/haikarainen/light):

```
bindsym XF86MonBrightnessUp exec light -A 5 && light -G | cut -d'.' -f1 > $WOBSOCK
bindsym XF86MonBrightnessDown exec light -U 5 && light -G | cut -d'.' -f1 > $WOBSOCK
```

Brightness using [brightnessctl](https://github.com/Hummer12007/brightnessctl):

```
bindsym XF86MonBrightnessDown exec brightnessctl set 5%- | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
bindsym XF86MonBrightnessUp exec brightnessctl set +5% | sed -En 's/.*\(([0-9]+)%\).*/\1/p' > $WOBSOCK
```

#### Systemd

Add this line to your config file:

```
exec systemctl --user import-environment DISPLAY WAYLAND_DISPLAY SWAYSOCK
```

Copy systemd unit files (if not provided by your distribution package):

```
cp contrib/systemd/wob.{service,socket} ~/.local/share/systemd/user/
systemctl daemon-reload --user
```

Enable systemd wob socket:

```
systemctl enable --now --user wob.socket
```

## License

ISC, see [LICENSE](/LICENSE).



bindsym --no-repeat $mod+1 workspace number 1; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+2 workspace number 2; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+3 workspace number 3; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+4 workspace number 4; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+5 workspace number 5; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+6 workspace number 6; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+7 workspace number 7; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+8 workspace number 8; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+9 workspace number 9; exec "echo '1' > /tmp/sway-overview"
bindsym --no-repeat $mod+0 workspace number 10; exec "echo '1' > /tmp/sway-overview"

bindsym --release $mod+1 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+2 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+3 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+4 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+5 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+6 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+7 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+8 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+9 exec "echo '0' > /tmp/sway-overview"
bindsym --release $mod+0 exec "echo '0' > /tmp/sway-overview"
