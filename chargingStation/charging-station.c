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
#include "ipv6.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP

#define RETRY_INTERVAL 30  // Retry interval for registration attempts

// Address of the central node
#define RES_CHARGER_REGISTER_URI "/registration/charger"
#define RES_CAR_REGISTER_URI "/registration/car"

#define CHARGER_MAX_POWER 22000 // In Wh

static uint8_t my_id = 0;   // Stores the ID assigned by the server after registration

static struct etimer registration_timer;

extern coap_resource_t res_charging_status;

// Callback function to handle the server's response to the charger registration request
static void charging_station_handler(coap_message_t *response);

// Callback function to handle the server's response to the car registration request
static void vehicles_handler(coap_message_t *response);



PROCESS(charging_station, "Charging Station");
AUTOSTART_PROCESSES(&charging_station);

PROCESS_THREAD(charging_station, ev, data){
    static coap_endpoint_t central_node_ep;
    static coap_message_t request[1];

    static char buffer[128];

    PROCESS_BEGIN();

    LOG_INFO("Charging station started\n");

    coap_activate_resource(&res_charging_status, "res_charging_status");
    LOG_INFO("CoAP resources activated\n");

    // Initialize the CoAP server endpoint
    coap_endpoint_parse(CENTRAL_NODE_EP, strlen(CENTRAL_NODE_EP), &central_node_ep);

    etimer_set(&registration_timer, 10 * CLOCK_SECOND);     // wait until central node is ready

    while(1) {
        // If not registered, attempt registration
        if(my_id == 0 && etimer_expired(&registration_timer)) {
            LOG_INFO("Sending registration request....\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
            coap_set_header_uri_path(request, RES_CHARGER_REGISTER_URI);

            snprintf(buffer, sizeof(buffer), "maxPower=%d", CHARGER_MAX_POWER);

            LOG_INFO("Data: maxPower=%d\n", CHARGER_MAX_POWER);

            coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));
            COAP_BLOCKING_REQUEST(&central_node_ep, request, charging_station_handler);
        }

        // Handle button press events
        if(ev == button_hal_press_event && my_id > 0) {
            LOG_INFO("Sending vehicle connection request....\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

            // Build payload
            const car_t* selected_car = get_random_car();
            
            snprintf(buffer, sizeof(buffer), 
                "type=connection&carMaxPower=%d&carMaxCapacity=%u&currentCharge=%d&desiredCharge=%d&plate=%s",
                selected_car->carMaxPower, selected_car->carCapacity, selected_car->currentCharge, selected_car->desiredCharge, selected_car->plate);

            // For testing, use a fixed car configuration
            // snprintf(buffer, sizeof(buffer), "type=connection&carMaxKW=%.2f&carMaxCapacity=%u&currentCharge=%.2f&desiredCharge=%.2f&plate=%s",
            //  22.00, 1, 80.00, 82.00, "XY987ZT");

            LOG_INFO("Data: type=connection | carMaxPower=%d | carMaxCapacity=%u | currentCharge=%d | desiredCharge=%d | plate=%s\n",
                selected_car->carMaxPower, selected_car->carCapacity, selected_car->currentCharge, selected_car->desiredCharge, selected_car->plate);
    
            coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));

            COAP_BLOCKING_REQUEST(&central_node_ep, request, vehicles_handler); 
        }

        if(ev == button_hal_periodic_event && my_id > 0) {
            button_hal_button_t *btn = (button_hal_button_t *)data;
            if(btn->press_duration_seconds > 3) {
                LOG_INFO("Sending vehicle disconnection request....\n");

                coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
                coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

                // Build payload
                snprintf(buffer, sizeof(buffer), "type=disconnection");

                LOG_INFO("Data: type=disconnection\n");
        
                coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));

                COAP_BLOCKING_REQUEST(&central_node_ep, request, vehicles_handler);
            }
        }

        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}



// Callback function to handle the server's response to the charger registration request
static void charging_station_handler(coap_message_t *response) {
    const uint8_t *chunk = NULL;

    if (response) {
        int len = coap_get_payload(response, &chunk);
        if (len > 0) {
            char buffer[32] = {0};
            memcpy(buffer, chunk, len);

            char *eq_pos = strchr(buffer, '=');
            if (eq_pos) {
                my_id = (uint8_t)atoi(eq_pos + 1);
                etimer_stop(&registration_timer);
                LOG_INFO("Registration successful - ID=%u\n", my_id);

                return;
            }
        }
    }

    LOG_INFO("Registration failed - retrying in %d seconds...\n", RETRY_INTERVAL);
    etimer_set(&registration_timer, RETRY_INTERVAL * CLOCK_SECOND);    // reset for retry
}


// Callback function to handle the server's response to the car registration request
static void vehicles_handler(coap_message_t *response) {
    if(response) {
        uint8_t code = response->code;

        if(code == CHANGED_2_04) {
            LOG_INFO("Update car status successful\n");
        } else {
            LOG_INFO("Update car status failed\n");
        }
    }
}
