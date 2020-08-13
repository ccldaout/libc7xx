TMPDIR = $(HOME)/tmp
OBJDIR = $(TMPDIR)/obj
DBGDIR = $(TMPDIR)/gdb
BINDIR = $(TMPDIR)/bin
LIBDIR = $(TMPDIR)/lib

MAKE_JOBS ?= 4

CPP_STD ?= c++17
CPPFLAGS_ = -std=$(CPP_STD) -fPIC -I. -I$(HOME)/.ccldaout/include -Wall -Werror
 LDFLAGS_ = -L$(LIBDIR) -Wl,-rpath,$(LIBDIR)
  LDLIBS_ = -lc7++ -lpthread

ifeq "$(wildcard .DEBUG)" ""
  CPPFLAGS_ += -O3
  ECHO_BEG = "\\x1b[32m"
  ECHO_END = "\\x1b[0m"
else
  CPPFLAGS_ += -O0 -g2 -D__C7_DEBUG
  OBJDIR = $(DBGDIR)
  LIBDIR = $(DBGDIR)
  BINSFX = .g
  ECHO_BEG = "\\x1b[33m"
  ECHO_END = "\\x1b[0m"
endif

CPPFLAGS_ += $(CPPFLAGS)

ifeq (,$(findstring s,$(MAKEFLAGS)))
  ECHO = /bin/echo -e
else
  ECHO = :
endif

define cmd
  @$(ECHO) $(ECHO_BEG)$1$(ECHO_END)
  @$1
endef

$(OBJDIR)/%.o: %.cpp
	$(call cmd,c++ -c $(CPPFLAGS_) -o $@ $<)

$(BINDIR)/%: $(OBJDIR)/%_test.o
	$(call cmd,c++ -g $(LDFLAGS_) -o $@ $(filter %.o,$^) $(LDLIBS_))

$(BINDIR)/%$(BINSFX): $(OBJDIR)/%_test.o
	$(call cmd,c++ -g $(LDFLAGS_) -o $@ $(filter %.o,$^) $(LDLIBS_))

LIBSRC = $(filter-out %_test.cpp,$(wildcard *.cpp))
LIBOBJ = $(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(LIBSRC)))
 PROGS = $(addsuffix $(BINSFX),$(addprefix $(BINDIR)/,$(subst _test.cpp,,$(wildcard *_test.cpp))))

.PHONY: build
build:
	@make -j$(MAKE_JOBS) --no-print-directory lib
	@make -j$(MAKE_JOBS) --no-print-directory progs

.PHONY: lib
lib: $(LIBDIR)/libc7++.so
$(LIBDIR)/libc7++.so: $(LIBOBJ)
	$(call cmd,c++ -shared -g -o $@ $^ -lpthread)

.PHONY: progs
progs: $(PROGS)

.PHONY: debug
debug:
	@mkdir -p .DEBUG
	@make --no-print-directory

.PHONY: nodebug
nodebug:
	@rm -fr .DEBUG
	@make --no-print-directory

.PHONY: depend
depend:
	@rm -f $(OBJDIR)/Makefile.dep
	@for cpp in *.cpp; do \
	  c++ -c $(CPPFLAGS_) -MM -MT '$$(OBJDIR)'/$${cpp%.cpp}.o $${cpp} >> $(OBJDIR)/Makefile.dep; \
	done
	@mv -f $(OBJDIR)/Makefile.dep Makefile.dep

.PHONY: init
init:
	mkdir -p $(TMPDIR) $(OBJDIR) $(DBGDIR) $(LIBDIR) $(BINDIR)

.PHONY: clean
clean:
	rm -f $(PROGS) $(LIBDIR)/*.so
	rm -f $(OBJDIR)/*.o

.PHONY: export
export:
	@chmod u+w $(HOME)/lib
	cp -p $(LIBSRC) *.hpp $(HOME)/prog/libc7xx/src/
	cp -p *_test.cpp $(HOME)/prog/libc7xx/test/
	cp -p *.hpp $(HOME)/h/
	cp -p $(LIBDIR)/libc7++.so $(HOME)/lib/


include Makefile.dep
