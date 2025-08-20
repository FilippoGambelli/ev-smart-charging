#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "eml_net.h"
#include "coap-engine.h"
#include "real-data/solar-data.h"
#include "emlearn-utils/data.h"
#include "emlearn-utils/normalization.h"
#include "emlearnModel.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_pred_power_obs,
               "title=\"Predicted Power Data\";rt=\"text/plain\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

/* Event handler is called when the resource state changes*/
static void res_event_handler(void) {
  // Notify all observers that data has changed
  coap_notify_observers(&res_pred_power_obs);
}

/* GET handler for the predicted power resource */
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    float features[SEQ_LEN * N_FEATURES]; // Array to hold normalized input features for the model
    float outputs[HORIZON];               // Array to store the model's predictions

    time_t t;

    // Create normalized features for the prediction sequence
    for (int i = 0; i < SEQ_LEN; i++) {
        int idx = solar_data_counter - SEQ_LEN + 1 + i; // Index into historical solar data

        t = solar_data_timestamp[idx]; // Get the timestamp for the current index
        struct tm *tm_info = localtime(&t); // Convert timestamp to local time structure

        // Extract hour, minute, and week-of-year (approximate)
        int hour   = tm_info->tm_hour;
        int minute = tm_info->tm_min;
        int woy    = tm_info->tm_yday / 7; // Week of year (0-51)

        // Convert time features to angles for circular encoding
        float hour_angle = 2.0f * M_PI * hour / 24.0f;
        float min_angle  = 2.0f * M_PI * minute / 60.0f;
        float woy_angle  = 2.0f * M_PI * woy / 52.0f;

        // Compute sine and cosine for circular encoding of time features
        float hour_sin = sinf(hour_angle);
        float hour_cos = cosf(hour_angle);
        float min_sin  = sinf(min_angle);
        float min_cos  = cosf(min_angle);
        float woy_sin  = sinf(woy_angle);
        float woy_cos  = cosf(woy_angle);

        // Normalize input features using precomputed min/max values
        features[i * N_FEATURES + 0]  = normalize(Gb[idx],   X_min[0], X_max[0]);
        features[i * N_FEATURES + 1]  = normalize(Gd[idx],   X_min[1], X_max[1]);
        features[i * N_FEATURES + 2]  = normalize(Gr[idx],   X_min[2], X_max[2]);
        features[i * N_FEATURES + 3]  = normalize(HSun[idx], X_min[3], X_max[3]);
        features[i * N_FEATURES + 4]  = normalize(T[idx],    X_min[4], X_max[4]);
        features[i * N_FEATURES + 5]  = normalize(WS[idx],   X_min[5], X_max[5]);
        features[i * N_FEATURES + 6]  = normalize(hour_sin,  X_min[6], X_max[6]);
        features[i * N_FEATURES + 7]  = normalize(hour_cos,  X_min[7], X_max[7]);
        features[i * N_FEATURES + 8]  = normalize(min_sin,   X_min[8], X_max[8]);
        features[i * N_FEATURES + 9]  = normalize(min_cos,   X_min[9], X_max[9]);
        features[i * N_FEATURES + 10] = normalize(woy_sin,   X_min[10], X_max[10]);
        features[i * N_FEATURES + 11] = normalize(woy_cos,   X_min[11], X_max[11]);
    }

    // Predict using the emlearn TinyML model
    eml_net_predict_proba(&emlearnModel, features, SEQ_LEN * N_FEATURES, outputs, HORIZON);

    // Denormalize the model outputs
    for (int i = 0; i < HORIZON; i++) {
        outputs[i] = denormalize(outputs[i], y_min, y_max);
    }

    // Compute the trend using weighted differences between consecutive predictions
    float weights[HORIZON-1] = {0.35f, 0.25f, 0.15f, 0.10f, 0.08f, 0.05f, 0.02f};
    float trend_val = 0.0f;
    for (int i = 0; i < HORIZON-1; i++) {
        trend_val += (outputs[i+1] - outputs[i]) * weights[i];
    }
    int trend = (trend_val >= 0.0f) ? 1 : -1;

    // Write the plain text response
    int offset_buf = snprintf((char *)buffer, preferred_size, "trend=%d", trend);

    // Prevent unused variable warnings (required by compiler settings)
    //printf("%p\n", eml_net_activation_function_strs);
    //printf("%p\n", eml_error_str);
    (void)eml_net_activation_function_strs;
    (void)eml_error_str;

    // Set CoAP response headers and payload as plain text
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, offset_buf);
}