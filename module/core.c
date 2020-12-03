#include <stdio.h>
#include "../include/wsn.h"
#include "../include/usefulfunc.h"
#include "../include/base.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>


int main (int argc , char * argv []) {
    int comm_size, glob_rank; //master comm group rank
    int nrows, ncols; // cartesian layout size
    int iteration; // number of iterations for the system
    MPI_Comm sub_comm;

    /* start up initial MPI environment */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &glob_rank);

   
    if (glob_rank == 0){
        // runtime user input m*n nodes
         bool valid_input = false;
         while (!valid_input){
            fflush(stdout);
            printf("Enter row size: ");
            fflush(stdout);
            scanf("%d", &nrows);
            printf("Enter col size: ");
            fflush(stdout);
            scanf("%d", &ncols);

            //if not valid 
            if( (nrows*ncols) != comm_size-1){
                printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols,comm_size-1);
                
            }else{
                valid_input = true;
            }
        }

        printf("number of iterations for the system(before termination): ");
        fflush(stdout);
        scanf("%d", &iteration);

    }
    MPI_Bcast(&nrows, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    MPI_Bcast(&ncols, 1, MPI_INT, 0, MPI_COMM_WORLD); 

     /*************************************************************/
	/* split processes into groups: 1)base station  2) WSN nodes
	/*************************************************************/

    MPI_Comm_split( MPI_COMM_WORLD,glob_rank == 0, 0, &sub_comm);

    
    if (glob_rank != 0){
        /* For WSN nodes */
        wsn_network(MPI_COMM_WORLD, sub_comm, nrows, ncols);
     
    }else{
        /* For base station*/
        base_station( MPI_COMM_WORLD,  nrows,  ncols,  iteration);
    
        
    }


    /* Program clean ups & end */
    MPI_Barrier(MPI_COMM_WORLD);
    if (glob_rank == 0) {
        printf("Program end. \n");fflush(stdout); 
    }
    MPI_Finalize();
    return 0;
}



