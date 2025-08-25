#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "info-charging-station.h"
#include <math.h>
#include <stdint.h> 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_register_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static uint8_t register_device(const uip_ipaddr_t *src_addr);


struct charging_station EV_charger[MAX_DEVICES];
uint8_t device_count = 0;
uint8_t vehicle_count = 0;

RESOURCE(res_charger_reg,
        "title=\"Register Charger\";rt=\"register_charger\";get_format=\"application/json\";post_format=\"text/plain\"",
        res_register_get_handler,   // GET handler
        res_register_post_handler,  // POST handler
        NULL,                       // PUT handler 
        NULL);                      // DELETE handler

// Handler for POST requests to register a new device
static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    uip_ipaddr_t src_addr;
    uip_ipaddr_copy(&src_addr, &UIP_IP_BUF->srcipaddr); // Get the source IP address of the device making the request
    uint8_t device_id = register_device(&src_addr);     // Attempt to register the device and get its assigned ID

    if (device_id == 0) {
        // Registration failed
        LOG_INFO("Charger registration failed, no available slots\n");
        coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
        return;
    }

    const int idx = device_id - 1; // Adjust for zero-based index, because device_id starts from 1

    // Check for maxW parameter
    size_t len = 0;
    const char *text = NULL;
    len = coap_get_post_variable(request, "maxPower", &text);

    if (len > 0) {
        float maxW_value = strtof(text, NULL);
        EV_charger[idx].max_charging_power = maxW_value; // Store the maximum charging power
        EV_charger[idx].car_registered = false;

        int offset_buf = snprintf((char *)buffer, preferred_size, "ChargerID=%u", device_id);

        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_payload(response, buffer, offset_buf);
        coap_set_status_code(response, CREATED_2_01);

        LOG_INFO("Charger registered with ID %d | Max Power: %d,%04d kW\n", device_id, (int)maxW_value, (int)((maxW_value - (int)maxW_value) * 10000));
    } else {
        LOG_INFO("Charger registered with ID %d, Max Power not specified\n", device_id);
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}

static void res_register_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    int pos = 0;

    // Start the JSON array
    pos += snprintf((char *)buffer + pos, preferred_size - pos, "[");

    // Iterate over all registered devices
    for (uint8_t i = 0; i < device_count; i++) {
        struct charging_station *cs = &EV_charger[i];

        // Add a comma separator if this is not the first element
        if (i > 0) {
            pos += snprintf((char *)buffer + pos, preferred_size - pos, ",");
        }

        // Append JSON object for the current charging station
        pos += snprintf((char *)buffer + pos, preferred_size - pos,
            "{\"id\":%u,\"maxP\":%d.%04d,\"carReg\":%s,\"isCh\":%s,\"aP\":%d.%04d,\"gP\":%d.%04d,\"vMaxP\":%d.%04d,"
            "\"vCap\":%d.%04d,\"socCur\":%d.%04d,\"socTgt\":%d.%04d,\"plate\":\"%s\",\"prio\":%d,"
            "\"estDur\":%d,\"remT\":%d,\"remE\":%d.%04d}",
            cs->id,
            (int)cs->max_charging_power, (int)((cs->max_charging_power - (int)cs->max_charging_power) * 10000),
            cs->car_registered ? "true" : "false",
            cs->is_charging ? "true" : "false", 
            (int)cs->assigned_power, (int)((cs->assigned_power - (int)cs->assigned_power) * 10000),
            (int)cs->grid_power_used, (int)((cs->grid_power_used - (int)cs->grid_power_used) * 10000),
            (int)cs->vehicle_max_charging_power, (int)((cs->vehicle_max_charging_power - (int)cs->vehicle_max_charging_power) * 10000),
            (int)cs->vehicle_max_capacity, (int)((cs->vehicle_max_capacity - (int)cs->vehicle_max_capacity) * 10000),
            (int)cs->soc_current, (int)((cs->soc_current - (int)cs->soc_current) * 10000),
            (int)cs->soc_target, (int)((cs->soc_target - (int)cs->soc_target) * 10000),
            cs->license_plate, cs->priority, cs->estimated_charging_duration,
            cs->remaining_time_seconds,
            (int)cs->remaining_energy, (int)((cs->remaining_energy - (int)cs->remaining_energy) * 10000));


        // Stop writing if buffer space is exhausted
        if (pos >= preferred_size) {
            pos = preferred_size;
            break;
        }
    }

    // Close the JSON array (only if there is still space left)
    if (pos < preferred_size) {
        pos += snprintf((char *)buffer + pos, preferred_size - pos, "]");
    }

    // Set CoAP response headers and payload
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, pos);
    coap_set_status_code(response, CONTENT_2_05);

    LOG_INFO("Received GET - Charger Registration - All Chargers Info Sent\n");
}

// Register a device by storing its IP address and assigning an ID
static uint8_t register_device(const uip_ipaddr_t *src_addr) {
    if(device_count < MAX_DEVICES) {
        uint8_t new_id = device_count + 1;
        EV_charger[device_count].id = new_id;
        uip_ipaddr_copy(&EV_charger[device_count].addr, src_addr);
        device_count++;

        return new_id;
    } else {
        return 0; // Registration failed
    }
}