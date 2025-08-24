#ifndef CAR_POOL_H
#define CAR_POOL_H

#include <stdint.h>

typedef struct {
    int carMaxPower;    // in W
    unsigned int carCapacity;   // in Wh
    int currentCharge;
    int desiredCharge;
    const char* plate;
} car_t;

const car_t* get_random_car(void);

#endif // CAR_POOL_H
