
SUBDIRS = data po src 

DISTCLEAN_FILES = 

# ===========================================================

include config.mk

DISTCLEAN_FILES += config.h config.mk config.log config.sh

MAKEFLAGS += --no-print-directory

all:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

strip:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

clean:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

distclean:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done
	-rm -f $(DISTCLEAN_FILES)

install:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

install-strip:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

uninstall:
	@for dir in ${SUBDIRS} ; do \
		echo "$(MAKE): Entering directory [$${dir}]"; \
		$(MAKE) -C $${dir} $@ || exit 1; \
		echo "$(MAKE): Leaving directory [$${dir}]"; \
	done

check:

distcheck:

installcheck:

dist:
	sh configure release dist

.PHONY: subdirs $(SUBDIRS)

