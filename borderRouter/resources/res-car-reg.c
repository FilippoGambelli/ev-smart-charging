#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "coap-blocking-api.h"
#include "coap-endpoint.h"
#include "info-charging-station.h"
#include <math.h>
#include <stdint.h> 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define CLOUD_APPLICATION "coap://[fd00::202:2:2:2]:5683"

#define PRIORITY_CHARGING_FACTOR 1.4 // Factor for priority charging
#define STANDARD_CHARGING_FACTOR 1.7 // Factor for standard charging


// Function prototypes
static void res_connection_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static uint8_t get_device_id_by_addr(const uip_ipaddr_t *src_addr);

// Define the CoAP resource "res_register" for device registration
RESOURCE(res_car_reg,
        "title=\"Register Car\";rt=\"Control\"",
        NULL,                         // GET handler (not used)
        res_connection_post_handler,  // POST handler
        NULL,                         // PUT handler (not used)
        NULL);                        // DELETE handler (not used)
  
// Handler for POST requests to register a new device
static void res_connection_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
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

    const int idx = device_id - 1; // Adjust for zero-based index

    // Only update the device if all four parameters are present
    if (param_count == 5) {
        vehicle_count++;

        EV_charger[idx].vehicle_max_charging_power = vehicle_max_power;
        EV_charger[idx].vehicle_max_capacity = vehicle_max_capacity;
        EV_charger[idx].soc_current = soc_current;
        EV_charger[idx].soc_target = soc_target;
        strcpy(EV_charger[idx].license_plate, plate);

        LOG_INFO("Vehicle registered with ID %d, Vehicle Max Power: %.2f kW, Vehicle Max Capacity: %u kWh, Current SOC: %.2f %%, Target SOC: %.2f %%, License Plate: %s\n",
            device_id, vehicle_max_power, vehicle_max_capacity, soc_current, soc_target, plate);


        EV_charger[idx].priority = 1; // Assume priority for this example

        // Calculate estimated charging duration
        float energy_needed = (soc_target - soc_current) / 100 * vehicle_max_capacity; // kWh needed to reach target SOC
        float effective_power = (vehicle_max_power < EV_charger[idx].max_charging_power)
                                    ? vehicle_max_power
                                    : EV_charger[idx].max_charging_power; // Use the lower of vehicle max power or station max power

        float charging_duration = (energy_needed / effective_power) * 3600; // seconds

        float factor = EV_charger[idx].priority ? PRIORITY_CHARGING_FACTOR : STANDARD_CHARGING_FACTOR;

        float estimated_charging_duration = charging_duration * factor; //seconds

        EV_charger[idx].estimated_charging_duration = estimated_charging_duration;
        EV_charger[idx].remaining_time_seconds = estimated_charging_duration;

        EV_charger[idx].is_charging = false;


        // Return the estimated_charging_duration
        unsigned int duration_minutes = (unsigned int)(estimated_charging_duration / 60.0f);
        coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%u", (unsigned int) duration_minutes));
        coap_set_status_code(response, CREATED_2_01);
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
