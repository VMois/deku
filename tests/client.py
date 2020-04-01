#!/usr/bin/env python3

# script for testing single Responder instance on local computer

import socket
import msgpack
import math
from io import BytesIO

HOST = '127.0.0.1'
PORT = 3434
CHUNK_SIZE = 4  # must be the same as in C++ Responder code

def encode(b: bytes):
    buf = BytesIO()
    chunks_num = math.ceil(len(b) / (CHUNK_SIZE - 1))
    for i in range(chunks_num):
        if i is not chunks_num - 1:
            buf.write(b'\x01')
            buf.write(b[i * (CHUNK_SIZE - 1): (i + 1) * (CHUNK_SIZE - 1)])
        else:
            buf.write(b'\x00')
            tmp_b = b[i * (CHUNK_SIZE - 1): (i + 1) * (CHUNK_SIZE - 1)]
            buf.write(tmp_b)
            padding = (CHUNK_SIZE - 1) - len(tmp_b)
            buf.write(padding * b'\x00')
    buf.seek(0)
    return buf

def pack_job(function_name: str, raw_input_data: bytes):
    return 2, msgpack.packb((function_name, raw_input_data), use_bin_type=True)

# user-defined input
input_data = [1,2,3]
raw_input_data = msgpack.packb(input_data, use_bin_type=True)
print(raw_input_data)

# job protocol
protocol_id, raw_job_metadata = pack_job('echo', raw_input_data)

# message data pack
data = (protocol_id, raw_job_metadata)
raw_data = msgpack.packb(data, use_bin_type=True)

# socker data pack
encoded_data = encode(raw_data)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(encoded_data.read())
    data = s.recv(1024)

print('Received: ', repr(data))
