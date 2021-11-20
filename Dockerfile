# syntax=docker/dockerfile:1
FROM ubuntu:18.04

RUN mkdir /app
WORKDIR /app

COPY . /app

RUN apt-get update && \
    apt-get install make && \
    apt-get -y install gcc libopenmpi-dev python-pip
RUN pip install mpi4py

