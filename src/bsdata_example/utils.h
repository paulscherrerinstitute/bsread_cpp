#ifndef UTILS_H
#define UTILS_H

#include <time.h>

/**
 * @brief time_get same as clock_gettime except that it returns time in seconds
 * @param clock_type
 * @return
 */
inline double dbltime_get(int clock_type=CLOCK_REALTIME){
    struct timespec t;
    clock_gettime(clock_type,&t);
    return t.tv_sec + t.tv_nsec/1e9;
}

/**
 * @brief time_nanosleep same as clock_nanosleep, except that it takes argument in seconds
 * @param sec
 * @param clock_type
 */
inline void time_nanosleep(double sec, int clock_type=CLOCK_REALTIME){
    struct timespec t;
    t.tv_sec=int(sec);
    t.tv_nsec= (sec-t.tv_sec)*1e9;

    clock_nanosleep(clock_type,0,&t,0);
}


#include <float.h>
typedef struct dbl_stats{
    double val;
    double min;
    double max;
    double avg;
    int avg_n;
}dbl_stats_t;

inline void dbl_stats_init(dbl_stats_t& stats, int average){
    stats.avg = 0;  // We keep the average the same so we can reuse the function as min/max reset..
    stats.avg_n = average;
    stats.max = DBL_MIN;
    stats.min = DBL_MAX;
    stats.val = 0;
}

inline void dbl_stats_add(dbl_stats_t& stats,double val){
    stats.val = val;
    stats.max = val > stats.max ? val : stats.max;
    stats.min = val < stats.min ? val : stats.min;

    stats.avg -= stats.avg/stats.avg_n;
    stats.avg += val/stats.avg_n;
}

inline void dbl_stats_print(const char* name, dbl_stats_t& stats){
    printf("%10s: %5.5f [%5.5f:%5.5f]\n",name, stats.avg,stats.min,stats.max);
}


#endif // UTILS_H
