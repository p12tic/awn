#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006 (ita)

import os
import Params, intltool, gnome, misc
import Object

# the following two variables are used by the target "waf dist"
VERSION='0.2.2'
APPNAME='avant-window-navigator'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def set_options(opt):
	# add your custom options here, for example:
	#opt.add_option('--bunny', type='string', help='how many bunnies we have', dest='bunny')
	pass

def configure(conf):
	conf.check_tool('gcc gnome cs intltool misc python')

	# This check and block is the library is not here, display the information and stock parameters in the destvar variable.
	#TODO Check good version
	if not conf.check_pkg('libgnome-2.0',        destvar='GNOME',    vnum='2.10.0', mandatory=True): Params.fatal('libgnome-2.0 headers needed')
	if not conf.check_pkg('gtk+-2.0',            destvar='GTK',      vnum='2.6.0', mandatory=True): Params.fatal('gtk+-2.0 headers needed')
	if not conf.check_pkg('glib-2.0',            destvar='GLIB',     vnum='2.8.0'): Params.fatal('glib-2.0 headers needed')
	if not conf.check_pkg('gobject-2.0',            destvar='GOBJECT',     vnum='2.0.0'): Params.fatal('gobject-2.0 headers needed')
	if not conf.check_pkg('gdk-x11-2.0',         destvar='GDK',      vnum='1.0.0'): Params.fatal('gdk-x11-2.0 headers needed')
	if not conf.check_pkg('libwnck-1.0',         destvar='LIBWNCK',      vnum='2.19.3'): Params.fatal('libwnck-1.0 headers needed')
	if not conf.check_pkg('gnome-vfs-2.0',         destvar='GNOMEVFS',      vnum='2.0.0'): Params.fatal('gnome-vfs-2.0 headers needed')
	if not conf.check_pkg('dbus-glib-1',         destvar='DBUSGLIB', vnum='0.60', mandatory=True): Params.fatal('dbus-glib-1 headers needed')
	if not conf.check_pkg('gnome-desktop-2.0',         destvar='GNOMEDESKTOP', vnum='2.0.0'): Params.fatal('gnome-desktop-2.0 headers needed')
	if not conf.check_pkg('libglade-2.0',        destvar='GLADE',    vnum='2.0.1'): Params.fatal('libglade-2.0 headers needed')
	if not conf.check_pkg('xproto',        destvar='XPROTO',    vnum='0.0.1'): Params.fatal('xproto headers needed')
	if not conf.check_pkg('xdamage',        destvar='XDAMAGE',    vnum='0.0.1'): Params.fatal('xdamage headers needed')
	if not conf.check_pkg('xcomposite',        destvar='XCOMPOSITE',    vnum='0.0.1'): Params.fatal('xcomposite headers needed')
	if not conf.check_pkg('xrender',        destvar='XRENDER',    vnum='0.0.1'): Params.fatal('xrender headers needed')
	
	
	#Use of options bunny = --options
	#if Params.g_options.bunny:
	#	something

	conf.define('VERSION', VERSION)
	conf.define('GETTEXT_PACKAGE', 'avant-window-navigator-0.2.2')
	conf.define('PACKAGE', 'avant-window-navigator')

	# Set define, like special directory
	conf.define('PKGDATADIR', conf.env['PREFIX']+'share/'+conf.env['PACKAGE'])
	conf.define('ACTIVE_DIR', conf.env['DATADIR']+'/active')
	conf.define('BIN_DIR', conf.env['PREFIX']+'/bin')

	#Set environnement variable (env)
	#conf.env['SOMETHING'] = something
	
	#Append values to a environnement variable
	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')
	conf.write_config_header('config.h')

def build(bld):
	env = bld.env()
	
	#Subdir to include
	bld.add_subdirs('po') #Must stay in subdir
	bld.add_subdirs('applets') #Test install
	bld.add_subdirs('awn-applet-activation') #Test install
	bld.add_subdirs('awn-manager') #Test install
	bld.add_subdirs('data') # Install inline stuff
	bld.add_subdirs('libawn')
	#bld.add_subdirs('pyawn') #TODO
	bld.add_subdirs('src')
	
	#Build libawn, a local library
	libawn = bld.create_obj('gnome', 'shlib')
	libawn.find_sources_in_dirs('libawn')
	libawn.includes='libawn'
	libawn.uselib='GTK GLIB GOBJECT GDK LIBWNCK GNOMEVFS DBUSGLIB'
	libawn.target='awn' #It'll rename it libawn
	libawn.install_subdir = 'LIBDIR'

	#Build the binary
	awn = bld.create_obj('gnome', 'program')
	awn.find_sources_in_dirs('src', excludes=['src/icon-test.c'])
	awn.includes='src'
	awn.uselib='GTK GLIB GOBJECT GDK LIBWNCK GNOMEVFS DBUSGLIB GNOMEDESKTOP GLADE XPROTO XDAMAGE XCOMPOSITE XRENDER GNOME'
	awn.uselib_local='awn'
	awn.target='avant-window-navigator'
	#TODO change target files for dbus (or modify headers)
	awn.add_dbus_file('src/awn-dbus.xml', 'awn_task_manager', 'glib-server',)
	awn.add_dbus_file('src/awn-applet-manager-dbus.xml', 'awn_applet_manager', 'glib-server')
	awn.add_marshal_file('src/awn-marshallers.list', '_awn_marshal', '--body')
	awn.add_marshal_file('src/awn-marshallers.list', '_awn_marshal', '--header')

	env = bld.env_of_name('default')

	#Example of use of env
	#if env['HAVE_GTKSHARP'] and env['MCS']:
	#	bld.add_subdirs('sharp')

	#if env['SGML2MAN']:
	#	bld.add_subdirs('man')

def shutdown():
	# maybe comments to say why they do ... i do not know i do not make gnome sw usually (ita)
	#gnome.postinstall_schemas('gnome-test')
	gnome.postinstall_icons()
	#gnome.postinstall_scrollkeeper('gnome-hello')

