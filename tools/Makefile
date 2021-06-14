#
# Makefile
#
# Copyright (c) 2019 ccldaout@gmail.com
#
# This software is released under the MIT License.
# http://opensource.org/licenses/mit-license.php
#


C7_TARGET_BASE = tools

include ../Makefile.common

SRCS := $(wildcard *.cpp)
PRGS := $(addprefix $(C7_OUT_BINDIR)/,$(patsubst %.c,%,$(SRCS)))

C7_CLEAN_REMOVED += $(PRGS)

.PHONY: build
build: pytools $(PRGS)

.PHONY: pytools
pytools: $(addprefix $(C7_OUT_PY2DIR)/,$(wildcard *.py))
