#include <stdio.h>
#include "../include/usefulfunc.h"
#include "../include/wsn.h"
#include "../include/base.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define PACK_SIZE 100
#define DELAY_TIME 200
#define DETECT_INTERVAL 1000
#define SA_UPDATE_TIME 1500


void *satelliteUpdate(void *args);

typedef struct {
    time_t timestamp;
    int reading;
} satellite;  


typedef struct {
    char IP[20];
    char MAC[20];
    int coord[2];
} node;  

int row;
int col;
int terminate_signal=0;
int totalAlarm = 0;
int correctAlarm = 0;

void *satelliteUpdate(void* args){
    srand(time(NULL));
    satellite *data = args;

    while (! terminate_signal){
        for(int i=0;i<row;i++){
            for(int j = 0; j< col; j++){
                int index = i*col +j;
                time_t ts = time(NULL);
                char * time_str = ctime(&ts);
                time_str[strlen(time_str)-1] = '\0';

                int new_reading = random_generator(LOWER,UPPER);
                // printf("thread node %d reading %d time %s, \n", index, new_reading, time_str);
                data[index].timestamp = ts;
                data[index].reading = new_reading;
            }
        }

        delay(SA_UPDATE_TIME);
    }
    printf("threading end.\n");

} 

/********************************************************************
 * 
 * description:
 * This method is used for logging in each iteration of the base station. 
 * It unpack the msg sent bbyb the WSN nodes, format & log the information in to a txt file. 
 * Parameters:

 *     FILE *  pFile             - pointer of the file obj
 *     char *  buffer            - msg received from the WSN node
 *     MPI_Comm master_comm      - comm group of the base station & nodes
 *     int     iteration         - current iteration of the base station
 *     time_t  recev_time        - time when recv the msg
 *     node    nodes             - information of each node
 * 
 * returns: None
 ************************************************************************/
void logToFile(FILE *pFile, char *buffer,MPI_Comm master_comm, int iteration, time_t recev_time, node *nodes, satellite* data)
{
    printf("start logging\n");
    int packstart = 0; 
    int reported_node_rank;
    long int node_time;
    unsigned char reported_MAC[6]; //mac address 
    int reported_IP[4]; // IP address
    int reading, neighbour_count; 

    

    char * recv_time_str = ctime(&recev_time);
    recv_time_str[strlen(recv_time_str)-1] = '\0';


    MPI_Unpack(buffer, PACK_SIZE, &packstart, &reported_node_rank, 1, MPI_INT, master_comm);

    time_t sa_time = data[reported_node_rank].timestamp;
    // char * sa_time_str = ctime(&sa_time);
    // sa_time_str[strlen(sa_time_str)-1] = '\0';
    int isTrueAlarm = 0;
    int sa_reading = data[reported_node_rank].reading;
    // printf("time here is %s\n",sa_time_str );


    MPI_Unpack(buffer, PACK_SIZE, &packstart, &reading, 1, MPI_INT, master_comm);
    MPI_Unpack(buffer, PACK_SIZE, &packstart, &neighbour_count, 1, MPI_INT, master_comm);

    if(sa_reading >= (THRESHOLD-TOLERANCE_RANGE) ){
        isTrueAlarm = 1;
        correctAlarm+=1;

    }

    int reported_neighbours[neighbour_count];
    int reported_neighbours_val[neighbour_count];

    for(int i = 0; i < neighbour_count; i ++){
        MPI_Unpack(buffer, PACK_SIZE, &packstart, &reported_neighbours[i], 1, MPI_INT, master_comm);
        MPI_Unpack(buffer, PACK_SIZE, &packstart, &reported_neighbours_val[i], 1, MPI_INT, master_comm);
    }

    MPI_Unpack(buffer, PACK_SIZE, &packstart, &node_time, 1, MPI_LONG, master_comm);


    time_t t_time = (time_t) node_time;
    char * time_str = ctime(&t_time);
    time_str[strlen(time_str)-1] = '\0';
  
   

    time_t log_time = time(NULL);
    char * log_str = ctime(&log_time);
    log_str[strlen(log_str)-1] = '\0';

    fprintf(pFile, "------------------------------------------------------\n");
    fprintf(pFile, "Iteration: %d\n", iteration);
    fprintf(pFile, "Logged Time: %s\n", log_str);
    fprintf(pFile, "Alert Reported Time: %s\n", recv_time_str);
    fprintf(pFile, "Alert type (0: false ,1: true) %d\n\n\n", isTrueAlarm);
    fprintf(pFile, "Reporting Node 	   Coord		Temp		MAC 			IP\n");
    fprintf(pFile, "%d                (%d,%d)                 %d         %s                %s    \n\n", reported_node_rank, nodes[reported_node_rank].coord[0],nodes[reported_node_rank].coord[1], reading,nodes[reported_node_rank].MAC,nodes[reported_node_rank].IP );
    fprintf(pFile, "Adjacent Nodes	   Coord		Temp		MAC 			IP\n");

     for(int i = 0; i < neighbour_count; i ++){
        int neighbour_node = reported_neighbours[i];
        int neighbour_reading = reported_neighbours_val[i];
        fprintf(pFile, "%d                (%d,%d)             %d         %s                %s    \n", neighbour_node, nodes[neighbour_node].coord[0],nodes[neighbour_node].coord[1], neighbour_reading,nodes[neighbour_node].MAC,nodes[neighbour_node].IP );
    }

    char * sa_time_str = ctime(&sa_time);
    sa_time_str[strlen(sa_time_str)-1] = '\0';
    fprintf(pFile,"\nInfrared Satellite Reporting Time : %s\n",sa_time_str);
    fprintf(pFile, "Infrared Satellite Reporting node %d's (Celsius) : %d, coord (%d,%d) \n", reported_node_rank, sa_reading,nodes[reported_node_rank].coord[0],nodes[reported_node_rank].coord[1]);

    fprintf(pFile,"Communication Time (seconds): %f\n", difftime(recev_time,t_time));                                  
    fprintf(pFile, "Total Messages send between reporting node and base station: 1\n");
    fprintf(pFile, "Number of adjacent matches to reporting node:  %d\n", neighbour_count);

	
}


int base_station(MPI_Comm master_comm, int nrows, int ncols, int iteration){

    satellite satellite_data[nrows*ncols];
    int comm_size;
    MPI_Comm_size(master_comm, &comm_size);

    node nodes_info[comm_size-1];

    pthread_t tid;

    row = nrows;
    col = ncols;
    int create = pthread_create(&tid, NULL, satelliteUpdate, satellite_data);
  
    //error handling 
    if (create) {
        printf("ERROR; return code from pthread_create() is %d\n", create);
        exit(-1);
    }


    /*create log file name */
    char * filename = (char *) malloc(15);
    strcpy(filename, "station_log.txt");
    FILE *pFile = fopen(filename, "w");

    /*recv information from each node */
    MPI_Status status ;
    for (int i = 0; i < comm_size-1; i++){
        MPI_Recv (  nodes_info[i].MAC, 20 , MPI_CHAR , i+1 ,0 , master_comm , &status ) ;
        MPI_Recv (  nodes_info[i].IP, 20 , MPI_CHAR , i+1 ,0 , master_comm , &status ) ;
        MPI_Recv (  nodes_info[i].coord, 2 , MPI_INT , i+1 ,0 , master_comm , &status ) ;

    }

    
    MPI_Request alarm_req;
    MPI_Status status_alarm;

    /*iteration starts */
    for (int k = 0; k< iteration; k++){
        delay(DELAY_TIME);
        clock_t beginTime = clock();
        char buffer[PACK_SIZE];
        int isRecv;

        double total_time = (clock() - beginTime)*1000/CLOCKS_PER_SEC;

         /*listen to alarms within each time window*/
        while( total_time < DETECT_INTERVAL){
            MPI_Irecv (buffer, PACK_SIZE, MPI_PACKED, MPI_ANY_SOURCE, MSG_SENDINFO, master_comm, &alarm_req);
            MPI_Test(&alarm_req, &isRecv, &status_alarm);


            clock_t detectLoopStart = clock();
            while( total_time < DETECT_INTERVAL && !isRecv ){
                MPI_Test(&alarm_req, &isRecv, &status_alarm);
                total_time +=  (clock() - detectLoopStart)*1000/CLOCKS_PER_SEC;
        
            }
            if(! isRecv){
                MPI_Cancel(&alarm_req);
            }
            else{
                 /*log the info if receives an alarm*/
                int logStart = clock();
                time_t recev_time = time(NULL);
                logToFile(pFile, buffer, master_comm, k, recev_time,nodes_info, satellite_data);
                isRecv = 0;
                total_time +=  (clock() - logStart)*1000/CLOCKS_PER_SEC;
                totalAlarm +=1;

                
            }
        }
        
    }

     /*log summary*/
    fprintf(pFile, "--------------------------SUMMARY----------------------------\n");
    fprintf(pFile, "Total communications: %d\n", totalAlarm);
    fprintf(pFile, "Correct alarms: %d\n", correctAlarm);
    fprintf(pFile, "Correct rate: %f\n", (float)correctAlarm/totalAlarm);



    fclose(pFile);
    

    printf("start to send termination signal...\n");
    /* after iterations, send termination msg to each node*/
    terminate_signal = 1;
    int quit = 1;
    
    MPI_Request send_request[comm_size-1];
    for (int i = 1; i < comm_size; i++){
        MPI_Send ( &quit ,1 , MPI_INT , i , MSG_TERMINATE ,master_comm) ;
    }

//           rc = pthread_join(thread[t], &status);
//        if (rc) {
//           printf("ERROR; return code from pthread_join() is %d\n", rc);
//           exit(-1);
//           }
//        printf("Main: completed join with thread %ld having a status   
//              of %ld\n",t,(long)status);
//        }
 
//  printf("Main: program completed. Exiting.\n");
//  pthread_exit(NULL);
    
    
    /*clean ups*/

    pthread_join(tid, NULL);
    free(filename);
    return 0;
}




