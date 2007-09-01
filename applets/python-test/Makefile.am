test_DATA = python-test.py
testdir = $(libdir)/awn/applets/python_test

desktopdir = $(libdir)/awn/applets
desktop_DATA = \
	python-test.desktop
@INTLTOOL_DESKTOP_RULE@

$(desktop_DATA).in: $(desktop_DATA).in.in
	sed -e "s|\@LIBDIR\@|$(libdir)|g" $< > $@


EXTRA_DIST = \
	python-test.desktop.in.in

