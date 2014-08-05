# $Header: /cvs/G/CRLOGIC/Makefile,v 1.4 2012/05/25 06:50:13 ebner Exp $

LIBVERSION = 4.4.5

build:
	$(MAKE) build -C src LIBVERSION=$(LIBVERSION)

clean:
	$(MAKE) clean -C src LIBVERSION=$(LIBVERSION)

install: build
	$(MAKE) install -C src LIBVERSION=$(LIBVERSION)

uninstall:
	rm /work/sls/config/medm/G_CRLOGIC_expert.adl

	$(MAKE) uninstall -C src LIBVERSION=$(LIBVERSION)

medm:
	cp App/config/medm/G_CRLOGIC_expert.adl /work/sls/config/medm

help:
	@echo "The following targets are available with this Makefile:-"
	@echo "make (calls default target)"
	@echo "make build (default)"
	@echo "make clean"
	@echo "make install"
	@echo "make uninstall"
	@echo "make help"
