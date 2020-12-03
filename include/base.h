#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef _BASE_H_
#define _BASE_H_
#define UPDATE_TIME 500;
extern int base_station(MPI_Comm master_comm, int nrows, int ncols, int iteration);
#endif