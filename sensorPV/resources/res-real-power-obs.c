#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "real-data/power-data.h"

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

  struct tm *t = localtime(&timestamp_power[power_data_counter]);

  int length = snprintf((char *)buffer, preferred_size,
      "{\"Timestamp\":\"%02d-%02d-%04d %02d:%02d:%02d\",\"P\":%.4f}",
      t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec,P[power_data_counter]);

  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, buffer, length);
}