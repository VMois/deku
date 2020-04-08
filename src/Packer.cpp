#include "Packer.h"

// writes data to output stream, return true if next chunk must be read
bool Packer::decode_chunk(std::stringstream& output, char* buffer) {
    output.write(buffer + 1, CHUNK_SIZE - 1);
    return buffer[0] == '\x01';
}

void Packer::encode(std::stringstream& input, char*& buffer, int& size) {
    input.seekg(0);
    size_t return_data_len = input.str().size();
    int chunks_num = std::ceil(float(return_data_len) / (CHUNK_SIZE - 1));
    size = chunks_num * CHUNK_SIZE;
    buffer = new char[size];

    for (int chunk_i = 0; chunk_i < chunks_num; chunk_i++) {
        int chunk_start_position  = CHUNK_SIZE * chunk_i;

        if (chunk_i != (chunks_num - 1)) {
            buffer[chunk_start_position] = '\x01';
            input.read(buffer + chunk_start_position + 1, 3);
        } else {
            buffer[chunk_start_position] = '\x00';
            int bytes_to_read = return_data_len - chunk_i * (CHUNK_SIZE - 1);
            input.read(buffer + chunk_start_position + 1, bytes_to_read);
            for (int i = bytes_to_read + 1; i < CHUNK_SIZE; i++) {
                buffer[chunk_start_position + i] = '\x00';
            }
        }
    }
};

const int Packer::CHUNK_SIZE = 4;

int Packer::getChunkSize() {
    return CHUNK_SIZE;
}