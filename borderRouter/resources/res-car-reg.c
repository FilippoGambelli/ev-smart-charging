#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "info-charger-station.h"
#include <math.h>
#include <stdint.h> 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

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

    float vehicle_max_power = NAN;
    float soc_current = NAN;
    float soc_target = NAN;
    uint32_t parking_time = UINT32_MAX;

    int param_count = 0;

    // carMaxKW
    len = coap_get_post_variable(request, "carMaxKW", &text);
    if (len > 0) {
        vehicle_max_power = strtof(text, NULL);
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
    len = coap_get_post_variable(request, "parkingTime", &text);
    if (len > 0) {
        parking_time = (uint32_t)strtoul(text, NULL, 10);
        param_count++;
    }

    const int idx = device_id - 1; // Adjust for zero-based index

    // Only update the device if all four parameters are present
    if (param_count == 4) {
        devices[idx].vehicle_max_charging_power = vehicle_max_power;
        devices[idx].soc_current = soc_current;
        devices[idx].soc_target = soc_target;
        devices[idx].parking_time_minutes = parking_time;

        LOG_INFO("Vehicle registered with ID %d, Vehicle Max Power: %.2f kW, Current SOC: %.2f %%, Target SOC: %.2f %%, Parking Time: %u minutes\n",
                 device_id, vehicle_max_power, soc_current, soc_target, parking_time);

        coap_set_status_code(response, CREATED_2_01);
    } else {
        LOG_INFO("Missing parameters, device not updated\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}


// Register a device by storing its IP address and assigning an ID
static uint8_t get_device_id_by_addr(const uip_ipaddr_t *src_addr) {
    for(uint8_t i = 0; i < device_count; i++) {
        if(uip_ipaddr_cmp(&devices[i].addr, src_addr)) {
            // Found matching IP, return the assigned ID
            return devices[i].id;
        }
    }
    return 0;
}
