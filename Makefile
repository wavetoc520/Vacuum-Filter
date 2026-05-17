CFLAGS = -pg -g -Wall -std=c++14 -mpopcnt -march=native

all: test

test: test.cpp vacuum.h hash.h
	g++ $(CFLAGS) -Ofast -o main main.cpp 

clean:
	rm -f main
