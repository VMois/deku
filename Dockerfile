FROM ubuntu:bionic
 
# install build dependencies
#RUN apt-get update && apt-get install -y g++ make

WORKDIR /app
COPY example.o /app/main.o

CMD ["/app/main.o"]