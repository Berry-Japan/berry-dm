# berry-dm: Display Manager for Berry Linux

A minimalistic display manager for console.

## Features

- Lightweight and minimalistic
- JPEG, PNG, Animated GIF, BMP support
- Theme support
- PAM support

## Installation

To run berry-dm

    $ berry-dm

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

## Configuration

See /etc/berry-dm.conf for example.

## Screenshot

![Screenshot](screen02.png)
![Screenshot](screen01.png)

