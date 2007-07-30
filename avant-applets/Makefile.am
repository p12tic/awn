glade_DATA = avant-applets.glade
gladedir = $(pkgdatadir)


install-exec-local:
	sed -e "s|\@PKGDATADIR\@|$(pkgdatadir)|g" avant-applets.in.py > avant-applets.py
	$(INSTALL) -m 755 avant-applets.py $(DESTDIR)$(bindir)/avant-applets

EXTRA_DIST = $(glade_DATA) avant-applets.in.py

clean:
	rm -f *.pyo *.pyc *.bak *.gladep 
