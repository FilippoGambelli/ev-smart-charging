#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "power-manager/power-manager.h"
#include "ipv6.h"
#include "config-ml/config-ml.h"
#include "coap-callback-api.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP


// Real dichiaration of extern variables
float power_PV_real;
int power_PV_trend;
float power_PV_pred;


// Define a Contiki process for the CoAP observe client
PROCESS(coap_observe_client, "CoAP Observe Client");
// Automatically start the process at boot
AUTOSTART_PROCESSES(&coap_observe_client);

#define RES_POWER_OBS_URI "/res_real_power_obs"  // Resource on server to observe

static coap_observee_t *obs = NULL;  // Keeps track of the current observation

void response_handler_pred_power(coap_callback_request_state_t *callback_state) {
    coap_message_t *response = callback_state->state.response;
    if(response != NULL) {
        const uint8_t *payload;
        size_t len = coap_get_payload(response, &payload);

        if(len > 0 && payload != NULL) {
            char buf[len + 1];
            memcpy(buf, payload, len);
            buf[len] = '\0';

            char *ptr = strstr(buf, "trend=");
            if(ptr != NULL) {
                ptr += strlen("trend=");
                power_PV_trend = strtof(ptr, NULL);
            }

            ptr = strstr(buf, "firstPrediction=");
            if(ptr != NULL) {
                ptr += strlen("firstPrediction=");
                power_PV_pred = strtof(ptr, NULL) / 1000.0f;
            }
            LOG_INFO("!!PREDIZIONE RICEVUTA: trend: %d, firstPrediction: %.3f kW\n",
              power_PV_trend, power_PV_pred);

            power_manager_update_charging_station();
        }
    } else {
        // LOG_WARN("No response received from /res_pred_power\n");
    }
}

// Callback function called when a notification from the server is received
static void notification_realpower_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag) {
  int len = 0;
  const uint8_t *payload = NULL;

  if(notification) {
    // Extract payload from the notification
    len = coap_get_payload(notification, &payload);
  }


  // Handle different notification flags
  switch(flag) {
    case NOTIFICATION_OK:
      LOG_INFO("NOTIFICATION OK: %*s", len, (char *)payload);
      LOG_INFO_("\n");

      if(len > 0 && payload != NULL) {

        char buf[len + 1];
        memcpy(buf, payload, len);
        buf[len] = '\0';

        char *ptr = strstr(buf, "realPV=");
        if(ptr != NULL) {
              ptr += strlen("realPV=");     // shift after =
              power_PV_real = strtof(ptr, NULL)/1000;
        }

        /*
        ptr = strstr(buf, "timestamp=");
        if(ptr != NULL) {
            power_data_timestamp[power_data_counter] = strtol(ptr + 10, NULL, 10);
        }*/
      }
      
      break;
    case OBSERVE_OK: /* server accepted observation request */
      LOG_INFO("OBSERVE_OK: %*s", len, (char *)payload);
      LOG_INFO_("\n");
      break;
    case OBSERVE_NOT_SUPPORTED: // Server does not support observation
      LOG_INFO("OBSERVE_NOT_SUPPORTED: %*s", len, (char *)payload);
      LOG_INFO_("\n");
      obs = NULL;
      break;
    case ERROR_RESPONSE_CODE: // Server returned an error response
      LOG_INFO("ERROR_RESPONSE_CODE: %*s", len, (char *)payload);
      LOG_INFO_("\n");
      obs = NULL;
      break;
    case NO_REPLY_FROM_SERVER: // No response from server
      LOG_INFO("NO_REPLY_FROM_SERVER: removing observe registration with token %x%x", obs->token[0], obs->token[1]);
      LOG_INFO_("\n");
      obs = NULL;
      break;
  }
}

extern coap_resource_t res_car_reg;
extern coap_resource_t res_charger_reg;
extern coap_resource_t res_ml_pred_interval;

// Main process thread
PROCESS_THREAD(coap_observe_client, ev, data) {
  static coap_endpoint_t sensor_pv_ep;

  PROCESS_BEGIN();

  #if BORDER_ROUTER_CONF_WEBSERVER
    PROCESS_NAME(webserver_nogui_process);
    process_start(&webserver_nogui_process, NULL);
  #endif /* BORDER_ROUTER_CONF_WEBSERVER */

  // Parse server endpoint string into coap_endpoint_t structure
  coap_endpoint_parse(SENSOR_PV_EP, strlen(SENSOR_PV_EP), &sensor_pv_ep);

  LOG_INFO("Starting CoAP observation");
  LOG_INFO_("\n");

  obs = coap_obs_request_registration(&sensor_pv_ep, RES_POWER_OBS_URI, notification_realpower_callback, NULL);

  if(obs == NULL) {
    LOG_INFO("Error registering observation");
    LOG_INFO_("\n");
  }

  coap_activate_resource(&res_charger_reg, "registration/charger");
  coap_activate_resource(&res_car_reg, "registration/car");
  coap_activate_resource(&res_ml_pred_interval, "res_ml_pred_interval");

  etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);

  static coap_message_t request[1];
  static coap_callback_request_state_t request_state;

  // Main loop: wait for events (notifications, timers, etc.)
  while(1) {
    PROCESS_WAIT_EVENT();

    // Handle predicted power data timer event
    if(ev == PROCESS_EVENT_TIMER && data == &e_timer_ml_pred){
      LOG_INFO("!!RICHIESTA GET PER PREDIZIONE\n");
      // Initialize the CoAP message (CON type, PUT method, message ID 0)
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);

      coap_set_header_uri_path(request, "/res_pred_power");

      coap_send_request(&request_state, &sensor_pv_ep, request, response_handler_pred_power);

      etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);
    }
  }

  PROCESS_END();
}