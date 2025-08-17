#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "real-data/solar-data.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_solar_obs,
               "title=\"Solar Data\";rt=\"application/json\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

/* Event handler is called when the resource state changes*/
static void res_event_handler(void) {
    solar_data_counter++;

    // Notify all observers that data has changed
    coap_notify_observers(&res_solar_obs);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    struct tm *t = localtime(&solar_data_timestamp[solar_data_counter]);

    int length = snprintf((char *)buffer, preferred_size,
        "{\"Time\":%02d-%02d-%04d %02d:%02d:%02d,\"Gb\":%.4f,\"Gd\":%.4f,\"Gr\":%.4f,"
        "\"HSun\":%.4f,\"T\":%.4f,\"WS\":%.4f}",
        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,
        Gb[solar_data_counter], Gd[solar_data_counter], Gr[solar_data_counter], HSun[solar_data_counter], T[solar_data_counter], WS[solar_data_counter]);

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
}