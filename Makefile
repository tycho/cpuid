all: cpuid

CC := gcc
CFLAGS := -O2 -fno-strict-aliasing -std=gnu89 -Wall -Wextra -Werror
OBJECTS := cache.o cpuid.o feature.o handlers.o main.o util.o

cpuid: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f cpuid
	rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

