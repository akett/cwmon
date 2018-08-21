CXX=g++
CXXFLAGS=-g -Wall -std=c++14
LDLIBS=-L/usr/lib/arm-linux/gnueabihf -L/usr/include/pqxx -L/usr/include/postgresql -lwiringPi -lpqxx -lpq -lncurses

cwmon: main.o
	$(CXX) $(CXXFLAGS) $(LDLIBS) -o cwmon main.o

main.o: main.h
	$(CXX) $(CXXFLAGS) $(LDLIBS) -c main.cpp

clean:
	-rm *.o $(objects) cwmon
