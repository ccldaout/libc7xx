#
# Makefile
#
# Copyright (c) 2019 ccldaout@gmail.com
#
# This software is released under the MIT License.
# http://opensource.org/licenses/mit-license.php
#


SHELL = /bin/bash

C7_TARGET_BASE = libc7++$(C7_TARGET_DBGSFX)
C7_CC_IPATH = .

include ../Makefile.version
include ../Makefile.common

CXXFLAGS += -fPIC
C7_LD_LPATH =
C7_LD_RPATH =
C7_NOLIBC7XX = y

C7_SONAME      = $(C7_TARGET_BASE).$(C7_LIBSFX).$(C7_VER_MAJOR)
C7_REAL_NAME   = $(C7_TARGET_BASE).$(C7_LIBSFX).$(C7_VER_MAJOR).$(C7_VER_MINOR).$(C7_VER_PATCH)
C7_LINKER_NAME = $(C7_TARGET_BASE).$(C7_LIBSFX)
C7_VERSION_HPP = $(C7_OUT_INCDIR)/_c7version.hpp

LDFLAGS += -Wl,-soname,$(C7_SONAME)

ARCH := $(shell uname -m)
HDRS := $(filter-out %_private.hpp,$(wildcard *.hpp */*.hpp */*/*.hpp))
SRCS := $(wildcard *.$(ARCH).s *.cpp */*.cpp */*/*.cpp)
OBJS := $(addprefix $(C7_OUT_OBJDIR)/,$(patsubst %.s,%.o,$(patsubst %.cpp,%.o,$(SRCS))))

C7_DIRS += $(C7_OUT_OBJDIR)/c7event
C7_DIRS += $(C7_OUT_OBJDIR)/c7event/ext
C7_DIRS += $(C7_OUT_OBJDIR)/c7format
C7_DIRS += $(C7_OUT_OBJDIR)/c7nseq
C7_DIRS += $(C7_OUT_OBJDIR)/c7strmbuf
C7_DIRS += $(C7_OUT_OBJDIR)/c7thread


$(C7_OUT_OBJDIR)/c7event/%.o: %.cpp
	$(call cmd_o,$(CXX) -c $(CXXFLAGS) -o $@ $<)

$(C7_OUT_OBJDIR)/c7event/ext/%.o: %.cpp
	$(call cmd_o,$(CXX) -c $(CXXFLAGS) -o $@ $<)

$(C7_OUT_OBJDIR)/c7thread/%.o: %.cpp
	$(call cmd_o,$(CXX) -c $(CXXFLAGS) -o $@ $<)


.PHONY: build
build:
	@$(MAKE) --no-print-directory init headers
	@$(MAKE) --no-print-directory -j$(MAKE_JOBS) $(C7_TARGET_LIB)

$(C7_TARGET_LIB): $(OBJS) .
	$(call cmd_o,c++ -shared $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS))
	@ln -sf $(C7_REAL_NAME) $(C7_OUT_LIBDIR)/$(C7_SONAME)
	@ln -sf $(C7_REAL_NAME) $(C7_OUT_LIBDIR)/$(C7_LINKER_NAME)

.PHONY: headers
headers:
	@mkdir -p $(C7_OUT_INCDIR) >/dev/null 2>&1
	@tar cf - $(HDRS) | tar xpf - -C $(C7_OUT_INCDIR)
	@echo "#define C7XX_VERSION_MAJOR ($(C7_VER_MAJOR)UL)" >| $(C7_VERSION_HPP)
	@echo "#define C7XX_VERSION_MINOR ($(C7_VER_MINOR)UL)" >> $(C7_VERSION_HPP)
	@echo "#define C7XX_VERSION_PATCH ($(C7_VER_PATCH)UL)" >> $(C7_VERSION_HPP)
	@echo "#define C7XX_VERSION_make(r0_, r1_, r2_) ((r0_)*1000000UL + (r1_)*1000UL + (r2_))" >> $(C7_VERSION_HPP)
	@echo "#define C7XX_VERSION C7XX_VERSION_make(C7XX_VERSION_MAJOR, C7XX_VERSION_MINOR, C7XX_VERSION_PATCH)" >> $(C7_VERSION_HPP)

.PHONY: depend
depend:
	@rm -f $(C7_OUT_OBJDIR)/Makefile.dep.*
	@rm -f $(C7_VERSION_HPP)
	@for cpp in $(filter %.cpp,$(SRCS)); do \
	  c++ -c $(CXXFLAGS) -MM -MT '$$(C7_OUT_OBJDIR)'/$${cpp%.cpp}.o $${cpp} >| $(C7_OUT_OBJDIR)/Makefile.dep.$${cpp##*/}; \
	done
	@cat $(C7_OUT_OBJDIR)/Makefile.dep.* >| Makefile.dep


include Makefile.dep
