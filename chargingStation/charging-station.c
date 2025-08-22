#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "etimer.h"
#include "os/dev/button-hal.h"
#include "car-pool/car-pool.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP

// Address of the central node
#define CENTRAL_NODE_EP "coap://[fd00::201:1:1:1]:5683"
#define RES_CHARGER_REGISTER_URI "/registration/charger"
#define RES_CAR_REGISTER_URI "/registration/car"

// EXAMPLE
#define CHARGER_maxKW 22.0

static uint8_t my_id = 0; // Stores the ID assigned by the server after registration

// Callback function to handle the server's response to the charger registration request
static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if(response) {
    int len = coap_get_payload(response, &chunk);
    if(len > 0) {
      char id_str[4] = {0};
      memcpy(id_str, chunk, len < sizeof(id_str)-1 ? len : sizeof(id_str)-1);
      my_id = (uint8_t)atoi(id_str);
    }
  }
}

// Callback function to handle the server's response to the car registration request
static void vehicles_handler(coap_message_t *response) {
  if(response) {
    uint8_t code = response->code;

    if(code == CHANGED_2_04) {
      // Successfully registered the car
      LOG_INFO("Update car status successful\n");
    } else {
      // Registration failed
      LOG_INFO("Update car status failed with code: %u\n", code);
    }
  }
}

extern coap_resource_t res_charging_status;


PROCESS(charging_station_process, "Charging Station Node Process");
AUTOSTART_PROCESSES(&charging_station_process);

PROCESS_THREAD(charging_station_process, ev, data){
    static coap_endpoint_t server_ep;
    static coap_message_t request[1];

    static struct etimer registration_timer;
    static char buffer_req[128];

    PROCESS_BEGIN();

    LOG_INFO("Charging station node started\n");

    coap_activate_resource(&res_charging_status, "res_charging_status");

    // Initialize the CoAP server endpoint
    coap_endpoint_parse(CENTRAL_NODE_EP, strlen(CENTRAL_NODE_EP), &server_ep);

    // Set the initial registration timer
    etimer_set(&registration_timer, 5 * CLOCK_SECOND); // for testing, previously 5 minutes

    while(1) {
        // If not registered, attempt registration
        if(my_id == 0 && etimer_expired(&registration_timer)) {
            LOG_INFO("Sending registration request...\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
            coap_set_header_uri_path(request, RES_CHARGER_REGISTER_URI);

            snprintf(buffer_req, sizeof(buffer_req), "maxKW=%.2f", CHARGER_maxKW);
            coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));

            COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

            if(my_id > 0) {
                LOG_INFO("Registration successful, ID=%u\n", my_id);
            } else {
                LOG_INFO("Registration failed, retrying in 5 minutes...\n");
                etimer_set(&registration_timer, 5 * 60 * CLOCK_SECOND); // reset for retry
            }
        }

        // Handle button press events
        if(ev == button_hal_press_event && my_id > 0) {
            LOG_INFO("Sending vehicle connection request......\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

            // Build payload
            const car_t* selected_car = get_random_car();

            
            snprintf(buffer_req, sizeof(buffer_req), 
              "type=connection&carMaxKW=%.2f&carMaxCapacity=%u&currentCharge=%.2f&desiredCharge=%.2f&plate=%s",
              selected_car->carMaxKW, selected_car->carCapacity, selected_car->currentCharge, selected_car->desiredCharge, selected_car->plate);
            
            // For testing, use a fixed car configuration
            // snprintf(buffer_req, sizeof(buffer_req), "type=connection&carMaxKW=%.2f&carMaxCapacity=%u&currentCharge=%.2f&desiredCharge=%.2f&plate=%s",
            //  22.00, 1, 80.00, 82.00, "XY987ZT");
	
			      coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));

            COAP_BLOCKING_REQUEST(&server_ep, request, vehicles_handler);
        }

        if(ev == button_hal_periodic_event && my_id > 0) {
          button_hal_button_t *btn = (button_hal_button_t *)data;
          if(btn->press_duration_seconds > 3) {
            LOG_INFO("Sending vehicle disconnection request......\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

            // Build payload
            snprintf(buffer_req, sizeof(buffer_req), "type=disconnection");
	
			      coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));

            COAP_BLOCKING_REQUEST(&server_ep, request, vehicles_handler);
          }
        }

        PROCESS_WAIT_EVENT(); // wait for the next event (timer, button, etc.)
    }

    PROCESS_END();
}