
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
CFLAGS := -Os -fno-strict-aliasing -std=gnu89 -Wall -Wextra -Wpadded -pedantic
OBJECTS := cache.o cpuid.o feature.o handlers.o main.o util.o version.o

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
DEPS := $(shell ls $(OBJECTS:.o=.d) 2>/dev/null)

ifneq ($(DEPS),)
-include $(DEPS)
endif
endif

depend: $(DEPS)

$(BINARY): $(OBJECTS)
	$(QUIET_LINK)$(CC) -o $@ $(OBJECTS)

clean:
	$(QUIET)rm -f $(BINARY)
	$(QUIET)rm -f $(OBJECTS) build.h license.h
	$(QUIET)rm -f $(DEPS)

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

