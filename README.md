# MPI TSP
Multi-threaded implementation of the travelling salesman problem

## What it is ?

The travelling salesman problem (TSP) asks the following question: "Given a list of cities and the distances between each pair of cities, what is the shortest possible route that visits each city exactly once and returns to the origin city?" It is an NP-hard problem in combinatorial optimization, important in theoretical computer science and operations research.

In the theory of computational complexity, the decision version of the TSP (where given a length L, the task is to decide whether the graph has a tour of at most L) belongs to the class of NP-complete problems. Thus, it is possible that the worst-case running time for any algorithm for the TSP increases superpolynomially (but no more than exponentially) with the number of cities.

## Implementation

In general, the implementation followed the path suggested by Peter Pacheco in the reference book. Only a few parts have been modified. One of them is at the beginning of the algorithm, when the main process (process 0) needs to send the problem data to the other processes. We ended up solving this problem in a little different way. Instead of sending the entire graph matrix to the other processes, and replicating the search in width to generate tasks in each one of them, we chose to do this search and division in the initial process. After generating each stack of tasks for each process, the main process sends only the generated stacks, and the other processes already start working on top of the stack soon after receiving them ready-made. These processes communicate via MPI and, within each process, we use threads through pthreads. Between these threads, they have a dynamic work balance, using the strategy described in the book.

As for the search in width and the division of tasks itself, we follow the logic suggested in the book, generating tasks and putting them in a queue until we reach a level of the tree that has a number of generated tasks greater than the number of processes. This function is also repeated within each process to generate the different task stacks for each thread.

Having all the stacks of each thread in each process ready, the algorithm starts to run in parallel. We have an Evaluate_Tours function responsible for generating new tours and comparing them with the best found so far. It also executes the dynamic balancing strategy between processes, checking if there are processes waiting for new tasks and dividing the stacks if necessary.

Finally, after each thread and each process finishes its searches, the reduction strategy is made to find which thread has the lowest cost tour. We also follow what was suggested in the book to use the MPI_Allreduce operator to find the process with the best tour, and then this process sends the tour in question to the main process, which is printed on the screen finalizing the algorithm.

## Usage

We have 3 scenarios available: TSP for 5, 15 or 17 cities. To run the test, just run the following command defining which of these scenarios you will use:
```
$ make run-mpi-<number>
```
We created a docker-compose to make the test easier to run. To use it, just run the following command:
```
$ make up 
```