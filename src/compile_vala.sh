#!/bin/sh

valac -C -H awn-panel-dispatcher.h awn-panel-dispatcher.vala --pkg dbus-glib-1 --pkg gtk+-2.0 awn-panel.vapi
#valac -C -H awn-applet.h awn-applet.vala --pkg dbus-glib-1 --pkg gtk+-2.0 --pkg x11 --pkg posix awn-panel-connector.vapi awn-defines.vapi --pkg desktop-agnostic
sed -i -r 's/^#include <(awn.+h)>$/#include "\1"/' awn-panel-dispatcher.[ch]

