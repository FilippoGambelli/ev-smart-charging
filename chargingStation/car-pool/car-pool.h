#ifndef CAR_POOL_H
#define CAR_POOL_H

#include <stdint.h>

typedef struct {
    double carMaxKW;
    unsigned int carCapacity;
    double currentCharge;
    double desiredCharge;
    const char* plate;
} car_t;

// Restituisce un puntatore a un auto casuale
const car_t* get_random_car(void);

#endif // CAR_POOL_H
