#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "eml_net.h"
#include "coap-engine.h"
#include "real-data/solar-data.h"
#include "modelTiny.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define N_FEATURES 12
#define SEQ_LEN 16
#define HORIZON 16

// Min e Max per le features
static const float X_min[N_FEATURES] = {
    0.0f, 0.0f, 0.0f, 0.0f, -3.29f, 0.0f,
   -1.0f, -1.0f, -0.8660254f, -0.8660254f, -1.0f, -1.0f
};

static const float X_max[N_FEATURES] = {
   973.99f, 436.64f, 20.4f, 69.63f, 34.07f, 11.38f,
    1.0f, 1.0f, 0.8660254f, 0.8660254f, 1.0f, 1.0f
};

// Min e Max per l'output
static const float y_min = 0.0f;
static const float y_max = 111750.0f;


static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_pred_power_obs,
               "title=\"Predicted Power Data\";rt=\"application/json\";obs",
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

// Funzioni di normalizzazione
static float normalize(float val, float min_val, float max_val) {
    return (val - min_val) / (max_val - min_val);
}

static float denormalize(float val, float min_val, float max_val) {
    return val * (max_val - min_val) + min_val;
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    float features[SEQ_LEN * N_FEATURES];
    float outputs[HORIZON];
    time_t t;

    // Creazione delle features normalizzate
    for (int i = 0; i < SEQ_LEN; i++) {
        int idx = solar_data_counter - SEQ_LEN + i;  // correggo l’indice

        t = timestamp[idx];
        struct tm *tm_info = localtime(&t);

        int hour   = tm_info->tm_hour;
        int minute = tm_info->tm_min;
        int woy    = tm_info->tm_yday / 7;  // settimana dell’anno

        float hour_angle = 2.0f * M_PI * hour / 24.0f;
        float min_angle  = 2.0f * M_PI * minute / 60.0f;
        float woy_angle  = 2.0f * M_PI * woy / 52.0f;

        float hour_sin = sinf(hour_angle);
        float hour_cos = cosf(hour_angle);
        float min_sin  = sinf(min_angle);
        float min_cos  = cosf(min_angle);
        float woy_sin  = sinf(woy_angle);
        float woy_cos  = cosf(woy_angle);

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

    // Predizione con il modello emlearn
    eml_net_predict_proba(&modelTiny, features, SEQ_LEN * N_FEATURES, outputs, HORIZON);

    // Denormalizzo l’output
    for (int i = 0; i < HORIZON; i++) {
        outputs[i] = denormalize(outputs[i], y_min, y_max);
    }

    // Timestamp base (ultimo della sequenza)
    time_t base_time = t;

    int offset_buf = 0;
    offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf, "{");

    for (int i = 0; i < HORIZON; i++) {
        // aggiungo 15 minuti per ogni step
        time_t pred_time = base_time + (i + 1) * 15 * 60;
        struct tm *tm_info = localtime(&pred_time);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M", tm_info);

        offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf,
                               "\"%d\":{\"value\":%.4f,\"time\":\"%s\"}%s",
                               i + 1, outputs[i], time_str,
                               (i < HORIZON - 1) ? "," : "");
    }

    offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf, "}");

    printf("%p\n", eml_net_activation_function_strs); // This is needed to avoid compiler error (warnings == errors)
    printf("%p\n", eml_error_str);

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, offset_buf);
}