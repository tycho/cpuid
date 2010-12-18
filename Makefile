all: cpuid

CC := gcc
CFLAGS := -O2
OBJECTS := amd.o cpuid.o feature.o intel.o main.o 

cpuid: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	-rm cpuid
	-rm $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

