# syntax=docker/dockerfile:1
FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install make && \
    apt-get -y install gcc libopenmpi-dev python-pip
RUN pip install mpi4py

ADD headers ./headers
ADD src ./src
ADD instances ./instances

COPY main.c ./
COPY Makefile ./

RUN make mpiapp
