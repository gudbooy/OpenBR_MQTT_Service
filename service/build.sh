#bin/bash

gcc `pkg-config --libs --cflags dbus-1 glib-2.0 dbus-glib-1` listen.c -o listen `pkg-config --libs dbus-glib-1`
gcc `pkg-config --libs --cflags dbus-1 glib-2.0 dbus-glib-1` send.c -o send `pkg-config --libs dbus-glib-1`



