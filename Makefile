.SILENT:
.DEFAULT_GOAL := help

COLOR_RESET = \033[0m
COLOR_COMMAND = \033[36m
COLOR_YELLOW = \033[33m
COLOR_GREEN = \033[32m
COLOR_RED = \033[31m

PROJECT := Parallel TSP

CFLAGS = -Wall

## Object files
_OBJ_FILES = tour.o stack.o graph.o tsp.o utils.o queue.o
OBJ_FILES = $(_OBJ_FILES:%.o=./src/obj/%.o)

## Build imagem Docker
build:
	docker build -t parallel-tsp .

## Run project
run:
	docker-compose up

tour: ./src/tour.c ./headers/tour.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

stack: ./src/stack.c ./headers/stack.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

graph: ./src/graph.c ./headers/graph.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

tsp: ./src/tsp.c ./headers/tsp.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

utils: ./src/utils.c ./headers/utils.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

queue: ./src/queue.c ./headers/queue.h
	gcc $(CFLAGS) -c $< -o ./src/obj/$@.o

## Make project dependencies
dependencies:
	make tour
	make stack
	make graph
	make tsp
	make utils
	make queue

## Make entire app
app:
	make dependencies
	gcc $(CFLAGS) -pthread -o main main.c $(OBJ_FILES)

## MPI app
mpiapp:
	make dependencies
	mpicc -pthread -o main main.c $(OBJ_FILES)

## Make App and run with cinco.txt instance
run-threads-cinco: app
	./main 4 5 instances/cinco.txt

## Make App and run with gr17.txt instance
run-threads-gr17: app
	./main 4 17 instances/gr17.txt

## Make App and run with quinze.txt instance
run-threads-quinze: app
	./main 4 15 instances/quinze.txt

## Make MpiApp and run with cinco.txt instance
run-mpi-cinco: mpiapp
	mpiexec -np 4 ./main 4 5 instances/cinco.txt

## Make MpiApp and run with gr17.txt instance
run-mpi-gr17: mpiapp
	mpiexec -np 4 ./main 4 17 instances/gr17.txt

## Make MpiApp and run with quinze.txt instance
run-mpi-quinze: mpiapp
	mpiexec -np 4 ./main 4 15 instances/quinze.txt

## Commands
help:
	printf "\n${COLOR_YELLOW}${PROJECT}\n--------------------\n${COLOR_RESET}"
	awk '/^[a-zA-Z\-\_0-9\.%]+:/ { \
		helpMessage = match(lastLine, /^## (.*)/); \
		if (helpMessage) { \
			helpCommand = substr($$1, 0, index($$1, ":")); \
			helpMessage = substr(lastLine, RSTART + 3, RLENGTH); \
			printf "${COLOR_COMMAND}$$ make %s${COLOR_RESET} %s\n", helpCommand, helpMessage; \
		} \
	} \
	{ lastLine = $$0 }' $(MAKEFILE_LIST) | sort
	printf "\n"
