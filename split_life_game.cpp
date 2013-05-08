/*
Name   : Melissa Steinkamp
Program: Split Game of Life
Class  : CSIS 434 Parallel Computing
Date   : 4/25/12
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_X 500
#define MAX_Y 500

#define STEPS_MAX 200
#define UNCHANGED_MAX 10

/**
* Main program to run a game of Life, split between 4 processors
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
  int   lnx; 				// local x dimension of grid
	int   lny; 				//	local y dimension of grid
    int   max_steps;        /* max # timesteps to simulate */
    int   max_unchanged;    /* max # timesteps with no vegetation change */
    int   vegies;       /* amount of stable vegetation */
    int   nsteps;       /* number of steps actually run */
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
    void  initialize_grid(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int seed, double prob, int id, MPI::Status status);
    int   game_o_life(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int max_steps, 
    		int max_unchanged, int *pvegies, int myID, MPI::Status status);
    
    // Initialize MPI, get rank and size
    MPI::Init();
    numProcs = MPI::COMM_WORLD.Get_size();
    myID = MPI::COMM_WORLD.Get_rank();
    
    ndied = 0;
    nunsettled = 0;
    nstable = 0;
    tot_steps_stable = 0;
    tot_veg_stable = 0;
    
    if (myID == master)
    {
        /* First read in all the parameters. */
        nx = MAX_X+1;
        while (nx > MAX_X || ny > MAX_Y) 
        {
            printf("Enter X and Y dimensions of wilderness: ");
            fflush(stdout);
            scanf("%d%d", &nx, &ny);
        }
    
        printf("Enter population probability: ");
        fflush(stdout);
        scanf("%lf", &prob);
    
        printf("Enter random number seed: ");
        fflush(stdout);
        scanf("%d", &seed0);
        
        
        // calculate local grid dimensions
        lnx = nx / numProcs; 
		lny = ny / numProcs;
        
        // send data to other processors
        for (int j = 1; j < numProcs; j++)
        {
        	MPI::COMM_WORLD.Send(&lnx, 1, MPI::INTEGER, j, 1);
        	MPI::COMM_WORLD.Send(&lny, 1, MPI::INTEGER, j, 2);
        	MPI::COMM_WORLD.Send(&prob, 1, MPI::DOUBLE, j, 3);
        	MPI::COMM_WORLD.Send(&seed0, 1, MPI::INTEGER, j, 4);
        }
    }
    else
    {
    	// Receive data from master
    	MPI::COMM_WORLD.Recv(&lnx, 1, MPI::INTEGER, 0, 1, status);
    	MPI::COMM_WORLD.Recv(&lny, 1, MPI::INTEGER, 0, 2, status);
    	MPI::COMM_WORLD.Recv(&prob, 1, MPI::DOUBLE, 0, 3, status);
    	MPI::COMM_WORLD.Recv(&seed0, 1, MPI::INTEGER, 0, 4, status);
    }
   
	/* Initialize the grid values using the given probability. */
	seed = seed0 * (i + (myID));
	initialize_grid(grid, lnx, lny, seed, prob, myID, status);

	/* Now run the game of life simulation, returning the number of steps. */
	max_steps = STEPS_MAX;
	max_unchanged = UNCHANGED_MAX;
	nsteps = game_o_life(grid, lnx, lny, max_steps, max_unchanged, &vegies, myID, status);

	printf("Number of time steps = %d, Vegetation total = %d\n", nsteps, vegies);
	if (vegies == 0) ndied = ndied + 1;
	else if (nsteps >= max_steps) nunsettled = nunsettled + 1;
	else 
	{
		nstable = nstable + 1;
		tot_steps_stable = tot_steps_stable + nsteps;
		tot_veg_stable = tot_veg_stable + vegies;
	}
    
    // Collect results at process 0
    if (myID == master)
    {
        ndied_all = ndied;
        nunsettled_all = nunsettled;
        nstable_all = nstable;
        tot_steps_stable_all = tot_steps_stable;
        tot_veg_stable_all = tot_veg_stable;
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
		printf("Percentage which died out: %g%%\n", 100.0*ndied_all);
		printf("Percentage unsettled:      %g%%\n", 100.0*nunsettled_all);
		printf("Percentage stablized:      %g%%\n", 100.0*nstable_all);
		printf("  Of which:\n");
		printf("  Average steps:           %g\n", tot_steps_stable_all);
		printf("  Average vegetation:      %g\n", tot_veg_stable_all);
	}
	
	// Shut down MPI
	MPI::Finalize();
    return 0;
}

/**
* initializes the grid for a game
*/
void initialize_grid(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int seed, double prob, 
			int id, MPI::Status status)
/*    int   grid[MAX_X+2][MAX_Y+2];   /* grid of vegetation values
    int   nx;           /* x dimension of local grid 
    int   ny;           /* y dimension of local grid 
    int   seed;         /* random number seed 
    double prob;            /* population probability */
    {
        int i, j;           /* loop counters */
        int iStart, jStart;	// starting values for i and j
        int iLim, jLim; 	// limits for loop counters based on proc id
        int index;          /* unique value for each grid cell */
        int new_seed;       /* unique seed for each grid point */
        double rand1(int *iseed);
        
        // set i,j and limits based on proc id
        if (id == 1)
        {
        	iStart = 1;
        	jStart = 1;
        	iLim = nx / 2;
        	jLim = ny / 2;
        }
        else if (id == 2)
        {
        	iStart = nx/2 + 1;
        	jStart = 1;
        	iLim = nx;
        	jLim = ny / 2;
        }
        else if (id == 3)
        {
        	iStart = 1;
        	jStart = ny/2 + 1;
        	iLim = nx / 2;
        	jLim = ny;
        }
        else if (id == 0)
        {
        	iStart = nx/2 + 1;
        	jStart = ny/2 + 1;
        	iLim = nx;
        	jLim = ny;
        }
    
        for (i=iStart; i<=iLim; i++) 
        {
            for (j=jStart; j<=jLim; j++) 
            {
                index = ny*i + j;
                new_seed = seed + index;
                if (rand1(&new_seed) > prob) grid[i][j] = 0;
                else grid[i][j] = 1;
            }
        }
    }

/**
* Runs a game
*/
int game_o_life(int grid[MAX_X+2][MAX_Y+2], int nx, int ny, int max_steps, 
			int max_unchanged, int *pvegies, int myID, MPI::Status status)
{
    int   step;         /* counts the time steps */
    int   converged;        /* has the vegetation stablized? */
    int   tempConverged;	
    int   allConverged;
    int   n_unchanged;      /* # timesteps with no vegetation change */
    int   old_vegies;       /* previous level of vegetation */
    int   old2_vegies;      /* previous level of vegetation */
    int   old3_vegies;      /* previous level of vegetation */
    int   vegies;       /* total amount of vegetation */
    int   neighbors;        /* quantity of neighboring vegetation */
    int   temp_grid[MAX_X][MAX_Y];  /* grid to hold updated values */
    int i, j;           /* loop counters */
    int verticalNeighbor;
	int horizontalNeighbor;
	int cornerNeighbor;
	
	// values of corners for sending
	int c1;
	int c2;
	int c3;
	int c4;
	
	// corner values for receiving
	int rc1;
	int rc2;
	int rc3;
	int rc4;
	
	// sides for sending
	int top[nx];
	int bottom[nx];
	int left[ny];
	int right[ny];
	
	// sides to be received
	int top2[nx];
	int bottom2[nx];
	int left2[ny];
	int right2[ny];
    
    // determine neighbors (based on np = 4)
	verticalNeighbor = (myID + 2) % 4;
	if (myID == 1)
	{
		horizontalNeighbor = 2;
		cornerNeighbor = 0;
	}
	else if (myID == 2)
	{
		horizontalNeighbor = 1;
		cornerNeighbor = 3;
	}
	else if (myID == 3)
	{
		horizontalNeighbor = 0;
		cornerNeighbor = 1;
	}
	else if (myID == 0)
	{
		horizontalNeighbor = 3;
		cornerNeighbor = 2;
	}

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
        for (i=1; i<=nx; i++) {
            for (j=1; j<=ny; j++) {
                vegies = vegies + grid[i][j];
            }
        }
        if (vegies == old_vegies || vegies == old2_vegies
            || vegies == old3_vegies) {
            n_unchanged = n_unchanged + 1;
            if (n_unchanged >= max_unchanged) converged = 1;
        }
        else {
            n_unchanged = 0;
        }
        old3_vegies = old2_vegies;
        old2_vegies = old_vegies;
        old_vegies = vegies;
        
        // send converged to master, recv converged flag for whole grid
        if (myID != 0)
        {
	        MPI::COMM_WORLD.Send(&converged, 1, MPI::INTEGER, 0, 10);
	        MPI::COMM_WORLD.Recv(&allConverged, 1, MPI::INTEGER, 0, 11, status);
	        converged = allConverged;
	    }
	    else
	    {
	    	// master checks if all procs' grids have converged
	    	if (converged)
	    	{
				for (int n = 1; n < 4; n++)
				{
					MPI::COMM_WORLD.Recv(&tempConverged, 1, MPI::INTEGER, MPI::ANY_SOURCE, 10, status);
					converged += tempConverged;
				}
			}
			if (converged == 4)
			{
				converged = 1;
			}
			else
			{
				converged = 0;
			}
			
			// tell other procs if everything's converged
			for (int n = 1; n < 4; n++)
			{
				MPI::COMM_WORLD.Send(&converged, 1, MPI::INTEGER, n, 11);
			}
        }
    
        if (!converged) 
        {
			// fill arrays for each side to be sent (copy from grid)
			for (i = 1; i <= nx; i++)
			{
				top[i] = grid[i][1];
				bottom[i] = grid[i][ny];
			}
			for (i = 1; i <= ny; i++)
			{
				left[i] = grid[1][i];
				right[i] = grid[nx][i];
			}
			
			// copy corner values
			c1 = grid[1][1];
			c2 = grid[nx][1];
			c3 = grid[1][ny];
			c4 = grid[nx][ny];
			
			// sends
			MPI::COMM_WORLD.Send(&top, nx, MPI::INTEGER, verticalNeighbor, 1);
			MPI::COMM_WORLD.Send(&bottom, nx, MPI::INTEGER, verticalNeighbor, 2);
			MPI::COMM_WORLD.Send(&left, ny, MPI::INTEGER, horizontalNeighbor, 3);
			MPI::COMM_WORLD.Send(&right, ny, MPI::INTEGER, horizontalNeighbor, 4);
			MPI::COMM_WORLD.Send(&c1, 1, MPI::INTEGER, cornerNeighbor, 5);
			MPI::COMM_WORLD.Send(&c2, 1, MPI::INTEGER, cornerNeighbor, 6);
			MPI::COMM_WORLD.Send(&c3, 1, MPI::INTEGER, cornerNeighbor, 7);
			MPI::COMM_WORLD.Send(&c4, 1, MPI::INTEGER, cornerNeighbor, 8);
			
			// receives
			MPI::COMM_WORLD.Recv(&bottom2, nx, MPI::INTEGER, verticalNeighbor, 1, status);
			MPI::COMM_WORLD.Recv(&top2, nx, MPI::INTEGER, verticalNeighbor, 2, status);
			MPI::COMM_WORLD.Recv(&right2, ny, MPI::INTEGER, horizontalNeighbor, 3, status);
			MPI::COMM_WORLD.Recv(&left2, ny, MPI::INTEGER, horizontalNeighbor, 4, status);
			MPI::COMM_WORLD.Recv(&rc1, 1, MPI::INTEGER, cornerNeighbor, 5, status);
			MPI::COMM_WORLD.Recv(&rc2, 1, MPI::INTEGER, cornerNeighbor, 6, status);
			MPI::COMM_WORLD.Recv(&rc3, 1, MPI::INTEGER, cornerNeighbor, 7, status);
			MPI::COMM_WORLD.Recv(&rc4, 1, MPI::INTEGER, cornerNeighbor, 8, status);

			/* Now run one time step, putting result in temp_grid. */
            for (i=1; i<=nx; i++) {
                for (j=1; j<=ny; j++) {
                    neighbors = grid[i-1][j-1] + grid[i-1][j] + grid[i-1][j+1]
                              + grid[i][j-1] + grid[i][j+1]
                              + grid[i+1][j-1] + grid[i+1][j] + grid[i+1][j+1];
                    temp_grid[i][j] = grid[i][j];
                    if (neighbors >= 25 || neighbors <= 3) {
                        temp_grid[i][j] = temp_grid[i][j] - 1;
                        if (temp_grid[i][j] < 0) temp_grid[i][j] = 0;
                    }
                    else if (neighbors <= 15) {
                        temp_grid[i][j] = temp_grid[i][j] + 1;
                        if (temp_grid[i][j] > 10) temp_grid[i][j] = 10;
                    }
                }
            }
    
            /* Now copy temp_grid back to grid. */
            for (i=1; i<=nx; i++) {
                for (j=1; j<=ny; j++) {
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
*/
double    rand1(int *iseed)
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
