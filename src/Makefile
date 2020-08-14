#
# Makefile
#
# Copyright (c) 2019 ccldaout@gmail.com
#
# This software is released under the MIT License.
# http://opensource.org/licenses/mit-license.php
#


C7_TARGET_BASE = libc7++
C7_CC_IPATH = .

include ../Makefile.common

CPPFLAGS += -fPIC
C7_LD_LPATH =
C7_LD_RPATH =
C7_NOLIBC7 = y

SRCS := $(wildcard *.cpp)
OBJS := $(addprefix $(C7_OUT_OBJDIR)/,$(patsubst %.cpp,%.o,$(SRCS)))

.PHONY: build
build: init headers $(C7_TARGET_LIB)

$(C7_TARGET_LIB): $(OBJS) .
	$(call cmd_o,c++ -shared $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS))

.PHONY: headers
headers:
	@mkdir -p $(C7_OUT_INCDIR) >/dev/null 2>&1
	@cp -p c7*.hpp $(C7_OUT_INCDIR)/

.PHONY: depend
depend:
	@rm -f Makefile.dep
	@for cpp in *.cpp; do \
	  c++ -c $(C7_CXXFLAGS) -MM -MT '$$(C7_OUT_OBJDIR)'/$${cpp%.cpp}.o $${cpp} >> Makefile.dep; \
	done

include Makefile.dep
