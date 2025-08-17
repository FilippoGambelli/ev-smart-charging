#include "normalization.h"

float normalize(float val, float min_val, float max_val) {
    return (val - min_val) / (max_val - min_val);
}

float denormalize(float val, float min_val, float max_val) {
    return val * (max_val - min_val) + min_val;
}