#!/bin/sh

valac -C -H awn-panel-connector.h awn-panel-connector.vala --pkg dbus-glib-1 --pkg gtk+-2.0 --pkg posix awn-defines.vapi --pkg desktop-agnostic --vapi awn-panel-connector.vapi
valac -C -H awn-applet.h awn-applet.vala --pkg dbus-glib-1 --pkg gtk+-2.0 --pkg posix awn-panel-connector.vapi awn-defines.vapi --pkg desktop-agnostic
sed -i -r 's/^#include <(awn.+h)>$/#include "\1"/' awn-applet.h awn-panel-connector.h

