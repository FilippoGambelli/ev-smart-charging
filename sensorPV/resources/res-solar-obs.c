#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "real-data/solar-data.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP



static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_solar_obs,
               "title=\"Solar Data\";rt=\"application/json\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

/* Event handler is called when the resource state changes */
static void res_event_handler(void) {
    solar_data_counter = (solar_data_counter + 1) % SOLAR_DATA_TIMESTAMP_LEN;

    // Notify all observers that data has changed
    coap_notify_observers(&res_solar_obs);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    struct tm *t = gmtime(&solar_data_timestamp[solar_data_counter]);

    int length = snprintf((char *)buffer, preferred_size,
        "{\"Time\":\"%02d-%02d-%04d %02d:%02d:%02d\","
        "\"Gb\":%d.%04d,"
        "\"Gd\":%d.%04d,"
        "\"Gr\":%d.%04d,"
        "\"HSun\":%d.%04d,"
        "\"T\":%d.%04d,"
        "\"WS\":%d.%04d}",
        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,
        (int)Gb[solar_data_counter], (int)((Gb[solar_data_counter] - (int)Gb[solar_data_counter]) * 10000),
        (int)Gd[solar_data_counter], (int)((Gd[solar_data_counter] - (int)Gd[solar_data_counter]) * 10000),
        (int)Gr[solar_data_counter], (int)((Gr[solar_data_counter] - (int)Gr[solar_data_counter]) * 10000),
        (int)HSun[solar_data_counter], (int)((HSun[solar_data_counter] - (int)HSun[solar_data_counter]) * 10000),
        (int)T[solar_data_counter], (int)((T[solar_data_counter] - (int)T[solar_data_counter]) * 10000),
        (int)WS[solar_data_counter], (int)((WS[solar_data_counter] - (int)WS[solar_data_counter]) * 10000));

    LOG_INFO("Timestamp: %02d-%02d-%04d %02d:%02d:%02d | Gb: %d,%04d W/m2 | Gd: %d,%04d W/m2 | Gr: %d,%04d W/m2 | HSun: %d,%04d degree | T: %d,%04d Celsius | WS: %d,%04d m/s\n",
        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,
        (int)Gb[solar_data_counter], (int)((Gb[solar_data_counter] - (int)Gb[solar_data_counter]) * 10000),
        (int)Gd[solar_data_counter], (int)((Gd[solar_data_counter] - (int)Gd[solar_data_counter]) * 10000),
        (int)Gr[solar_data_counter], (int)((Gr[solar_data_counter] - (int)Gr[solar_data_counter]) * 10000),
        (int)HSun[solar_data_counter], (int)((HSun[solar_data_counter] - (int)HSun[solar_data_counter]) * 10000),
        (int)T[solar_data_counter], (int)((T[solar_data_counter] - (int)T[solar_data_counter]) * 10000),
        (int)WS[solar_data_counter], (int)((WS[solar_data_counter] - (int)WS[solar_data_counter]) * 10000));

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
    coap_set_status_code(response, CONTENT_2_05);
}