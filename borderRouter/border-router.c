#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

// Define a Contiki process for the CoAP observe client
PROCESS(coap_observe_client, "CoAP Observe Client");
// Automatically start the process at boot
AUTOSTART_PROCESSES(&coap_observe_client);

#define RES_POWER_OBS_URI "/res_real_power_obs"  // Resource on server to observe
#define RES_PRED_POWER_OBS_URI "/res_pred_power_obs"  // Resource on server to observe
#define SERVER_EP "coap://[fd00::202:2:2:2]:5683"  // CoAP server endpoint

static coap_observee_t *obs = NULL;  // Keeps track of the current observation
static coap_observee_t *obs1 = NULL;  // Keeps track of the current observation

// Callback function called when a notification from the server is received
static void notification_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag) {
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

// Main process thread
PROCESS_THREAD(coap_observe_client, ev, data) {
  static coap_endpoint_t server_ep;

  PROCESS_BEGIN();

  #if BORDER_ROUTER_CONF_WEBSERVER
    PROCESS_NAME(webserver_nogui_process);
    process_start(&webserver_nogui_process, NULL);
  #endif /* BORDER_ROUTER_CONF_WEBSERVER */

  coap_activate_resource(&res_charger_reg, "registration/charger");
  coap_activate_resource(&res_car_reg, "registration/car");

  // Parse server endpoint string into coap_endpoint_t structure
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  LOG_INFO("Starting CoAP observation");
  LOG_INFO_("\n");

  obs = coap_obs_request_registration(&server_ep, RES_POWER_OBS_URI, notification_callback, NULL);

  if(obs == NULL) {
    LOG_INFO("Error registering observation");
    LOG_INFO_("\n");
  }

  obs1 = coap_obs_request_registration(&server_ep, RES_PRED_POWER_OBS_URI, notification_callback, NULL);

  if(obs1 == NULL) {
    LOG_INFO("Error registering observation");
    LOG_INFO_("\n");
  }

  // Main loop: wait for events (notifications, timers, etc.)
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}