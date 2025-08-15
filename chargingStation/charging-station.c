#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "etimer.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP

// Address of the central node
#define CENTRAL_NODE_EP "coap://[fd00::201:1:1:1]:5683"
#define RES_REGISTER_URI "/register"

static uint8_t my_id = 0; // Stores the ID assigned by the server after registration

// Callback function to handle the server's response to the registration request
static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if(response) {
    int len = coap_get_payload(response, &chunk);
    if(len > 0) {
      char id_str[4] = {0};
      memcpy(id_str, chunk, len < sizeof(id_str)-1 ? len : sizeof(id_str)-1);
      my_id = (uint8_t)atoi(id_str);
      printf("Registered with ID=%u\n", my_id);
    }
  }
}

PROCESS(charging_station_process, "Charging Station Node Process");
AUTOSTART_PROCESSES(&charging_station_process);

PROCESS_THREAD(charging_station_process, ev, data){
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];

  static struct etimer registration_timer;

  PROCESS_BEGIN();

  LOG_INFO("Charging station node started");
  LOG_INFO_("\n");

  // Initialize the CoAP endpoint of the central node
  coap_endpoint_parse(CENTRAL_NODE_EP, strlen(CENTRAL_NODE_EP), &server_ep);

  etimer_set(&registration_timer, 5 * 60 * CLOCK_SECOND);

  while(my_id == 0) {
    LOG_INFO("Sending registration request...\n");

    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, RES_REGISTER_URI);

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

    if(my_id > 0) {
      LOG_INFO("Registration successful, ID=%u", my_id);
      LOG_INFO_("\n");
      break;
    } else {
      LOG_INFO("Registration failed, retrying in 5 minutes...");
      LOG_INFO_("\n");
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
      etimer_reset(&registration_timer);
    }
  }

  PROCESS_END();
}