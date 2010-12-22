
ifneq ($(findstring MINGW,$(shell uname -s 2> /dev/null)),)
win32 = Yep
endif

ifdef win32
EXT := .exe
else
EXT :=
endif

BINARY := cpuid$(EXT)

all: $(BINARY)

CC := gcc
CFLAGS := -Os -fno-strict-aliasing -std=gnu89 -Wall -Wextra -Werror
OBJECTS := cache.o cpuid.o feature.o handlers.o main.o util.o

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(BINARY)
	rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

