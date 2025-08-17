#ifndef DATA_H
#define DATA_H

#define N_FEATURES 12
#define SEQ_LEN 16
#define HORIZON 16

static const float X_min[N_FEATURES] = {
    0.0f, 0.0f, 0.0f, 0.0f, -3.29f, 0.0f,
   -1.0f, -1.0f, -0.8660254f, -0.8660254f, -1.0f, -1.0f
};

static const float X_max[N_FEATURES] = {
    973.99f, 436.64f, 20.4f, 69.63f, 34.07f, 11.38f,
    1.0f, 1.0f, 0.8660254f, 0.8660254f, 1.0f, 1.0f
};

static const float y_min = 0.0f;
static const float y_max = 111750.0f;

#endif