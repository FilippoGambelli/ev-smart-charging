#include "car-pool.h"
#include <stdlib.h>
#include <time.h>

// Definizione di un “pool” di auto realistiche
static car_t car_pool[] = {
    {22.0, 70, 20.0, 80.0, "XY987ZT"},
    {11.0, 40, 10.0, 50.0, "AB123CD"},
    {50.0, 85, 30.0, 90.0, "ZZ999YY"},
    {7.4, 35, 5.0, 25.0, "LM456OP"},
    {22.0, 60, 15.0, 75.0, "GH789JK"}
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