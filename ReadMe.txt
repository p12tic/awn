This repository contains the code of the Avant Window Navigator (Awn). Awn is a dock-like bar which sits at the bottom of your screen that allows you to launch and control applications. It additionally provides the ability to embed external applets. The appearance is entirely possible to constomize, and thematic support is available.

Installation:

	Please ascertain whether your distribution has either binary packages available for Awn, or ebuilds/recipes/compilative scripts available via your distribution's package-manager. It will save you a lot of time and energy if you install via your distribution's package manager.

	The following packages are necessary to compile Awn from its source-code. Know that to compile it via most distributions, installation of the library and development/header packages are necessary to correctly compile this software. These packages are:

	* GNU Make
	* libdesktop-agnostic [1]_
	* libwnck 2.22 or later
	* libX11
	* dbus-glib
	* xdamage
	* xcomposite
	* xrender
	* Python 2.5 or later
	* PyGTK 2.12 or later
	* pyxdg (also known as python-xdg)

	.. _[1]:: https://launchpad.net/libdesktop-agnostic

	To build from source, run:

		autoupdate
		autoconf
		./configure
		make
		make install

	"./configure" can take arguments, please invoke "./configure --help" for addition information. Invocation of "su -c 'make install'" may be necessary instead of "make install" because the default installation-location is "/usr/local".

Usage:

	You can launch Awn from "applications:/Accessories/Avant_Window_Navigator.desktop".
	Configuration of Awn is possible by invoking "awn-settings".

Help:

	If you have any questions, error-reports, and/or ideas, you are able to locate assistance within:

	 - https://github.com/p12tic/awn
	 - https://github.com/p12tic/awn-extras
	 - https://github.com/p12tic/libdesktop-agnostic

	https://launchpad.net/~awn-testing/+archive/ppa contains evaluative packages.
