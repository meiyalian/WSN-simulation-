#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>


#ifndef _WSN_H_
#define _WSN_H_

#define LOWER 5
#define MSG_TERMINATE 3
#define MSG_AQUIRED 2
#define MSG_SENDINFO 4
extern int wsn_network(MPI_Comm, MPI_Comm, int, int);
#endif