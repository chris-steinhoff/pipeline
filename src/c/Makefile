
CFLAGS=-Wall -ansi -pedantic
CXXFLAGS=$(CFLAGS)

.PHONY: clean

all: pipeline

pipeline: pipeline.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -pthread -o pipeline pipeline.o

pipeline.o: pipeline.cpp pipeline.hpp

clean:
	rm -f *.o pipeline

