CPP_SOURCES = $(wildcard src/*.cpp src/*/*.cpp)
TEST_SOURCES = $(wildcard tests/cpp/*.cpp)
HEADERS = $(wildcard src/*.h src/*/*.h)

# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o}
TEST_OBJ = ${TEST_SOURCES:.cpp=.o}

# -g: Use debugging symbols in gcc
CFLAGS = -std=c++14 -Wall -Wextra
LIBRARIES = -ldeku -pthread -lczmq -lzmq -lhiredis -DSPDLOG_COMPILED_LIB -lspdlog
PATHS = -Llibs -I. -Iinclude 

build_docker: build_example
	docker build -t deku .

run_tests: build_tests
	build/tests.o

build_client:
	g++ ${CFLAGS} ${PATHS} tests/client.cpp ${LIBRARIES} -o build/client.o

build_tests: build_lib ${TEST_OBJ}
	g++ ${CFLAGS} ${PATHS} ${TEST_OBJ} ${LIBRARIES} -o build/tests.o

build_example: build_lib
	g++ ${CFLAGS} ${PATHS} examples/responder.cpp ${LIBRARIES} -o build/responder.o
	g++ ${CFLAGS} ${PATHS} examples/requester.cpp ${LIBRARIES} -o build/requester.o

build_lib: ${OBJ}
	ar rvs libs/libdeku.a src/*.o src/*/*.o

%.o: %.cpp ${HEADERS}
	g++ ${CFLAGS} ${PATHS} ${LIBRARIES} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf build/*.o
	rm -rf src/*.o
	rm -rf src/*/*.o
	rm -rf tests/*.o
