# sov — Sway Overview

An overlay that shows schemas for all workspaces to make navigation in sway easier.

Sway Overview was made for SwayOS (https://swayos.github.io)

[![alt text](screenshot.png)](https://www.youtube.com/watch?v=0MP9pAxpu8E)

Watch the introduction/user guide [video on youtube](https://www.youtube.com/watch?v=0MP9pAxpu8E)

## Features ##

- no downscaled and confusing thumbnails, just crystal clear app names and titles
- layout schema makes workspace identification easier
- super lightweight   
- multi-display support

## Long descripton ##

Sway overview is an overview applciation for the sway tiling window manager. Tiling window managers are about simplicity so by default they don't have any kind of overview feature. It is okay under five workspaces because you can just remember where specific windows were opened but over 5 workspaces it can get really complicated.
Sway overview draws a schematic layout of all your workspaces on each output. It contains all windows, window titles and window contents.
Thumbnail based overview solutions can become confusing quickly when the thumbnails become too small to recognize, sway overview won't.
The common usage of Sway overview is to bound its appereance to the desktop switcher button with a little latency.
Sway overview can be structured via html, styled via css.

## Installation

### Compiling from source

Install the needed dependencies and libraries:

```
git meson ninja-build pkg-config
libpng,libfreetype,
libgl,libglew,libegl,libwegl,wayland-client,wayland-protocols,xkbcommon,gles2
fonts-terminus
````

On debian based systems ( over version 12 ):
```
sudo apt-get install git meson ninja-build pkg-config libpng-dev libfreetype-dev libgl-dev libegl-dev libglew-dev libwayland-dev libxkbcommon-dev wayland-protocols libgles2-mesa-dev

```

On arch based systems :
```
sudo pacman -Qs git meson pkg-config ninja glew wayland-protocols libpng freetype2 libgl libegl wayland wayland-protocols libxkbcommon 
```
or use the AUR

Then run these commands:

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

Launch sov in a terminal, enter 0 to hide, 1 to show, 2 to toggle, 3 to quit sov, press return.

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

By default it shows up on mod + workspace number and disappears when you release mod or workspace number.
If you want to make it stay while holding the mod key use the "holdkey" command line parameter with the keycode of the modkey ( 65515 for me ), you can listen for keycodes with the wev utility. 

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

If you want to style sov, copy /usr/share/sov/html to ~/.config/sov/html and edit the html and css files inside it.

on most linux distros :
```
mkdir -p ~/.config/sov
cp -r /usr/share/sov/html ~/.config/sov/
```

on FreeBSD
```
mkdir -p ~/.config/sov
cp -r /usr/local/share/sov/html ~/.config/sov/
```

Additional options can be set as command line arguments :

```
-c, --columns=[columns]               Number of thumbnail columns
-a, --anchor=[lrtb]                   Anchor window to screen edge in directions, use rt for right top
-m, --margin=[size]                   Margin from screen edge
-r, --ratio=[ratio]                   Thumbnail to screen ratio, positive integer
-t, --timeout=[millisecs]             Milliseconds to wait for showing up overlays, positive integer
-k, --holdkey=[keycode]               Keycode of hold key, sov won't disappear while pressed, get value with wev

-h, --help                            Show help message and quit.
-v                                    Increase verbosity of messages, defaults to errors and warnings only.
-s,                                   Location of html folder for styling.
-n,                                   Use workspace name instead of workspace number.
```

## Technical Info ##

SOV was written in Headerless C. It is about using the __INCLUDE_LEVEL__ preprocessor macro available in GCC/CLANG to guard the implementation part of a source file so you can place it together with the header definition in a single file and you include this file freely in other source files. Read more about this in (https://github.com/milgra/headerlessc);

SOV uses Kinetic UI as UI/Widget rendering engine. It is a modern, minimal ui renderer with html/css based stylimg written in headerless C. It offers MacOS like smooth kinetic animations and font-level animatioms, gpu or cpu based alpha blending, wayland connector with window and layer support and deterministic session record and replay for fully automated application testing.

SOV uses the mt_core library for retain counted memory management, retain counted and recursively observable vector and map containers and helper functions.

SOV uses meson and ninja for lightning fast compilation.

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

dynamically update when new monitor plugged in/out 
auto tests with 2x scaling 
