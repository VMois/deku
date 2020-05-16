CPP_SOURCES = $(wildcard src/*.cpp src/*/*.cpp)
TEST_SOURCES = $(wildcard tests/cpp/*.cpp)
HEADERS = $(wildcard src/*.h src/*/*.h)

# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o}
TEST_OBJ = ${TEST_SOURCES:.cpp=.o}

# -g: Use debugging symbols in gcc
CFLAGS = -std=c++14 -Wall -Wextra
LIBRARIES = -ldeku -pthread -lczmq -lzmq -lhiredis
PATHS = -Llibs -I. -Iinclude

build_example_echo: build_lib
	g++ ${CFLAGS} ${PATHS} examples/echo/responder.cpp ${LIBRARIES} -o build/responder.o
	g++ ${CFLAGS} ${PATHS} examples/echo/requester.cpp ${LIBRARIES} -o build/requester.o

build_example_file: build_lib
	g++ ${CFLAGS} ${PATHS} examples/file/responder.cpp ${LIBRARIES} -o build/responder.o
	g++ ${CFLAGS} ${PATHS} examples/file/requester.cpp ${LIBRARIES} -o build/requester.o

build_lib: ${OBJ}
	ar rvs libs/libdeku.a src/*.o src/*/*.o

%.o: %.cpp ${HEADERS}
	g++ ${CFLAGS} ${PATHS} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf build/*.o
	rm -rf src/*.o
	rm -rf src/*/*.o
	rm -rf tests/*.o
