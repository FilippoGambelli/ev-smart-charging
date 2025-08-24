#ifndef DATA_H
#define DATA_H

#define N_FEATURES 12
#define SEQ_LEN     8
#define HORIZON     8

static const float X_min[N_FEATURES] = {
     0.0f,  0.0f,  0.0f,  0.0f,
    -3.0f,  0.0f, -1.0f, -1.0f,
    -0.8660254f, -0.8660254f, -1.0f, -1.0f
};

static const float X_max[N_FEATURES] = {
   974.0f, 437.0f,  20.0f,  70.0f,
    34.0f,  11.0f,   1.0f,   1.0f,
     0.8660254f,  0.8660254f,  1.0f,  1.0f
};

static const float y_min = 0.0f;
static const float y_max = 111750.0f;

#endif /* DATA_H */
