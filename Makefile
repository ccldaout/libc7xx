MAKE_JOBS ?= 8

include Makefile.version

UNITS = src tools
.PHONY: rebuild clean all $(UNITS) push

all: $(UNITS)
$(UNITS):
	@$(MAKE) --directory=$@ --no-print-directory clean
	$(MAKE) --directory=$@ --no-print-directory

GITTAG=r$(C7_VER_MAJOR).$(C7_VER_MINOR).$(C7_VER_PATCH)

push:
	git tag -a -f -m $(GITTAG) $(GITTAG)
	git push -f x22
	git push -f --tags x22
	git push -f github
	git push -f --tags github
