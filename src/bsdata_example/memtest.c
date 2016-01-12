#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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


void dbl_avg_roll(double* avg, double input,int N) {
    *avg -= *avg/N;
    *avg += input/N;
}

void memtest(size_t len,int loops){
    uint8_t* dest = malloc(len);
    uint8_t* src = malloc(len);

    double t_avg=0;
    for(int i=0;i<loops;i++){
	    double t = dbltime_get();

	    // memcpy(src,dest,len);
        memcpy(src,dest,len);

	    t=dbltime_get() - t;
        dbl_avg_roll(&t_avg,t,loops/4);
        // t_avg=t;        
    }
    
    printf("%4.4f us, %4.4f Gb/s [size: %f Mb]\n",t_avg*1e6,(len/1024.0/1024.0/1024.0)/t_avg,len/1024.0/1024.0);

    free(dest);	
    free(src);    
}

void memtest_thread(void* arg){
    size_t len = *((size_t*)(arg));
    while(1){
        memtest(len,10);
    }
}

int main(int argc, char *argv[])
{
    size_t len = strtod(argv[1],0)*1024*1024;
    int no_threads = strtol(argv[2],0,10);


    for(int i=0;i<no_threads;i++){
        pthread_t* t1 = malloc(sizeof(pthread_t));
        pthread_create(t1, 0, memtest_thread, &len);
    }



    while(1){
         time_nanosleep(1);
    }

    return 0;
}
