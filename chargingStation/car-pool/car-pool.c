#include "car-pool.h"
#include <stdlib.h>
#include <time.h>

// Definition of a pool of realistic cars
static car_t car_pool[] = {
    {22000, 70000, 20, 80, "XY987ZT"},
    {11000, 40000, 10, 50, "AB123CD"},
    {5000, 85000, 30, 90, "ZZ999YY"},
    {700, 35000, 5, 25, "LM456OP"},
    {2200, 60000, 15, 75, "GH789JK"}
};

#define CAR_POOL_SIZE (sizeof(car_pool)/sizeof(car_pool[0]))

const car_t* get_random_car(void) {
    static int initialized = 0;
    if(!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    int index = rand() % CAR_POOL_SIZE;
    return &car_pool[index];
}