
MAKEFLAGS += -Rr

ifneq ($(findstring MINGW,$(shell uname -s 2> /dev/null)),)
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
        export V
endif
endif

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')

BINARY := cpuid$(EXT)

all: $(BINARY)

CC := gcc
CP := cp -L
CFLAGS := -Os -I../inc -I. -fno-PIC -fno-strict-aliasing -std=gnu99 -Wall -Wextra -Wwrite-strings -pedantic
LDFLAGS :=
OBJECTS := cache.o cpuid.o feature.o handlers.o main.o sanity.o threads.o util.o version.o

ifeq ($(uname_S),Linux)
CFLAGS += -pthread
LDFLAGS += -pthread
endif

ifneq ($(findstring MINGW,$(uname_S)),)
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

.PHONY: all depend clean distclean

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
	$(QUIET_CC)$(CC) $(CFLAGS) -Wp,-MD,$*.d,-MT,$@ -c -o $@ $<
endif

build.h: .force-regen
	$(QUIET_GEN)tools/build.pl build.h

.PHONY: .force-regen

license.h: COPYING
	$(QUIET_GEN)tools/license.pl COPYING license.h

version.d: license.h build.h

version.o: license.h build.h

ifeq (,$(findstring clean,$(MAKECMDGOALS)))

TRACK_CFLAGS = $(subst ','\'',$(CC) $(CFLAGS) $(uname_S) $(uname_O))

.cflags: .force-cflags
	@FLAGS='$(TRACK_CFLAGS)'; \
	if test x"$$FLAGS" != x"`cat .cflags 2>/dev/null`" ; then \
		echo "    * rebuilding cpuid: new build flags or prefix"; \
		echo "$$FLAGS" > .cflags; \
	fi

.PHONY: .force-cflags

endif

