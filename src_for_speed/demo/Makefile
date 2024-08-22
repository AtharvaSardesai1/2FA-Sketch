GCC = g++
CFLAGS = -O2 -std=c++14
SSEFLAGS = -msse2 -mssse3 -msse4.1 -msse4.2 -mavx -march=native
FILES = elastic.out 1FA.out 2FASketch.out chainsketch.out spacesaving.out countheap.out cmheap.out

all: $(FILES) 

elastic.out: elastic.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o elastic.out elastic.cpp

1FA.out: 1FA.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o 1FA.out 1FA.cpp

chainsketch.out: chainsketch.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o chainsketch.out chainsketch.cpp

2FASketch.out: 2FASketch.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o 2FASketch.out 2FASketch.cpp

spacesaving.out: spacesaving.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o spacesaving.out spacesaving.cpp

countheap.out: countheap.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o countheap.out countheap.cpp

cmheap.out: cmheap.cpp
	$(GCC) $(CFLAGS) $(SSEFLAGS) -o cmheap.out cmheap.cpp

clean:
	rm $(all) -f *~ *.o *.out
