
MAKEFLAGS += --no-builtin-rules --no-builtin-variables

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')

prefix := /usr/local
bindir := $(prefix)/bin

ifneq ($(findstring MINGW,$(uname_S)),)
win32 = Yep
endif

ifdef win32
EXT := .exe
else
EXT :=
endif

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
        QUIET_DEPEND    = @echo '   ' DEPEND $@;
        QUIET_CC        = @echo '   ' CC $@;
        QUIET_GEN       = @echo '   ' GEN $@;
        QUIET_LINK      = @echo '   ' LD $@;
        QUIET           = @
        MAKEFLAGS      += --no-print-directory
        export V
endif
endif

ifeq ($(strip $(MAKE_JOBS)),)
    ifeq ($(uname_S),Darwin)
        CPUS := $(shell /usr/sbin/sysctl -n hw.ncpu)
    endif
    ifeq ($(uname_S),Linux)
        CPUS := $(shell grep ^processor /proc/cpuinfo | wc -l)
    endif
    ifneq (,$(findstring MINGW,$(uname_S))$(findstring CYGWIN,$(uname_S)))
        CPUS := $(shell getconf _NPROCESSORS_ONLN)
    endif
    MAKE_JOBS := $(CPUS)
endif

ifeq ($(strip $(MAKE_JOBS)),)
    MAKE_JOBS := 8
endif

BINARY := cpuid$(EXT)

top-level-make:
	@$(MAKE) -f GNUmakefile -j$(MAKE_JOBS) all

all: $(BINARY)

cc_supports_flag = $(if $(shell $(CC) -xc -c /dev/null -o /dev/null $(1) 2>/dev/null && echo yes),$(1),)

CC := gcc
CP := cp -L
CFLAGS := -Os -I. -fno-strict-aliasing \
	-std=gnu89 \
	-Wall \
	-Wextra \
	-Wdeclaration-after-statement \
	-Wimplicit-function-declaration \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wno-long-long \
	$(call cc_supports_flag,-Wno-overlength-strings) \
	-Wold-style-definition \
	-Wstrict-prototypes \
	$(EXTRA_CFLAGS)

LDFLAGS := -lm $(EXTRA_CFLAGS)
OBJECTS := cache.o clock.o cpuid.o feature.o handlers.o main.o sanity.o threads.o util.o version.o

# GCC is too down-rev on Illumos to allow this
ifneq ($(uname_S),SunOS)
ifneq ($(CC),clang)
CFLAGS += -fPIC
LDFLAGS += -fPIC
endif
endif

ifeq ($(uname_S),Darwin)
CFLAGS += -arch x86_64
LDFLAGS += -arch x86_64
endif

ifeq ($(uname_S),Linux)
CFLAGS += -pthread
LDFLAGS += -pthread -lrt
endif

ifeq ($(uname_S),FreeBSD)
CC := clang
CFLAGS += -pthread
LDFLAGS += -pthread
endif

ifneq ($(findstring MINGW,$(uname_S))$(findstring MSYS,$(uname_S)),)
LDFLAGS += -lpthread -lwinmm
endif

ifneq ($(findstring CYGWIN,$(uname_S)),)
LDFLAGS += -lwinmm
endif

ifneq ($(shell $(CC) --version | grep Apple),)
APPLE_COMPILER := YesPlease
endif

ifeq ($(uname_S),Darwin)
ifneq ($(USE_CHUD),)
CFLAGS += -m32 -pthread -mdynamic-no-pic -DUSE_CHUD
LDFLAGS += -m32 -pthread -mdynamic-no-pic -Wl,-F/System/Library/PrivateFrameworks -Wl,-framework,CHUD
endif
endif

ifdef NO_GNU_GETOPT
CFLAGS += -Igetopt
OBJECTS += getopt/getopt_long.o
endif

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
DEPS := $(shell ls $(OBJECTS:.o=.d) 2>/dev/null)

ifneq ($(DEPS),)
-include $(DEPS)
endif
endif

.PHONY: all depend clean distclean install

install: $(BINARY)
	install -D -m0755 $(BINARY) $(DESTDIR)$(bindir)/$(BINARY)

depend: $(DEPS)

$(BINARY): $(OBJECTS)
	$(QUIET_LINK)$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	$(QUIET)rm -f .cflags
	$(QUIET)rm -f $(BINARY)
	$(QUIET)rm -f $(OBJECTS) build.h license.h
	$(QUIET)rm -f $(OBJECTS:.o=.d)

ifdef NO_INLINE_DEPGEN
$(OBJECTS): $(OBJECTS:.o=.d)
endif

%.d: %.c .cflags
	$(QUIET_DEPEND)$(CC) -MM $(CFLAGS) -MT $*.o $< > $*.d

%.o: %.c .cflags
ifdef NO_INLINE_DEPGEN
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<
else
	$(QUIET_CC)$(CC) $(CFLAGS) -MD -c -o $@ $<
endif

build.h: .force-regen
	$(QUIET_GEN)tools/build.pl build.h

.PHONY: .force-regen

license.h: LICENSE
	$(QUIET_GEN)tools/license.pl LICENSE license.h

version.o: license.h build.h

ifeq (,$(findstring clean,$(MAKECMDGOALS)))

TRACK_CFLAGS = $(subst ','\'',$(CC) $(CFLAGS) $(uname_S) $(uname_O) $(prefix))

.cflags: .force-cflags
	@FLAGS='$(TRACK_CFLAGS)'; \
	if test x"$$FLAGS" != x"`cat .cflags 2>/dev/null`" ; then \
		echo "    * rebuilding cpuid: new build flags or prefix"; \
		echo "$$FLAGS" > .cflags; \
	fi

.PHONY: .force-cflags

endif

