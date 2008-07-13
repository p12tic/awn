#!/usr/bin/env python

import gconf
import gtk
import re
import awn

client = awn.Config()

l = client.get_list(awn.CONFIG_DEFAULT_GROUP,'applets_list', awn.CONFIG_LIST_STRING)
l2 = []
c = len(l)
i=0
control = ('taskman.desktop', 
'places.desktop',
'notification-area.desktop',
'rtm.desktop',
'calendar.desktop',
'wobblyzini.desktop',
'shinyswitcher.desktop',
'awnsystemmonitor.desktop',
'awn-meebo.desktop',
'simple-launcher.desktop',
'plugger.desktop',
'trasher.desktop',
'weather.desktop',
'battery-applet.desktop',
'awnterm.desktop',
'filebrowser.desktop',
'cairo-clock.desktop',
'cairo_main_menu.desktop',
'to-do.desktop',
'cairo_main_menu_classic.desktop',
'awn-pandora.desktop',
'standalone-launcher.desktop',
'comic.desktop',
'digitalClock.desktop',
'vala-test.desktop',
'switcher.desktop',
'main-menu.desktop',
'media-icon-next.desktop',
'mimenu.desktop',
'comics.desktop',
'arss.desktop',
'lastfm.desktop',
'media-control.desktop',
'showdesktop.desktop',
'quit-applet.desktop',
'separator.desktop',
'python-test.desktop',
'awnnotificationdaemon.desktop',
'mount-applet.desktop',
'thinkhdaps.desktop',
'Taskmand-applet.desktop',
'stacks.desktop',
'tsclient-app.desktop',
'file-browser-launcher.desktop',
'mail.desktop',
'media-icon-play.desktop',
'trash.desktop',
'volume-control.desktop',
'webapplet.desktop',
'affinity.desktop',
'media-icon-back.desktop',
'tomboy-applet.desktop')

while c > i:
	if filter(lambda x: x+1,[l[i].find(elem) for elem in control]) <> []:
		l2.append(l[i].replace('/lib/','/share/'))
	else:
		l2.append(l[i])
	i=i+1
if l2 <> l:
	client.set_list(awn.CONFIG_DEFAULT_GROUP,"applets_list", awn.CONFIG_LIST_STRING, l2)
	print l2
