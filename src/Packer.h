#pragma once
#include <math.h>
#include <sstream>

class Packer {
    static const int CHUNK_SIZE;
    public:
        static void encode(std::stringstream& input, char*& buffer, int& size);
        static bool decode_chunk(std::stringstream& output, char* buffer);
        static int getChunkSize();
};
