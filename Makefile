# Pseudo-recursive Makefile

SUBDIRS = protobuf src helpers configs test

all:
	@for DIR in ${SUBDIRS}; do $(MAKE) all -C $$DIR; done

clean:
	@for DIR in ${SUBDIRS}; do $(MAKE) clean -C $$DIR; done
	rm -fr build

install:
	@for DIR in ${SUBDIRS}; do $(MAKE) install -C $$DIR; done
