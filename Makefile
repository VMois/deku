CPP_SOURCES = $(wildcard src/*.cpp src/*/*.cpp)
HEADERS = $(wildcard src/*.h src/*/*.h)

TEST_SOURCES = $(wildcard tests/*.cpp)

# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o}
TEST_OBJ = ${TEST_SOURCES:.cpp=.o}

# -g: Use debugging symbols in gcc
CFLAGS = -std=c++14 -Wall -Wextra

build_docker: build_example
	docker build -t deku .

run_tests: build_tests
	./tests.o

build_tests: ${OBJ} ${TEST_OBJ}
	g++ ${CFLAGS} ${TEST_OBJ} ${OBJ} -lhiredis -o tests.o

build_example: ${OBJ} 
	g++ ${CFLAGS} ${OBJ} -lhiredis example.cpp -o example.o

%.o: %.cpp ${HEADERS}
	g++ ${CFLAGS} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf src/*.o
	rm -rf src/*/*.o
	rm -rf tests/*.o
