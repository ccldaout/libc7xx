SHELL = /bin/bash
MAKE_JOBS ?= 8

include Makefile.version

UNITS = src tools
.PHONY: rebuild clean all $(UNITS) push pull

GITTAG_F=r$(C7_VER_MAJOR).$(C7_VER_MINOR).$(C7_VER_PATCH)
GITTAG_S=r$(C7_VER_MAJOR).$(C7_VER_MINOR)

all: $(UNITS)

$(UNITS):
	@$(MAKE) --directory=$@ --no-print-directory clean
	$(MAKE) --directory=$@ --no-print-directory

tag_move:
	[[ $$(c7gitbr) = master ]]
	git tag -a -f -m $(GITTAG_F) $(GITTAG_F)
	git tag -a -f -m $(GITTAG_S) $(GITTAG_S)
	git push -f --tags
	git push -f --tags github

push:
	git push -f
	git push -f --tags
	git push -f github
	git push -f --tags github

pull:
	git pull; git fetch github; git fetch -f --tags
