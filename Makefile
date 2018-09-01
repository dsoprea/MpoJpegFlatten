CFLAGS=-O0 -g3 -std=c++11
# -Wall

main: main.o
	g++ $(CFLAGS) -o main main.o -lturbojpeg

main.o: main.cpp
	g++ $(CFLAGS) -c -o main.o main.cpp
