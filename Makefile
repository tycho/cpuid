all: cpuid

CC := gcc
CFLAGS := -O2
OBJECTS := cache.o cpuid.o feature.o main.o

cpuid: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	-rm cpuid
	-rm $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

