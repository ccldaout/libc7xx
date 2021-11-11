#
# Makefile
#
# Copyright (c) 2019 ccldaout@gmail.com
#
# This software is released under the MIT License.
# http://opensource.org/licenses/mit-license.php
#


C7_TARGET_BASE = libc7++
C7_VER_MAJOR = 3
C7_VER_MINOR = 3
C7_VER_PATCH = 1
C7_CC_IPATH = .

include ../Makefile.common

CXXFLAGS += -fPIC
C7_LD_LPATH =
C7_LD_RPATH =
C7_NOLIBC7XX = y

C7_SONAME      = $(C7_TARGET_BASE).$(C7_LIBSFX).$(C7_VER_MAJOR)
C7_REAL_NAME   = $(C7_TARGET_BASE).$(C7_LIBSFX).$(C7_VER_MAJOR).$(C7_VER_MINOR).$(C7_VER_PATCH)
C7_LINKER_NAME = $(C7_TARGET_BASE).$(C7_LIBSFX)

LDFLAGS += -Wl,-soname,$(C7_SONAME)

HDRS := $(wildcard *.hpp */*.hpp */*/*.hpp)
SRCS := $(wildcard *.cpp */*.cpp */*/*.cpp)
OBJS := $(addprefix $(C7_OUT_OBJDIR)/,$(patsubst %.cpp,%.o,$(SRCS)))

C7_DIRS += $(C7_OUT_OBJDIR)/c7event $(C7_OUT_OBJDIR)/c7event/ext


$(C7_OUT_OBJDIR)/c7event/%.o: %.cpp
	$(call cmd_o,c++ -c $(CXXFLAGS) -o $@ $<)

$(C7_OUT_OBJDIR)/c7event/ext/%.o: %.cpp
	$(call cmd_o,c++ -c $(CXXFLAGS) -o $@ $<)


.PHONY: build
build: init headers $(C7_TARGET_LIB)

$(C7_TARGET_LIB): $(OBJS) .
	$(call cmd_o,c++ -shared $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS))
	@ln -sf $(C7_REAL_NAME) $(C7_OUT_LIBDIR)/$(C7_SONAME)
	@ln -sf $(C7_REAL_NAME) $(C7_OUT_LIBDIR)/$(C7_LINKER_NAME)

.PHONY: headers
headers:
	@mkdir -p $(C7_OUT_INCDIR) >/dev/null 2>&1
	@tar cf - $(HDRS) | tar xpf - -C $(C7_OUT_INCDIR)

.PHONY: depend
depend:
	@rm -f Makefile.dep
	@for cpp in $(SRCS); do \
	  c++ -c $(CXXFLAGS) -MM -MT '$$(C7_OUT_OBJDIR)'/$${cpp%.cpp}.o $${cpp} >> Makefile.dep; \
	done

include Makefile.dep
