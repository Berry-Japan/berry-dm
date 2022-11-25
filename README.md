# berry-dm: Display Manager for Berry Linux

A minimalistic display manager for console.

## Features

- Lightweight and minimalistic
- JPEG, PNG, Animated GIF, BMP support
- Theme support
- PAM support

## Installation

To make berry-dm

```Bash
$ clang -o berry-dm -Os berry-dm.c login.c ui.c 3rd/ini.c -lm -lpam -lpam_misc
# mv berry-dm /usr/local/sbin/
$ cp berry-dm.pam /etc/pam.d/berry-dm
$ cp berry-dm.conf /etc/
$ cp berry-logo.txt /etc/
$ clang -Os -o berry-getty berry-getty.c
# mv berry-getty /usr/local/sbin/
```

To run berry-dm

    $ berry-dm

## Configuration

See /etc/berry-dm.conf for example.

```berry-dm.conf
# berry-dm

[config]
gui = console		# console/glsl
sessions = LXDE,/etc/X11/berryos-xsession,Wayfire (Weston),wayfire,Console,bash,Reboot,reboot,Shutdown,shutdown -h now
users = berry,root
languages = Japanese,ja_JP.utf8,English,en_US.utf8,Chinese,zh_TW.utf8
#image = /etc/berry-logo.jpg
image = /etc/berry-logo.gif
#text = /etc/berry-logo.txt

F7 = stty echo
F8 = clear
F9 = mount /dev/sda7 /root
F10 = ntpd -q -p pool.ntp.org
F11 = reboot
F12 = shutdown -h now
statusbar = <F10:NTP> <F11:Reboot> <F12:Shutdown>
```

## Screenshot

![Screenshot](screen02.png)
![Screenshot](screen01.png)

