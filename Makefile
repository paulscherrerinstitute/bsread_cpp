# $Header: /cvs/G/CRLOGIC/Makefile,v 1.1 2011/10/18 07:03:28 ebner Exp $

build:
	$(MAKE) build -C src/CRLOGICCore
	$(MAKE) build -C src/CRLOGICTimestamp
	$(MAKE) build -C src/CRLOGICMaxV
	$(MAKE) build -C src/CRLOGICVSC16
	$(MAKE) build -C src/CRLOGICDCR508
	$(MAKE) build -C src/CRLOGICVME58
	$(MAKE) build -C src/CRLOGICECM5xx
	$(MAKE) build -C src/CRLOGICHy8001Trigger
	$(MAKE) build -C src/CRLOGICChannel

clean:
	$(MAKE) clean -C src/CRLOGICCore
	$(MAKE) clean -C src/CRLOGICTimestamp
	$(MAKE) clean -C src/CRLOGICMaxV
	$(MAKE) clean -C src/CRLOGICVSC16
	$(MAKE) clean -C src/CRLOGICDCR508
	$(MAKE) clean -C src/CRLOGICVME58
	$(MAKE) clean -C src/CRLOGICECM5xx
	$(MAKE) clean -C src/CRLOGICHy8001Trigger
	$(MAKE) clean -C src/CRLOGICChannel

install: build
	$(MAKE) install -C src/CRLOGICCore
	$(MAKE) install -C src/CRLOGICTimestamp
	$(MAKE) install -C src/CRLOGICMaxV
	$(MAKE) install -C src/CRLOGICVSC16
	$(MAKE) install -C src/CRLOGICDCR508
	$(MAKE) install -C src/CRLOGICVME58
	$(MAKE) install -C src/CRLOGICECM5xx
	$(MAKE) install -C src/CRLOGICHy8001Trigger
	$(MAKE) install -C src/CRLOGICChannel

uninstall:
	$(MAKE) uninstall -C src/CRLOGICCore
	$(MAKE) uninstall -C src/CRLOGICTimestamp
	$(MAKE) uninstall -C src/CRLOGICMaxV
	$(MAKE) uninstall -C src/CRLOGICVSC16
	$(MAKE) uninstall -C src/CRLOGICDCR508
	$(MAKE) uninstall -C src/CRLOGICVME58
	$(MAKE) uninstall -C src/CRLOGICECM5xx
	$(MAKE) uninstall -C src/CRLOGICHy8001Trigger
	$(MAKE) uninstall -C src/CRLOGICChannel

help:
	@echo "The following targets are available with this Makefile:-"
	@echo "make (calls default target)"
	@echo "make build (default)"
	@echo "make clean"
	@echo "make install"
	@echo "make uninstall"
	@echo "make help"
