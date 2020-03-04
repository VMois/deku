CPP_SOURCES = $(wildcard src/*.cpp src/discover/*.cpp)
HEADERS = $(wildcard src/*.h src/discover/*.h)

# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o}

# -g: Use debugging symbols in gcc
CFLAGS = -std=c++11 -Wall -Wextra

build_docker: build_example
	docker build -t deku .

build_example: example

example: ${OBJ} 
	g++ ${OBJ} -lhiredis example.cpp -o example.o

%.o: %.cpp ${HEADERS}
	g++ ${CFLAGS} -c $< -o $@

clean:
	rm -rf *.o *.a
	rm -rf src/*.o
