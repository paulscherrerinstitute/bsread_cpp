
#LIBVERSION = 1.0.0
LIBVERSION = test

build:
	$(MAKE) build -C src LIBVERSION=$(LIBVERSION)

clean:
	$(MAKE) clean -C src LIBVERSION=$(LIBVERSION)

install: build
	$(MAKE) install -C src LIBVERSION=$(LIBVERSION)

uninstall:
	rm /work/sls/config/medm/bsread.adl

	$(MAKE) uninstall -C src LIBVERSION=$(LIBVERSION)

medm:
	cp App/config/medm/bsread.adl /work/sls/config/medm

help:
	@echo "The following targets are available with this Makefile:-"
	@echo "make (calls default target)"
	@echo "make build (default)"
	@echo "make clean"
	@echo "make install"
	@echo "make uninstall"
	@echo "make help"
