#!/bin/bash

# Note that running this script requires having Awn already installed
HEADER_FILES="task-manager.h task-icon.h task-item.h task-window.h task-launcher.h task-defines.h"

/usr/local/lib/vala/gen-introspect --namespace=Task $HEADER_FILES `pkg-config --cflags awn` `pkg-config --cflags libwnck-1.0` -DWNCK_I_KNOW_THIS_IS_UNSTABLE ./.libs/taskmanager.so > task-manager.gi
vapigen --pkg awn --pkg libwnck-1.0 --metadata=task-manager.metadata --library task-manager task-manager.gi
