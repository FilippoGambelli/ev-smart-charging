#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_solar_obs,
               "title=\"Solar Data\";rt=\"application/json\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

// Example variables (would normally come from sensors)
static float Gb   = 750.0; // W/m²
static float Gd   = 120.0; // W/m²
static float Gr   = 30.0;  // W/m²
static float T    = 25.4;  // °C
static float WS  = 3.2;   // m/s

/* Event handler is called when the resource state changes*/
static void res_event_handler(void) {
  // Example simulation: values change slightly each event
  Gb += 0.5;
  Gd += 0.2;
  Gr += 0.1;
  T  += 0.05;
  WS += 0.01;

  // Notify all observers that data has changed
  coap_notify_observers(&res_solar_obs);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  
  int length = snprintf((char *)buffer, preferred_size,
      "{\"Gb\":%d,\"Gd\":%d,\"Gr\":%d,"
      "\"T\":%d,\"WS\":%d}",
      (int)(Gb * 10), (int)(Gd * 10), (int)(Gr * 10), (int)(T  * 10), (int)(WS * 10));
  
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, buffer, length);
}
