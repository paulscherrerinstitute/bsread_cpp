#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <time.h>

/**
 * @brief time_get same as clock_gettime except that it returns time in seconds
 * @param clock_type
 * @return
 */
double dbltime_get(){
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec + t.tv_nsec/1e9;
}



int main(int argc, char *argv[])
{
    size_t len = strtod(argv[1],0)*1024*1024;
    len = len * sizeof(double);

    double* src = malloc(len);
    double* dest;


    while(1){
        dest = malloc(len*sizeof(double));
        double t = dbltime_get();
        memcpy(src,dest,len);
        t=dbltime_get() - t;
        printf("%4.4f us, %4.4f Gb/s [size: %f Mb]\n",t*1e6,(len/1024.0/1024.0/1024.0)/t,len/1024.0/1024.0);
        free(dest);
        sleep(1);
    }

    return 0;
}
