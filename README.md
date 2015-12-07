# berry-dm

Minimalistic display manager for console.

## Installation

``
gcc -o berry-dm -Os berry-dm.c login.c -lcursesw -lform -lpam -lpam_misc
cp berry-dm.pam /etc/pam.d/berry-dm
gcc -std=c99 -Os -o berry-getty berry-getty.c
``

## Configuration

