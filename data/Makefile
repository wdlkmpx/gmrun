include ../config.mk

all:

strip:

install:
	mkdir -p $(DESTDIR)$(datadir)/applications
	install -c -m 644 gmrun.desktop $(DESTDIR)$(datadir)/applications
	mkdir -p $(DESTDIR)$(datadir)/pixmaps
	install -c -m 644 gmrun.png $(DESTDIR)$(datadir)/pixmaps
	mkdir -p $(DESTDIR)$(mandir)/man1
	install -c -m 644 gmrun.1 $(DESTDIR)$(mandir)/man1
	mkdir -p $(DESTDIR)$(sysconfdir)
	install -c -m 644 gmrunrc $(DESTDIR)$(sysconfdir)

uninstall:
	rm -f $(DESTDIR)$(datadir)/applications/gmrun.desktop
	rm -f $(DESTDIR)$(datadir)/pixmaps/gmrun.png
	rm -f $(DESTDIR)$(mandir)/man1/gmrun.1
	rm -f $(DESTDIR)$(sysconfdir)/gmrunrc

install-strip: install

clean:

distclean:


