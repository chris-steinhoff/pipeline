
OBJS=pipeline.o
HEADERS=pipeline.hpp
CFLAGS=-Wall -ggdb -ansi -pedantic
CXXFLAGS=$(CFLAGS)
TEST_INC=/usr/local/include
GTEST_LIB=/usr/local/lib64/libgtest.a

.PHONY: clean

all: pipeline

pipeline: pipeline.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -pthread -o pipeline pipeline.o

pipeline.o: pipeline.cpp pipeline.hpp

test: $(HEADERS)
	g++ -I$(TEST_INC) $(CXXFLAGS) -pthread test_pipeline.cpp $(GTEST_LIB) -o gtest
	./gtest

clean:
	rm -f *.o pipeline

