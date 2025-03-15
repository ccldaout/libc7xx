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
PRGS := $(addprefix $(C7_OUT_BINDIR)/,$(patsubst %.cpp,%,$(SRCS)))
PY3  := $(wildcard *.py)
PY2  := cxx_token.py setup_dispatcher.py

C7_CLEAN_REMOVED += $(PRGS)

CFLAGS += -Wno-unused-value

$(C7_OUT_BINDIR)/c7prompt: C7_NOLIBC7XX = y
$(C7_OUT_BINDIR)/c7prompt: C7_NOTHREAD  = y
$(C7_OUT_BINDIR)/c7gitbr:  C7_NOLIBC7XX = y
$(C7_OUT_BINDIR)/c7gitbr:  C7_NOTHREAD  = y
$(C7_OUT_BINDIR)/c7npipe:  C7_NOLIBC7XX = y
$(C7_OUT_BINDIR)/c7npipe:  C7_NOTHREAD  = y

.PHONY: build
build: py3tools py2tools $(PRGS)

.PHONY: py3tools py2tools
py3tools: $(addprefix $(C7_OUT_PY3DIR)/,$(PY3))
py2tools: $(addprefix $(C7_OUT_PY2DIR)/,$(PY2))
