# TODO: separate container and builder
FROM ubuntu:bionic
 
# install build dependencies
RUN apt-get update && apt-get install -y g++ make

# install Redis C client
RUN apt-get install -y libczmq-dev libhiredis-dev=0.13.3-2.2

COPY . /app/
WORKDIR /app
RUN make clean && make build_example