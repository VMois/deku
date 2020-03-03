# TODO: separate container and builder
FROM ubuntu:bionic
 
# install build dependencies
RUN apt-get update && apt-get install -y g++ make

# install Redis C client
RUN apt-get install libhiredis-dev=0.13.3-2.2

WORKDIR /app
COPY example.o /app/main.o

CMD ["/app/main.o"]