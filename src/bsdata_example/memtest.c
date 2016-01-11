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

void time_nanosleep(double sec){
    struct timespec t;
    t.tv_sec=(int)(sec);
    t.tv_nsec= (sec-t.tv_sec)*1e9;

    clock_nanosleep(CLOCK_REALTIME,0,&t,0);
}

int main(int argc, char *argv[])
{
    size_t len = strtod(argv[1],0)*1024*1024;
    len = len * sizeof(double);

    double* src = malloc(len);
    double* dest;


//    size_t max_len = len;

//    len = 128;

    while(1){
        dest = malloc(len*sizeof(double));
        double t = dbltime_get();
        memcpy(src,dest,len);

        t=dbltime_get() - t;
        printf("%4.4f us, %4.4f Gb/s [size: %f Mb]\n",t*1e6,(len/1024.0/1024.0/1024.0)/t,len/1024.0/1024.0);
        free(dest);



//        len = len*2;
//        if(len > max_len){
//            break;
//        }

        time_nanosleep(0.01);
    }

    return 0;
}
