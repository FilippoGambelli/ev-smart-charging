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
#include "os/dev/leds.h"

// Helper macros to handle LEDs differently in Cooja vs real hardware
#ifdef COOJA
  #define LED_OFF(led) leds_off(LEDS_NUM_TO_MASK(led))
  #define LED_SINGLE_OFF(led) leds_single_off(led)
#else
  #define LED_OFF(led) leds_off(led)
  #define LED_SINGLE_OFF(led) leds_single_off(led)
#endif

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP

#define RETRY_INTERVAL 30  // Retry interval for registration attempts

#define CLOUD_APPLICATION_EP "coap://[fd00::1]:5683/"

// Address of the central node
#define RES_CHARGER_REGISTER_URI "/registration/charger"
#define RES_CAR_REGISTER_URI "/registration/car"
#define RES_CLOUD_REGISTER_URI "/registration"


#define CHARGER_MAX_POWER 22.0 // In kWh
static uint8_t my_id = 0;   // Stores the ID assigned by the server after registration

static struct etimer registration_timer;

static coap_endpoint_t central_node_ep;
static coap_endpoint_t cloud_application_ep;

extern coap_resource_t res_charging_status;


// Callback function to handle the cloud application's response to the charger registration device
static void reg_handler(coap_message_t *response);

// Callback function to handle the server's response to the charger registration request
static void charging_station_handler(coap_message_t *response);

// Callback function to handle the server's response to the car registration request
static void vehicles_handler(coap_message_t *response);



PROCESS(charging_station, "Charging Station");
AUTOSTART_PROCESSES(&charging_station);

PROCESS_THREAD(charging_station, ev, data){
    static coap_message_t request[1];

    static char buffer[128];

    PROCESS_BEGIN();

    LOG_INFO("Charging station started\n");

    coap_activate_resource(&res_charging_status, "charging_status");
    LOG_INFO("CoAP resources activated\n");


    LOG_INFO("Sending registration to cloud application....\n");

    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 7);
    coap_set_header_uri_path(request, RES_CLOUD_REGISTER_URI);

    // Request to cloud application
    coap_endpoint_parse(CLOUD_APPLICATION_EP, strlen(CLOUD_APPLICATION_EP), &cloud_application_ep);

    // Build payload (JSON)
    snprintf(buffer, sizeof(buffer),
        "{"
        "\"nodeType\":\"chargingStation\","
        "\"requiredNodes\":["
            "\"centralNode\""
        "]"
        "}"
    );

    coap_set_header_content_format(request, APPLICATION_JSON);
    coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));
    COAP_BLOCKING_REQUEST(&cloud_application_ep, request, reg_handler);

    etimer_set(&registration_timer, 10 * CLOCK_SECOND);     // wait until central node is ready

    while(1) {
        // If not registered, attempt registration
        if(my_id == 0 && etimer_expired(&registration_timer)) {
            LOG_INFO("Sending registration request....\n");

            coap_init_message(request, COAP_TYPE_CON, COAP_POST, 4);
            coap_set_header_uri_path(request, RES_CHARGER_REGISTER_URI);

            snprintf(buffer, sizeof(buffer), "maxPower=%d,%04d", (int)CHARGER_MAX_POWER, (int)((CHARGER_MAX_POWER - (int)CHARGER_MAX_POWER) * 10000));

            LOG_INFO("Data: maxPower: %d,%04d kW\n", (int)CHARGER_MAX_POWER, (int)((CHARGER_MAX_POWER - (int)CHARGER_MAX_POWER) * 10000));

            coap_set_header_content_format(request, TEXT_PLAIN);
            coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));
            COAP_BLOCKING_REQUEST(&central_node_ep, request, charging_station_handler);
        }

        // Handle button release events
        if(ev == button_hal_release_event && my_id > 0) {
            button_hal_button_t *btn = (button_hal_button_t *)data;
            
            if(btn->press_duration_seconds < 3) {
                LOG_INFO("Sending vehicle connection request....\n");

                coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 5);
                coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

                // Build payload
                const car_t* selected_car = get_next_car(my_id);
                
                snprintf(buffer, sizeof(buffer), 
                    "type=connection&carMaxPower=%d,%04d&carMaxCapacity=%d,%04d&currentCharge=%d,%04d&desiredCharge=%d,%04d&plate=%s",
                    (int)selected_car->carMaxPower, (int)((selected_car->carMaxPower - (int)selected_car->carMaxPower) * 10000),
                    (int)selected_car->carCapacity, (int)((selected_car->carCapacity - (int)selected_car->carCapacity) * 10000),
                    (int)selected_car->currentCharge, (int)((selected_car->currentCharge - (int)selected_car->currentCharge) * 10000),
                    (int)selected_car->desiredCharge, (int)((selected_car->desiredCharge - (int)selected_car->desiredCharge) * 10000),
                    selected_car->plate);

                LOG_INFO("Data - Type: connection | Car max power: %d,%04d kW | Car max capacity: %d,%04d kWh | Current charge: %d,%02d %% | Desired charge: %d,%02d %% | Plate: %s\n",
                    (int)selected_car->carMaxPower, (int)((selected_car->carMaxPower - (int)selected_car->carMaxPower) * 10000),
                    (int)selected_car->carCapacity, (int)((selected_car->carCapacity - (int)selected_car->carCapacity) * 10000),
                    (int)selected_car->currentCharge, (int)((selected_car->currentCharge - (int)selected_car->currentCharge) * 100),
                    (int)selected_car->desiredCharge, (int)((selected_car->desiredCharge - (int)selected_car->desiredCharge) * 100),
                    selected_car->plate);

                coap_set_header_content_format(request, TEXT_PLAIN);
                coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));

                COAP_BLOCKING_REQUEST(&central_node_ep, request, vehicles_handler); 
            } else {
                LOG_INFO("Sending vehicle disconnection request....\n");

                coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 6);
                coap_set_header_uri_path(request, RES_CAR_REGISTER_URI);

                // Build payload
                snprintf(buffer, sizeof(buffer), "type=disconnection");

                LOG_INFO("Data: type=disconnection\n");
        
                coap_set_header_content_format(request, TEXT_PLAIN);
                coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));
                COAP_BLOCKING_REQUEST(&central_node_ep, request, vehicles_handler);
            }
        }

        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}


// Callback function to handle the cloud application's response
static void reg_handler(coap_message_t *response) {
    const uint8_t *payload = NULL;
    size_t len = coap_get_payload(response, &payload);

    if(len > 0 && payload != NULL) {

        char *ptr = strstr((char *)payload, "centralNode=");
        if(ptr != NULL) {
            ptr += strlen("centralNode=");

            // Build the full CoAP endpoint URI with default port 5683
            char endpoint_uri[80];
            snprintf(endpoint_uri, sizeof(endpoint_uri), "coap://[%s]:5683", ptr);

            
            // Parse and save endpoint
            if (coap_endpoint_parse(endpoint_uri, strlen(endpoint_uri), &central_node_ep)) {
                LOG_INFO("Central node endpoint from cloud: %s\n", endpoint_uri);
            } else {
                LOG_INFO("Failed to parse central node endpoint\n");
            }
        }
    }
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
                LOG_INFO("Registration successful - ID: %u\n", my_id);

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
            LED_OFF(LEDS_ALL);
            LED_SINGLE_OFF(LEDS_YELLOW);
            LOG_INFO("Update car status successful\n");
        } else {
            LOG_INFO("Update car status failed\n");
        }
    }
}
