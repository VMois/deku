CPP_SOURCES = $(wildcard src/*.cpp src/*/*.cpp)
TEST_SOURCES = $(wildcard tests/cpp/*.cpp)
HEADERS = $(wildcard src/*.h src/*/*.h)

# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o}
TEST_OBJ = ${TEST_SOURCES:.cpp=.o}

# -g: Use debugging symbols in gcc
CFLAGS = -std=c++14 -Wall -Wextra
LIBRARIES = -pthread -lhiredis -DSPDLOG_COMPILED_LIB -lspdlog
PATHS = -Llibs -I. -Iinclude 

build_docker: build_example
	docker build -t deku .

run_tests: build_tests
	build/tests.o

build_tests: ${OBJ} ${TEST_OBJ}
	g++ ${CFLAGS} ${PATHS} ${TEST_OBJ} ${OBJ} ${LIBRARIES} -o build/tests.o

build_example: ${OBJ} 
	g++ ${CFLAGS} ${PATHS} ${OBJ} example.cpp ${LIBRARIES} -o build/example.o
	
%.o: %.cpp ${HEADERS}
	g++ ${CFLAGS} ${PATHS} ${LIBRARIES} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf build/*.o
	rm -rf src/*.o
	rm -rf src/*/*.o
	rm -rf tests/*.o
