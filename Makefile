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
	build/tests.o

build_tests: ${OBJ} ${TEST_OBJ}
	g++ ${CFLAGS} ${TEST_OBJ} ${OBJ} -pthread -lhiredis -o build/tests.o

build_example: ${OBJ} 
	g++ -Iinclude/ ${CFLAGS} ${OBJ} -lhiredis -pthread example.cpp -o build/example.o

%.o: %.cpp ${HEADERS}
	g++ -Iinclude/ ${CFLAGS} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf build/*.o
	rm -rf src/*.o
	rm -rf src/*/*.o
	rm -rf tests/*.o
