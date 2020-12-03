#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>




/********************************************************************
 * 
 * description:
 * This method is used for generating random integers within the range given 

 *     int  lower             - lower bound of the int
 *     int  higher            - upper bound of the int

 * 
 * returns: random int 
 ************************************************************************/
int random_generator(int lower, int upper){
    return (rand() % (upper - lower + 1))+ lower;
}


/********************************************************************
 * 
 * description:
 * This method is used for delay a program (thread) by clock()
 * 
 *     int  msecs             - milliseconds to delay
 * 
 * returns: None
 ************************************************************************/
void delay (unsigned int msecs) {
    clock_t goal = msecs*CLOCKS_PER_SEC/1000 + clock();  //convert msecs to clock count  
    while ( goal > clock() );               // Loop until it arrives.

}

/********************************************************************
 * 
 * description:
 * This method is used for generating a random MAC address
 * 
 *     unsigned char* address            - pointer to store the MAC address 
 * 
 * returns: None
 ************************************************************************/

void generateMACAddress(unsigned char* address ){
    int i;
    for(i = 0; i< 6; i++){
        address[i] = rand()&0xFF;
    }
}

/********************************************************************
 * 
 * description:
 * This method is used for generating a random MAC address
 * 
 *     unsigned int* address            - pointer to store the IP address 
 * 
 * returns: None
 ************************************************************************/

void generateIPAddress(int* address ){
    int i;
    for(i = 0; i< 4; i++){
        address[i] = random_generator(0,255);
    }
}



bool is_within_range(int value, int compare, int tolerance){
    return value <= compare+tolerance && value >= compare-tolerance;
}