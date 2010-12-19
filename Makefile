all: cpuid

CC := gcc
CFLAGS := -std=gnu89 -pedantic -Wall -Wextra -O2 -fno-strict-aliasing
OBJECTS := cache.o cpuid.o feature.o main.o util.o

cpuid: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f cpuid
	rm -f $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

