glade_DATA = avant-launchers.glade
gladedir = $(pkgdatadir)


install-exec-local:
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" avant-launchers.in.py > avant-launchers.py
	$(INSTALL) -m 755 avant-launchers.py $(DESTDIR)$(bindir)/avant-launchers

EXTRA_DIST = $(glade_DATA) avant-launchers.in.py

clean:
	rm -f *.pyo *.pyc *.bak *.gladep 
