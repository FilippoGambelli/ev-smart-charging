#include "car-pool.h"
#include <stdlib.h>
#include <time.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

// Definition of a pool of realistic cars
static car_t car_pool[] = {
    {22.0000f, 70.0000f, 20.00f, 80.00f, "XY987ZT"},
    {22.0000f, 85.0000f, 89.50f, 90.00f, "ZZ999YY"},
    {11.0000f, 40.0000f, 10.00f, 50.00f, "AB123CD"}
};

#define CAR_POOL_SIZE (sizeof(car_pool)/sizeof(car_pool[0]))

// Returns a random car from the pool
const car_t* get_random_car(void) {
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    int index = rand() % CAR_POOL_SIZE;
    return &car_pool[index];
}

// Returns the next car in the pool (cyclic)
const car_t* get_next_car(uint8_t id) {
    int index = (id - 1) % CAR_POOL_SIZE;
    return &car_pool[index];
}