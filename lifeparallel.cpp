/*
 Name   : Melissa Steinkamp
 Program: Split Game of Life
 Class  : CSIS 434 Parallel Computing
 Date   : 3/6/12
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_X 500
#define MAX_Y 500

#define STEPS_MAX 200
#define UNCHANGED_MAX 10

/**
* Main program to run a number of games of Life on a distributed system,
* dividing the number of simulations to run equally among the processors.
*/
int main()
{
    int master = 0;
    MPI::Status status;
    int myID;
    int numProcs;
    double elapsed_time;
    
    int   grid[MAX_X+2][MAX_Y+2];   /* grid of vegetation values */
    int   nx = 0;           /* x dimension of grid */
    int   ny = 0;           /* y dimension of grid */
    int   max_steps;        /* max # timesteps to simulate */
    int   max_unchanged;    /* max # timesteps with no vegetation change */
    int   vegies;       /* amount of stable vegetation */
    int   nsteps;       /* number of steps actually run */
    int   nsims;        /* number of simulations to perform */
    int   local_nsims;  /* number of simulations to run on each processor */
    int   ndied;        /* # populations which die out */
    int   ndied_all;    /* total # populations which die out */
    int   nunsettled;       /* # populations which don't stablize */
    int   nunsettled_all;   /* total # populations which don't stabilize */
    int   nstable;      /* # populations which do stablize */
    int   nstable_all;      /* total # populations which do stabilize */
    float tot_steps_stable; /* total/average steps to stablization */
    float tot_veg_stable;   /* total/average stable vegetation */
    float tot_steps_stable_all; /* total/average steps to stablization for all procs */
    float tot_veg_stable_all;   /* total/average stable vegetation for all procs*/
    double prob;        /* population probability */
    int   seed = 0;
    int seed0;      /* random number seeds */
    int   i;            /* loop counter */
    void  initialize_grid(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int seed, double prob);
    int   game_o_life(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int max_steps, 
      	int max_unchanged, int *pvegies);
    
    // Initialize MPI, get rank and size
    MPI::Init();
    numProcs = MPI::COMM_WORLD.Get_size();
    myID = MPI::COMM_WORLD.Get_rank();
    
    ndied = 0;
    nunsettled = 0;
    nstable = 0;
    tot_steps_stable = 0;
    tot_veg_stable = 0;
    
    // Code for the master processor
    if (myID == master)
    {
        // Read in all the parameters.
        nx = MAX_X+1;
        while (nx > MAX_X || ny > MAX_Y) 
        {
            printf("Enter X and Y dimensions of wilderness: ");
            scanf("%d%d", &nx, &ny);
        }
    
        printf("Enter population probability: ");
        scanf("%lf", &prob);
    
        printf("Enter number of simulations: ");
        scanf("%d", &nsims);
    
        printf("Enter random number seed: ");
        scanf("%d", &seed0);
        
        elapsed_time = -MPI_Wtime();
        
        local_nsims = nsims / numProcs;
        
        // Send data to each of the other processors
        for (int j = 1; j < numProcs; j++)
        {
        	MPI::COMM_WORLD.Send(&nx, 1, MPI::INTEGER, j, 1);
        	MPI::COMM_WORLD.Send(&ny, 1, MPI::INTEGER, j, 2);
        	MPI::COMM_WORLD.Send(&prob, 1, MPI::DOUBLE, j, 3);
        	MPI::COMM_WORLD.Send(&seed0, 1, MPI::INTEGER, j, 4);
        	MPI::COMM_WORLD.Send(&local_nsims, 1, MPI::INTEGER, j, 5);
        }
    }
    // Code for other processors
    else
    {
    	// Receive data from master
    	MPI::COMM_WORLD.Recv(&nx, 1, MPI::INTEGER, 0, 1, status);
    	MPI::COMM_WORLD.Recv(&ny, 1, MPI::INTEGER, 0, 2, status);
    	MPI::COMM_WORLD.Recv(&prob, 1, MPI::DOUBLE, 0, 3, status);
    	MPI::COMM_WORLD.Recv(&seed0, 1, MPI::INTEGER, 0, 4, status);
    	MPI::COMM_WORLD.Recv(&local_nsims, 1, MPI::INTEGER, 0, 5, status);
    }
   
    // Every processor: execute a number of simulations
    for (i = 1; i <= local_nsims; i++) 
    {
        // Initialize the grid values using the given probability. 
        seed = seed0 * (i + (local_nsims * myID));
        initialize_grid(grid, nx, ny, seed, prob);

        // Now run the game of life simulation, returning the number of steps. 
        max_steps = STEPS_MAX;
        max_unchanged = UNCHANGED_MAX;
        nsteps = game_o_life(grid, nx, ny, max_steps, max_unchanged, &vegies);

        // Print result of the simulation
        printf("Number of time steps = %d, Vegetation total = %d\n", nsteps, vegies);
        
        // Update totals
        if (vegies == 0) ndied = ndied + 1;
        else if (nsteps >= max_steps) nunsettled = nunsettled + 1;
        else 
        {
            nstable = nstable + 1;
            tot_steps_stable = tot_steps_stable + nsteps;
            tot_veg_stable = tot_veg_stable + vegies;
        }
    }
    
    // Collect results at the master processor
    if (myID == master)
    {
        // Start with the results at the master
        ndied_all = ndied;
        nunsettled_all = nunsettled;
        nstable_all = nstable;
        tot_steps_stable_all = tot_steps_stable;
        tot_veg_stable_all = tot_veg_stable;
        
        // Then add in the results from each of the other processors
        for (int j = 1; j < numProcs; j++)
        {
            MPI::COMM_WORLD.Recv(&ndied, 1, MPI::INTEGER, MPI::ANY_SOURCE, 1, status);
            ndied_all = ndied_all + ndied;
            
            MPI::COMM_WORLD.Recv(&nunsettled, 1, MPI::INTEGER, MPI::ANY_SOURCE, 2, status);
            nunsettled_all = nunsettled_all + nunsettled;
            
            MPI::COMM_WORLD.Recv(&nstable, 1, MPI::INTEGER, MPI::ANY_SOURCE, 3, status);
            nstable_all = nstable_all + nstable;
            
            MPI::COMM_WORLD.Recv(&tot_steps_stable, 1, MPI::INTEGER, MPI::ANY_SOURCE, 4, status);
            tot_steps_stable_all = tot_steps_stable_all + tot_steps_stable;
            
            MPI::COMM_WORLD.Recv(&tot_veg_stable, 1, MPI::INTEGER, MPI::ANY_SOURCE, 5, status);
            tot_veg_stable_all = tot_veg_stable_all + tot_veg_stable;
        }
    }
    // If not the master processor, send results to the master processor
    else
    {
        MPI::COMM_WORLD.Send(&ndied, 1, MPI::INTEGER, master, 1);
        MPI::COMM_WORLD.Send(&nunsettled, 1, MPI::INTEGER, master, 2);
        MPI::COMM_WORLD.Send(&nstable, 1, MPI::INTEGER, master, 3);
        MPI::COMM_WORLD.Send(&tot_steps_stable, 1, MPI::INTEGER, master, 4);
        MPI::COMM_WORLD.Send(&tot_veg_stable, 1, MPI::INTEGER, master, 5); 
    }
       
    // Calculate averages and display results
    if (myID == master)
    {
		if (nstable_all > 0) 
		{
			tot_steps_stable_all = tot_steps_stable_all / nstable_all;
			tot_veg_stable_all = tot_veg_stable_all / nstable_all;
		}
		elapsed_time += MPI_Wtime();
		printf("Percentage which died out: %g%%\n", 100.0*ndied_all/nsims);
		printf("Percentage unsettled:      %g%%\n", 100.0*nunsettled_all/nsims);
		printf("Percentage stablized:      %g%%\n", 100.0*nstable_all/nsims);
		printf("  Of which:\n");
		printf("  Average steps:           %g\n", tot_steps_stable_all);
		printf("  Average vegetation:      %g\n", tot_veg_stable_all);
		printf("Elapsed time: %lf", elapsed_time);
	}
	
	// Shut down MPI
	MPI::Finalize();
    return 0;
}

/**
* Initializes the grid for a game
* 
* @param grid - grid of vegetation values
* @param nx - x dimension of grid 
* @param ny - y dimension of grid 
* @param seed - random number seed 
* @param prob - population probability
*/
void initialize_grid(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int seed, double prob)
{
    int i, j;           /* loop counters */
    int index;          /* unique value for each grid cell */
    int new_seed;       /* unique seed for each grid point */
    double rand1(int *iseed);

    for (i=1; i<=nx; i++) 
    {
        for (j=1; j<=ny; j++) 
        {
            index = ny*i + j;
            new_seed = seed + index;
            if (rand1(&new_seed) > prob) 
                grid[i][j] = 0;
            else 
                grid[i][j] = 1;
        }
    }
}

/**
* Runs a game
* 
* @param grid - grid of vegetation values
* @param nx - x dimension of grid 
* @param ny - y dimension of grid 
* @param max_steps - max # timesteps to simulate
* @param max_unchanged - max # timesteps with no vegetation change
* @param pvegies - amount of stable vegetation
*/
int game_o_life(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int max_steps, 
			int max_unchanged, int *pvegies)
{
    int step;         /* counts the time steps */
    int converged;        /* has the vegetation stablized? */
    int n_unchanged;      /* # timesteps with no vegetation change */
    int old_vegies;       /* previous level of vegetation */
    int old2_vegies;      /* previous level of vegetation */
    int old3_vegies;      /* previous level of vegetation */
    int vegies;       /* total amount of vegetation */
    int neighbors;        /* quantity of neighboring vegetation */
    int temp_grid[MAX_X][MAX_Y];  /* grid to hold updated values */
    int i, j;           /* loop counters */

    step = 1;
    vegies = 1;
    old_vegies = -1;
    old2_vegies = -1;
    old3_vegies = -1;
    n_unchanged = 0;
    converged = 0;

    while (!converged && vegies > 0 && step < max_steps) 
    {
        /* Count the total amount of vegetation. */
        vegies = 0;
        for (i=1; i<=nx; i++) 
        {
            for (j=1; j<=ny; j++) 
            {
                vegies = vegies + grid[i][j];
            }
        }
        if (vegies == old_vegies || vegies == old2_vegies
            || vegies == old3_vegies) 
        {
            n_unchanged = n_unchanged + 1;
            if (n_unchanged >= max_unchanged) 
                converged = 1;
        }
        else 
        {
            n_unchanged = 0;
        }
        old3_vegies = old2_vegies;
        old2_vegies = old_vegies;
        old_vegies = vegies;
    
        if (!converged) 
        {
            /* Copy the sides of the grid to make torus simple. */
            for (i = 1; i <= nx; i++) 
            {
                grid[i][0] = grid[i][ny];
                grid[i][ny+1] = grid[i][1];
            }

            for (j = 0; j <= ny + 1; j++) 
            {
                grid[0][j] = grid[nx][j];
                grid[nx+1][j] = grid[1][j];
            }
    
            /* Now run one time step, putting result in temp_grid. */
            for (i=1; i<=nx; i++) 
            {
                for (j=1; j<=ny; j++) 
                {
                    neighbors = grid[i-1][j-1] + grid[i-1][j] + grid[i-1][j+1]
                              + grid[i][j-1] + grid[i][j+1]
                              + grid[i+1][j-1] + grid[i+1][j] + grid[i+1][j+1];
                    temp_grid[i][j] = grid[i][j];
                    if (neighbors >= 25 || neighbors <= 3) 
                    {
                        temp_grid[i][j] = temp_grid[i][j] - 1;
                        if (temp_grid[i][j] < 0) 
                            temp_grid[i][j] = 0;
                    }
                    else if (neighbors <= 15) 
                    {
                        temp_grid[i][j] = temp_grid[i][j] + 1;
                        if (temp_grid[i][j] > 10) 
                            temp_grid[i][j] = 10;
                    }
                }
            }
    
            /* Now copy temp_grid back to grid. */
            for (i=1; i<=nx; i++) 
            {
                for (j=1; j<=ny; j++) 
                {
                    grid[i][j] = temp_grid[i][j];
                }
            }
            step = step + 1;
        }
    }

    *pvegies = vegies;
    return(step);
}

/**
* Calculates a random number to be used in initializing the grid
* @param iseed - random number seed
*/
double rand1(int *iseed)
{
    double aa=16807.0;
    double mm=2147483647.0;
    double sseed;
    int    jseed;
    int    i;

    jseed = *iseed;
    for (i = 1; i <= 5; i++) 
    {
       sseed = jseed;
       jseed = aa*sseed/mm;
       sseed = aa*sseed - mm*jseed;
       jseed = sseed;
    }

    *iseed = jseed;

    return(sseed/mm);
}
