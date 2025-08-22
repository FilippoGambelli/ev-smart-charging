#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "coap-engine.h"
#include "real-data/power-data.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

int power_data_counter = 1440;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_real_power_obs,
               "title=\"Real Power Data\";rt=\"application/json\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

/* Event handler is called when the resource state changes*/
static void res_event_handler(void) {
    power_data_counter++;

    // Notify all observers that data has changed
    coap_notify_observers(&res_real_power_obs);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    int length = snprintf((char *)buffer, preferred_size,
        "timestamp=%ld&realPV=%.4f",
        (long) power_data_timestamp[power_data_counter], P[power_data_counter]);

    struct tm *tm_info = localtime(&power_data_timestamp[power_data_counter]);
    LOG_INFO("Timestamp: %02d-%02d-%04d %02d:%02d:%02d, Real PV: %.4f\n",
        tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
        P[power_data_counter]);

    coap_set_payload(response, buffer, length);
}