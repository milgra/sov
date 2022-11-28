# sov — Sway Overview

An overlay that shows schemas for all workspaces to make navigation in sway easier.

Sway Overview was made for SwayOS (https://swayos.github.io)

![alt text](screenshot.png)

## Features ##

- no downscaled and confusing thumbnails, just crystal clear app names and titles
- layout schema makes workspace identification easier
- super lightweight   
- multi-display support

## Installation

### Compiling from source

```
git clone git@github.com:milgra/sov.git
cd sov
meson setup build --buildtype=release
ninja -C build
sudo ninja -C build install
```

### From packages

[![Packaging status](https://repology.org/badge/tiny-repos/sov.svg)](https://repology.org/project/sov/versions)

## Usage

Launch sov in a terminal, enter 0 to hide, 1 to show, 2 to quit sov, press return.

```
sov
```

### Usage with sway wm

Launch sov in the config connected to a named pipe, but remove the named pipe first to avoid mkfifo errors. Add parameters if you want to tune some behaviour.

```
exec rm -f /tmp/sovpipe && mkfifo /tmp/sovpipe && tail -f /tmp/sovpipe | sov -t 500
```

Set up your sway config so that on workspace button press and release it writes 1 and 0 into the pipe:

```
bindsym --no-repeat $mod+1 workspace number 1; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+2 workspace number 2; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+3 workspace number 3; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+4 workspace number 4; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+5 workspace number 5; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+6 workspace number 6; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+7 workspace number 7; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+8 workspace number 8; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+9 workspace number 9; exec "echo 1 > /tmp/sovpipe"
bindsym --no-repeat $mod+0 workspace number 10; exec "echo 1 > /tmp/sovpipe"

bindsym --release $mod+1 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+2 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+3 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+4 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+5 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+6 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+7 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+8 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+9 exec "echo 0 > /tmp/sovpipe"
bindsym --release $mod+0 exec "echo 0 > /tmp/sovpipe"
```

### Usage with Systemd

Add this line to your config file:

```
exec systemctl --user import-environment DISPLAY WAYLAND_DISPLAY SWAYSOCK
```

Copy systemd unit files (if not provided by your distribution package):

```
cp contrib/systemd/sov.{service,socket} ~/.local/share/systemd/user/
systemctl daemon-reload --user
```

Enable systemd sov socket:

```
systemctl enable --now --user sov.socket
```

## Configuration and Styling ##

If you want to style sov, copy /usr/share/sov/config to ~/.config/sov/config and edit the html and css files inside it.

on most linux distros :
```
mkdir -p ~/.config/sov
cp -r /usr/share/sov/config ~/.config/sov
```

on FreeBSD
```
mkdir -p ~/.config/sov
cp -r /usr/local/share/sov/config ~/.config/sov
```

Additional options can be set as command line arguments :

```
-c, --columns=[columns]               Number of thumbnail columns
-a, --anchor=[lrtb]                   Anchor window to screen edge in directions, use rt for right top
-m, --margin=[size]                   Margin from screen edge
-r, --ratio=[ratio]                   Thumbnail to screen ratio, positive integer
-t, --timeout=[millisecs]             Milliseconds to wait for showing up overlays, positive integer

-h, --help                            Show help message and quit.
-v                                    Increase verbosity of messages, defaults to errors and warnings only.
-s,                                   Location of html folder for styling.
```

## Contribution/Development ##

Feel free to push fixes/improvements.

Please follow these guidelines :

- use clang format before commiting/after file save
- use mt_core functions and containers and memory handling
- make sure that the app is leak free. if you run the dev build it automagically checks for leaks on exit on two levels (mt_memory and clang address sanitizer ) and prints leaks
- always run all tests before push
- test detach/attach new output

Creating a debug build :

```
CC=clang meson build --buildtype=debug -Db_sanitize=address -Db_lundef=false
ninja -C build
```

## Donate ##

paypal : [https://paypal.me/milgra](https://paypal.me/milgra)  
patreon : [https://www.patreon.com/milgra](https://www.patreon.com/milgra)  
bitcoin : 37cSZoyQckihNvy939AgwBNCiutUVN82du  

## License ##

MIT, see [LICENSE](/LICENSE).

## TODO/BUGS ##

fix test and release workflows  
