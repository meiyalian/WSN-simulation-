#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef _USEFULFUNC_H_
#define _USEFULFUNC_H_

#define LOWER 5
#define UPPER 120
#define LISTEN_INTERVAL 150
#define Y_INTERVAL 200
#define TOLERANCE_RANGE 5
#define THRESHOLD 80


extern int random_generator(int lower, int upper);
extern bool is_within_range(int value, int compare, int tolerance);
extern void delay (unsigned int msecs);
extern void generateMACAddress(unsigned char* address );
extern void generateIPAddress(int* address );
#endif