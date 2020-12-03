#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/usefulfunc.h"
#include <sys/timeb.h>
#include "../include/wsn.h"

// constant variables for cart shift 
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
// define dimention of the layout
#define DIMENTION 2

// #define LISTEN_INTERVAL 100
#define WARNING_COUNT 2 
#define MAX_NEIGHBOURS 4
// #define Y_INTERVAL 300

#define PACK_SIZE 100



int wsn_network(MPI_Comm master_comm, MPI_Comm sub_comm, int nrows, int ncols){

    MPI_Comm cart_comm;
    int network_size, sub_rank;
    int reorder, my_cart_rank, ierr; //for cart create 
    int layout[DIMENTION],coord[DIMENTION];
    int wrap_around[DIMENTION];
    int neighbours[4]; // n[0]: left, n[1]: right , n[2]: top, n[3]: bott
    int neighbours_vals[4] = {-1,-1,-1,-1};

    int isRequest; 
    unsigned char address[6]; //mac address 
    int IP[4]; // IP address


    
    /* get the size & each process's rank of network */
    MPI_Comm_size(sub_comm, &network_size);
    MPI_Comm_rank(sub_comm, &sub_rank);

    /* create cartesian topology for WSN network */
    layout[0] = nrows;
    layout[1] = ncols;
    MPI_Dims_create(network_size, DIMENTION, layout);

    /* create cartesian mapping */

    /* periodic shift is .false. */
	wrap_around[0] = 0;
	wrap_around[1] = 0; 
    /* allow MPI to determine optimal process ordering  */
    reorder = 1;
	ierr =0;
	ierr = MPI_Cart_create(sub_comm, DIMENTION, layout, wrap_around, reorder, &cart_comm);
     /* error handling for creating  cartesian*/
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr); 

    /* find my coordinates in the cartesian communicator group */
    MPI_Cart_coords(cart_comm, sub_rank, DIMENTION, coord); 

    /* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(cart_comm, coord, &my_cart_rank);

    /* get neighbor nodes */
    MPI_Cart_shift( cart_comm, SHIFT_ROW, DISP, &neighbours[2], &neighbours[3]);
    MPI_Cart_shift( cart_comm, SHIFT_COL, DISP, &neighbours[0], &neighbours[1]);

    /* unique seed for each node */
    srand(((unsigned)time( NULL )+my_cart_rank)% 65535); 

     /* generate a  MAC address for simulation purpose*/
    generateMACAddress(address);
    generateIPAddress(IP);

    /*send basic information about the node to the base station */ 
    char mac_buffer[20];
    snprintf(mac_buffer, 20, "%02X:%02X:%02X:%02X:%02X:%02X", address[0], address[1],address[2],address[3],address[4],address[5]);

    char ip_buffer[20];
    snprintf(ip_buffer, 20, "%d.%d.%d.%d", IP[0], IP[1],IP[2],IP[3]);

    MPI_Send ( mac_buffer , strlen ( mac_buffer ) + 1 , MPI_CHAR , 0 , 0 ,master_comm );
    MPI_Send ( ip_buffer , strlen ( ip_buffer ) + 1 , MPI_CHAR , 0 , 0 ,master_comm );
    MPI_Send ( coord , 2 , MPI_INT , 0 , 0 ,master_comm );


    int recvdata, isTerminate=0, aquire = 1 ;
    MPI_Request handle_termininate;
    MPI_Status status_terminate, receive_status[MAX_NEIGHBOURS];
    MPI_Request send_request[MAX_NEIGHBOURS];
    MPI_Request recv_request[MAX_NEIGHBOURS];

    MPI_Irecv(&recvdata, 1, MPI_INT, 0, MSG_TERMINATE, master_comm, &handle_termininate);
    MPI_Test(&handle_termininate, &isTerminate, &status_terminate);
    while (!isTerminate ){
        clock_t begintime=clock(); 
        int reading = random_generator(LOWER, UPPER);
        
        
        // printf("totile time=%f\n",(float)(clock() - begintime)*1000/CLOCKS_PER_SEC);


        MPI_Request receive_request[MAX_NEIGHBOURS];

        if (reading > THRESHOLD){
        
            for(int i=0; i<MAX_NEIGHBOURS; i++){
                /* send request to neighbour nodes */
                MPI_Isend(&aquire, 1, MPI_INT, neighbours[i], MSG_AQUIRED, cart_comm, &send_request[i]);
                MPI_Irecv (&neighbours_vals[i] , 1 , MPI_INT,  neighbours[i] , MSG_SENDINFO ,cart_comm , &recv_request[i] ) ;
            }
        }
        MPI_Request request_aquire;
        MPI_Status status_recv;
        int isReceived = 0;


        MPI_Irecv ( &isRequest , 1 , MPI_INT,  MPI_ANY_SOURCE , MSG_AQUIRED ,cart_comm , &request_aquire ) ;
        while ( (clock() - begintime)*1000/CLOCKS_PER_SEC < LISTEN_INTERVAL){
            
            MPI_Test(&request_aquire, &isReceived, &status_recv);
            if (isReceived){ 

                MPI_Request send_req;
                if (status_recv.MPI_SOURCE == neighbours[0] ||status_recv.MPI_SOURCE == neighbours[1]  || status_recv.MPI_SOURCE == neighbours[2] ||status_recv.MPI_SOURCE == neighbours[3] ){
                    MPI_Isend(&reading, 1, MPI_INT, status_recv.MPI_SOURCE,MSG_SENDINFO, cart_comm, &send_req );
                 
                }
                
                MPI_Irecv (&isRequest ,1, MPI_INT,  MPI_ANY_SOURCE , MSG_AQUIRED ,cart_comm , &request_aquire ) ;
             
            }
            
        }
        MPI_Cancel(&request_aquire);

        // if (recevnum >= 2){
        //     printf("node%d recev %d nei\n", my_cart_rank, recevnum);
        // }
        
    
        //&& clock()-begintime < Y_INTERVAL
        if (reading > THRESHOLD){
            /*wait for neighbour's msg*/

            // MPI_Waitall(MAX_NEIGHBOURS, recv_request, receive_status); 
            // printf("finished waithing from node %d\n", my_cart_rank);

            int count = 0; 
            for (int i = 0; i< 4; i++){
                if ( neighbours_vals[i]>= (reading - TOLERANCE_RANGE)){
                    count +=1;
                }
            }


            if (count>=WARNING_COUNT){

                /*send msg to base station */
                  /*time + report node + reported value + number of nodes that triggerred warnings + left/right/up/down neighbours + values */
                char msg_pack[PACK_SIZE];
                int position = 0;
                // long int time = (long int)generate_time;

                // time_t t_time = (time_t) time;
         
                
                MPI_Pack(&my_cart_rank, 1, MPI_INT, msg_pack, PACK_SIZE, &position, master_comm);
                MPI_Pack(&reading, 1, MPI_INT, msg_pack, PACK_SIZE, &position, master_comm);
                MPI_Pack(&count, 1, MPI_INT, msg_pack, PACK_SIZE, &position, master_comm); 
                for (int i = 0; i < 4 ; i++){
                    if (neighbours_vals[i]>= (reading - TOLERANCE_RANGE)){
                         MPI_Pack(&neighbours[i], 1, MPI_INT, msg_pack, PACK_SIZE, &position, master_comm);
                         MPI_Pack(&neighbours_vals[i], 1, MPI_INT, msg_pack, PACK_SIZE, &position, master_comm);
                    }
                }
                MPI_Request sent_to_base;
                MPI_Status sta;
                time_t generate_time = time(NULL);
                MPI_Pack(&generate_time, 1, MPI_LONG, msg_pack, PACK_SIZE, &position, master_comm);

                MPI_Isend(msg_pack, position, MPI_PACKED, 0, MSG_SENDINFO, master_comm, &sent_to_base);
                MPI_Wait(&sent_to_base, &sta);
                printf("node %d send to base\n", my_cart_rank);


            }

        }
        
        MPI_Test(&handle_termininate, &isTerminate, &status_terminate);
        
        int remaining_time = Y_INTERVAL - (int)((clock() - begintime)*1000/CLOCKS_PER_SEC);
        if (remaining_time > 0){
            delay(remaining_time);
        }
    }
    
    printf("node %d terminated.\n" , sub_rank);


    /* clean ups & return */
    
    MPI_Comm_free( &cart_comm );
    return 0;
}

