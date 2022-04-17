
include ../config.mk

USE_NLS ?= no
LINGUAS ?= 
GETTEXT_PACKAGE ?= 

all: all-$(USE_NLS)
all-no:

all-yes:
	@sh zzpo.sh mo

install: install-data-$(USE_NLS)
install-data-no: all
install-data-yes:
	@sh zzpo.sh install "$(DESTDIR)$(localedir)"

strip:
install-strip: install

uninstall:
	@sh zzpo.sh uninstall "$(DESTDIR)$(localedir)"

mostlyclean:
	rm -f *.new.po *.mo *.gmo
clean: mostlyclean
distclean: mostlyclean

update-pot:
	@sh zzpo.sh pot

update-po:
	@sh zzpo.sh po

