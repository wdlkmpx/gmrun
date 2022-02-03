# http://unlicense.org/

SUBDIRS = src data

DISTCLEAN_FILES = 

# ===========================================================

include config.mk

DISTCLEAN_FILES += config.h config.mk config.log

WSCRIPT = w_conf/_exec_make_dir.sh $(MAKE)

MAKEFLAGS += --no-print-directory

all: subdirs

subdirs:
	@sh $(WSCRIPT) "" $(SUBDIRS)

strip:
	@sh $(WSCRIPT) strip $(SUBDIRS)

clean:
	@sh $(WSCRIPT) clean $(SUBDIRS)

distclean:
	@sh $(WSCRIPT) distclean $(SUBDIRS)
	-rm -f $(DISTCLEAN_FILES)

install:
	@sh $(WSCRIPT) install $(SUBDIRS)

install-strip:
	@sh $(WSCRIPT) install-strip $(SUBDIRS)

uninstall:
	@sh $(WSCRIPT) uninstall $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS)
