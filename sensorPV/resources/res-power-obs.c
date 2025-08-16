#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "eml_net.h"
#include "coap-engine.h"
#include "solar-data/solar-data.h"
#include "modelTiny.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_power_obs,
               "title=\"Power Data\";rt=\"application/json\";obs",
               res_get_handler,
               NULL,
               NULL,
               NULL,
               res_event_handler);

/* Event handler is called when the resource state changes*/
static void res_event_handler(void) {
  // Notify all observers that data has changed
  coap_notify_observers(&res_power_obs);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

  /* --- Calcolo feature temporali --- */
  time_t t = timestamp[counter];

  struct tm *tm_info = localtime(&t);

  int hour = tm_info->tm_hour;
  int minute = tm_info->tm_min;
  int woy = tm_info->tm_yday / 7;

  float hour_angle = 2.0f * M_PI * hour / 24.0f;
  float min_angle  = 2.0f * M_PI * minute / 60.0f;
  float woy_angle  = 2.0f * M_PI * woy / 52.0f;

  float hour_sin = sinf(hour_angle);
  float hour_cos = cosf(hour_angle);
  float min_sin  = sinf(min_angle);
  float min_cos  = cosf(min_angle);
  float woy_sin  = sinf(woy_angle);
  float woy_cos  = cosf(woy_angle);

  /* --- Creazione vettore feature --- */
  float features[] = {
    Gb[counter], Gd[counter], Gr[counter], HSun[counter], T[counter], WS[counter],
    hour_sin, hour_cos, min_sin, min_cos, woy_sin, woy_cos
  };

  float outputs[16]; // 16 valori in output

  printf("%p\n", eml_net_activation_function_strs); // This is needed to avoid compiler error (warnings == errors)
  printf("%p\n", eml_error_str);

  eml_net_predict_proba(&modelTiny, features, sizeof(features)/sizeof(float), outputs, 16);

  /* --- Creazione JSON con tutti i valori --- */
  int offset_buf = 0;
  offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf, "{");

  for(int i = 0; i < 16; i++) {
    offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf,
                           "\"%d\":%.4f%s", i+1, outputs[i], (i<15)?",":"");
  }

  offset_buf += snprintf((char *)buffer + offset_buf, preferred_size - offset_buf, "}");

  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, buffer, offset_buf);
}