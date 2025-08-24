#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "coap-blocking-api.h"
#include "coap-endpoint.h"
#include "coap-callback-api.h"
#include "power-manager/power-manager.h"
#include <math.h>
#include <stdint.h> 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

static coap_endpoint_t cloud_application_ep;

static int EV_charger_last_idx = -1;

#define PRIORITY_CHARGING_FACTOR 1.4 // Factor for priority charging
#define STANDARD_CHARGING_FACTOR 1.7 // Factor for standard charging

static void res_connection_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static int get_device_idx_by_addr(const uip_ipaddr_t *src_addr);
static void cloud_response_handler(coap_callback_request_state_t *callback_state);


RESOURCE(res_car_reg,
        "title=\"Register Car\";rt=\"text/plain\"",
        NULL,                           // GET handler
        NULL,                           // POST handler
        res_connection_put_handler,     // PUT handler
        NULL);                          // DELETE handler

        
// Handler for POST requests to register a new device
static void res_connection_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    uip_ipaddr_t source_addr;
    uip_ipaddr_copy(&source_addr, &UIP_IP_BUF->srcipaddr);

    int idx = get_device_idx_by_addr(&source_addr);

    if (idx >= 0 && EV_charger[idx].id > 0) {
        LOG_INFO("Charger ID found: %u\n", EV_charger[idx].id);
    } else {
        LOG_INFO("Charger not registered\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
        return;
    }

    size_t len = 0;
    const char *text = NULL;

    int vehicle_max_power = 0;
    int vehicle_max_capacity = 0;
    float soc_current = 0.0;
    int soc_target = 0;
    char plate[12] = "";

    int param_count = 0;

    len = coap_get_post_variable(request, "type", &text);
    if (len > 0) {
        if (strcmp(text, "disconnection") == 0) {
            EV_charger[idx].car_registered = false;
            EV_charger[idx].is_charging = false;
            coap_set_status_code(response, CHANGED_2_04);
            vehicle_count--;

            // New vehicle disconnected, update charging stations
            power_manager_update_charging_station();

            return;
        }
    }

    // carMaxW
    len = coap_get_post_variable(request, "carMaxPower", &text);
    if (len > 0) {
        vehicle_max_power = atoi(text);
        param_count++;
    }

    // carMaxCapacity
    len = coap_get_post_variable(request, "carMaxCapacity", &text);
    if (len > 0) {
        vehicle_max_capacity = atoi(text);
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
        soc_target = atoi(text);
        param_count++;
    }

    // plate
    len = coap_get_post_variable(request, "plate", &text);
    if (len > 0) {
        strcpy(plate, text);
        param_count++;
    }

    // Only update the device if all five parameters are present
    if (param_count == 5) {
        vehicle_count++;
        EV_charger[idx].car_registered = true;
        EV_charger[idx].is_charging = false;

        EV_charger[idx].vehicle_max_charging_power = vehicle_max_power;
        EV_charger[idx].vehicle_max_capacity = vehicle_max_capacity;
        EV_charger[idx].soc_current = soc_current;
        EV_charger[idx].soc_target = soc_target;
        strcpy(EV_charger[idx].license_plate, plate);
            
        // Request to cloud application to get priority plate
        coap_endpoint_parse("coap://[fd00::1]:5683/", strlen("coap://[fd00::1]:5683/"), &cloud_application_ep);

        static coap_message_t request[1];
        static char query[32];
        static coap_callback_request_state_t request_state;

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "plate");
        
        snprintf(query, sizeof(query), "plate=%s", plate);
        coap_set_header_uri_query(request, query);
        
        EV_charger_last_idx = idx;              // Store the index for use in the response handler

        // Asynchronous request to cloud application
        coap_send_request(&request_state, &cloud_application_ep, request, cloud_response_handler);

        coap_set_status_code(response, CHANGED_2_04);
    } else {
        LOG_INFO("Missing parameters, device not updated\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}


// Register a device by storing its IP address and assigning an ID
static int get_device_idx_by_addr(const uip_ipaddr_t *src_addr) {
    for(int i = 0; i < device_count; i++) {
        if(uip_ipaddr_cmp(&EV_charger[i].addr, src_addr)) {
            // Found matching IP, return the assigned ID
            return i;
        }
    }
    return -1;
}


static void cloud_response_handler(coap_callback_request_state_t *callback_state) {
    // Get the CoAP response from the callback state
    coap_message_t *response = callback_state->state.response;

    if(!response) {
        return;
    } else {
        const uint8_t *chunk;
        int len = coap_get_payload(response, &chunk);
        
        if(len > 0) {
            // Expected response: "priority=1" or "priority=0"
            if(strstr((const char *)chunk, "priority=1") != NULL) {
                EV_charger[EV_charger_last_idx].priority = 1;
            } else {
                EV_charger[EV_charger_last_idx].priority = 0;
            }
        }
    }

    // Calculate energy needed (Wh) to reach target SOC
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
    float factor = EV_charger[EV_charger_last_idx].priority ? PRIORITY_CHARGING_FACTOR : STANDARD_CHARGING_FACTOR;

    // Estimated charging duration in seconds
    float estimated_charging_duration = charging_duration * factor;

    EV_charger[EV_charger_last_idx].estimated_charging_duration = (int)estimated_charging_duration;
    EV_charger[EV_charger_last_idx].remaining_time_seconds = (int)estimated_charging_duration;

    LOG_INFO("Vehicle registered - ID: %d | Vehicle Max Power: %d W | Vehicle Capacity: %d Wh | SOC: %d.%d %% | Target SOC: %d %% | Plate: %s | Priority: %d | Remaining Charging Duration: %d s\n",
    EV_charger[EV_charger_last_idx].id, EV_charger[EV_charger_last_idx].vehicle_max_charging_power, EV_charger[EV_charger_last_idx].vehicle_max_capacity,
    (int)EV_charger[EV_charger_last_idx].soc_current, (int)((EV_charger[EV_charger_last_idx].soc_current - (int)EV_charger[EV_charger_last_idx].soc_current) * 10),
    EV_charger[EV_charger_last_idx].soc_target, EV_charger[EV_charger_last_idx].license_plate, EV_charger[EV_charger_last_idx].priority, EV_charger[EV_charger_last_idx].remaining_time_seconds);


    // New vehicle registered, update charging stations
    power_manager_update_charging_station();
}
