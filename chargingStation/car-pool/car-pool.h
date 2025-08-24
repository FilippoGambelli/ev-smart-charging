#ifndef CAR_POOL_H
#define CAR_POOL_H

#include <stdint.h>

typedef struct {
    float carMaxPower;      // in kW
    float carCapacity;      // in kWh
    float currentCharge;    // in kWh
    float desiredCharge;    // in kWh
    const char* plate;      // license plate
} car_t;

// Returns a random car from the pool
const car_t* get_random_car(void);

// Returns the next car in the pool (cyclic)
const car_t* get_next_car(uint8_t id);

#endif // CAR_POOL_H