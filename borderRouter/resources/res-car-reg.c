#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "coap-blocking-api.h"
#include "coap-endpoint.h"
#include "coap-callback-api.h"
#include "info-charging-station.h"
#include <math.h>
#include <stdint.h> 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

coap_endpoint_t cloud_application_ep;

static int EV_charger_last_idx = -1;

#define PRIORITY_CHARGING_FACTOR 1.4 // Factor for priority charging
#define STANDARD_CHARGING_FACTOR 1.7 // Factor for standard charging


// Function prototypes
static void res_connection_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static uint8_t get_device_id_by_addr(const uip_ipaddr_t *src_addr);

void client_response_handler(coap_callback_request_state_t *callback_state) {
    // Get the CoAP response from the callback state
    coap_message_t *response = callback_state->state.response;
    if(!response) {
        // TODO: addd LOG_INFO
        return;
    }

    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);

    if(len > 0) {
        // Expected response: "priority=1" or "priority=0"
        if(strstr((const char *)chunk, "priority=1") != NULL) {
            EV_charger[EV_charger_last_idx].priority = 1;
        } else {
            EV_charger[EV_charger_last_idx].priority = 0;
        }

        LOG_INFO("EV_charger.priority: %d\n", EV_charger[EV_charger_last_idx].priority);

        // Calculate energy needed (kWh) to reach target SOC
        float energy_needed = (EV_charger[EV_charger_last_idx].soc_target - 
                               EV_charger[EV_charger_last_idx].soc_current) / 100.0f *
                               EV_charger[EV_charger_last_idx].vehicle_max_capacity;

        // Effective charging power = minimum of vehicle max power and station max power
        float effective_power = (EV_charger[EV_charger_last_idx].vehicle_max_charging_power < 
                                 EV_charger[EV_charger_last_idx].max_charging_power)
                                 ? EV_charger[EV_charger_last_idx].vehicle_max_charging_power
                                 : EV_charger[EV_charger_last_idx].max_charging_power;

        // Base charging duration in seconds
        float charging_duration = (energy_needed / effective_power) * 3600.0f;

        // Apply factor depending on priority
        float factor = EV_charger[EV_charger_last_idx].priority 
                       ? PRIORITY_CHARGING_FACTOR 
                       : STANDARD_CHARGING_FACTOR;

        // Estimated charging duration in seconds
        float estimated_charging_duration = charging_duration * factor;

        EV_charger[EV_charger_last_idx].estimated_charging_duration = estimated_charging_duration;
        EV_charger[EV_charger_last_idx].remaining_time_seconds = estimated_charging_duration;
        
        // Mark charger as not currently charging
        EV_charger[EV_charger_last_idx].is_charging = false;
    }
}


// Define the CoAP resource "res_register" for device registration
RESOURCE(res_car_reg,
        "title=\"Register Car\";rt=\"Control\"",
        NULL,                         // GET handler (not used)
        NULL,                         // POST handler
        res_connection_put_handler,  // PUT handler (not used)
        NULL);                        // DELETE handler (not used)
  
// Handler for POST requests to register a new device
static void res_connection_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    uip_ipaddr_t source_addr;
    uip_ipaddr_copy(&source_addr, &UIP_IP_BUF->srcipaddr);

    uint8_t device_id = get_device_id_by_addr(&source_addr);

    if (device_id > 0) {
        LOG_INFO("Charger ID found: %u\n", device_id);
    } else {
        LOG_INFO("Charger not registered\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
        return;
    }

    size_t len = 0;
    const char *text = NULL;

    float vehicle_max_power;
    uint8_t vehicle_max_capacity;
    float soc_current;
    float soc_target;
    char plate[12] = "";

    int param_count = 0;

    const int idx = device_id - 1; // Adjust for zero-based index

    len = coap_get_post_variable(request, "type", &text);
    if (len > 0) {
        if (strcmp(text, "disconnection") == 0) {
            EV_charger[idx].car_registered = false;
            coap_set_status_code(response, CHANGED_2_04);
            return;
        }
    }

    // carMaxKW
    len = coap_get_post_variable(request, "carMaxKW", &text);
    if (len > 0) {
        vehicle_max_power = strtof(text, NULL);
        param_count++;
    }

    // carMaxCapacity
    len = coap_get_post_variable(request, "carMaxCapacity", &text);
    if (len > 0) {
        vehicle_max_capacity = (unsigned int) strtof(text, NULL);
        param_count++;
    }

    // currentCharge
    len = coap_get_post_variable(request, "currentCharge", &text);
    if (len > 0) {
        soc_current = strtof(text, NULL);
        param_count++;
    }

    // desiredCharge
    len = coap_get_post_variable(request, "desiredCharge", &text);
    if (len > 0) {
        soc_target = strtof(text, NULL);
        param_count++;
    }

    // parkingTime
    len = coap_get_post_variable(request, "plate", &text);
    if (len > 0) {
        strcpy(plate, text);
        param_count++;
    }

    // Only update the device if all four parameters are present
    if (param_count == 5) {
        vehicle_count++;

        EV_charger[idx].car_registered = true;
        EV_charger[idx].vehicle_max_charging_power = vehicle_max_power;
        EV_charger[idx].vehicle_max_capacity = vehicle_max_capacity;
        EV_charger[idx].soc_current = soc_current;
        EV_charger[idx].soc_target = soc_target;
        strcpy(EV_charger[idx].license_plate, plate);

        EV_charger_last_idx = idx;

        LOG_INFO("Vehicle registered with ID %d, Vehicle Max Power: %.2f kW, Vehicle Max Capacity: %u kWh, Current SOC: %.2f %%, Target SOC: %.2f %%, License Plate: %s\n",
            device_id, vehicle_max_power, vehicle_max_capacity, soc_current, soc_target, plate);

        // Request to cloud application to get priority plate
        coap_endpoint_parse("coap://[fd00::1]:5683/", strlen("coap://[fd00::1]:5683/"), &cloud_application_ep);

        coap_message_t request[1];
        char query[64];
        static coap_callback_request_state_t request_state;


        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "plate");

        snprintf(query, sizeof(query), "plate=%s", plate);
        coap_set_header_uri_query(request, query);

        coap_send_request(&request_state, &cloud_application_ep, request, client_response_handler);

        coap_set_status_code(response, CHANGED_2_04);
    } else {
        LOG_INFO("Missing parameters, device not updated\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}


// Register a device by storing its IP address and assigning an ID
static uint8_t get_device_id_by_addr(const uip_ipaddr_t *src_addr) {
    for(uint8_t i = 0; i < device_count; i++) {
        if(uip_ipaddr_cmp(&EV_charger[i].addr, src_addr)) {
            // Found matching IP, return the assigned ID
            return EV_charger[i].id;
        }
    }
    return 0;
}
