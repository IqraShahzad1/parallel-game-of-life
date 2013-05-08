parallel-game-of-life
=====================

C++ programs that implement the Message Passing Interface (MPI) to run simulations of Conway's Game of Life on a distributed system using different communication models:

lifeparallel.cpp - Runs a number of Conway's Game of Life simulations on a distributed system. The number of simulations run is divided equally among the processors.

split_life_game.cpp - Runs a simulation of Conway's Game of Life once with a distributed system. Management of the grid for this simulation is divided among four processors.

These programs were written for a Parallel Computing class and are based on an existing serial C program for the Game of Life provided by the professor.
