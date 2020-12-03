ALL: core

core: core.c usefulfunc.c wsn.c base.c
	mpicc -Wall core.c usefulfunc.c wsn.c base.c -o core -lm -pthread

run:	
	mpirun -oversubscribe -np 41 core

clean :
	/bin/rm -f core   
