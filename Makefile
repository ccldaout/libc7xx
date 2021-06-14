MAKE_JOBS ?= 8

UNITS = src tools
.PHONY: rebuild clean all $(UNITS)

all: $(UNITS)
$(UNITS):
	$(MAKE) --directory=$@ --no-print-directory clean
	$(MAKE) --directory=$@ --no-print-directory -j$(MAKE_JOBS)
