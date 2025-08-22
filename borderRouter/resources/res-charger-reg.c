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

// Function prototypes
static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static uint8_t register_device(const uip_ipaddr_t *src_addr);

static void res_register_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

struct charging_station EV_charger[MAX_DEVICES];
uint8_t device_count = 0;
uint8_t vehicle_count = 0;

// Define the CoAP resource "res_register" for device registration
RESOURCE(res_charger_reg,
        "title=\"Register Charger\";rt=\"Control\"",
        res_register_get_handler,   // GET handler (not used)
        res_register_post_handler,  // POST handler
        NULL,                       // PUT handler (not used)
        NULL);                      // DELETE handler (not used)

// Handler for POST requests to register a new device
static void res_register_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    uip_ipaddr_t src_addr;
    // Get the source IP address of the device making the request
    uip_ipaddr_copy(&src_addr, &UIP_IP_BUF->srcipaddr);

    // Attempt to register the device and get its assigned ID
    uint8_t device_id = register_device(&src_addr);

    if (device_id == 0) {
        // Registration failed (list full)
        LOG_INFO("Charger registration failed, no available slots\n");
        coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03); // HTTP 503 Service Unavailable
        return;
    }

    const int idx = device_id - 1; // Adjust for zero-based index

    // Check for maxKW parameter
    size_t len = 0;
    const char *text = NULL;
    len = coap_get_post_variable(request, "maxKW", &text);

    if (len > 0) {
        float maxKW_val = strtof(text, NULL);
        EV_charger[idx].max_charging_power = maxKW_val; // Store the maximum charging power
        EV_charger[idx].car_registered = false;

        // Return the assigned ID in the response payload
        char id_str[4];
        snprintf(id_str, sizeof(id_str), "%u", device_id);
        memcpy(buffer, id_str, strlen(id_str));
        coap_set_payload(response, buffer, strlen(id_str));

        LOG_INFO("Charger registered with ID %d, Max Power: %.2f kW\n", device_id, maxKW_val);
        coap_set_status_code(response, CREATED_2_01); // HTTP 201 Created
    } else {
        LOG_INFO("Charger registered with ID %d, Max Power not specified\n", device_id);
        coap_set_status_code(response, BAD_REQUEST_4_00); // HTTP 400 Bad Request
    }
}

static void res_register_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    int len = 0;
    char json_buf[1024]; // temporary buffer for JSON
    int pos = 0;

    // Start JSON array
    pos += snprintf(&json_buf[pos], sizeof(json_buf) - pos, "[");

    // Iterate over all registered devices
    for (uint8_t i = 0; i < device_count; i++) {
        struct charging_station *cs = &EV_charger[i];

        // Add comma separator if not the first element
        if (i > 0) {
            pos += snprintf(&json_buf[pos], sizeof(json_buf) - pos, ",");
        }

        // Build JSON object for each charger (compact version)
        pos += snprintf(&json_buf[pos], sizeof(json_buf) - pos,
            "{"
            "\"id\":%u,"
            "\"maxP\":%.2f,"
            "\"carReg\":%s,"
            "\"isCh\":%s,"
            "\"aP\":%.2f,"
            "\"gP\":%.2f,"
            "\"vMaxP\":%.2f,"
            "\"vCap\":%u,"
            "\"socCur\":%.2f,"
            "\"socTgt\":%.2f,"
            "\"plate\":\"%s\","
            "\"prio\":%d,"
            "\"estDur\":%lu,"
            "\"remT\":%lu,"
            "\"remE\":%.2f"
            "}",
            cs->id,
            cs->max_charging_power,
            cs->car_registered ? "true" : "false",
            cs->is_charging ? "true" : "false",
            cs->assigned_power,
            cs->grid_power_used,
            cs->vehicle_max_charging_power,
            cs->vehicle_max_capacity,
            cs->soc_current,
            cs->soc_target,
            cs->license_plate,
            cs->priority,
            (unsigned long)cs->estimated_charging_duration,
            (unsigned long)cs->remaining_time_seconds,
            cs->remaining_energy
        );

    }

    // Close JSON array
    pos += snprintf(&json_buf[pos], sizeof(json_buf) - pos, "]");

    // Copy into CoAP response buffer
    len = strlen(json_buf);
    if(len > preferred_size) {
        len = preferred_size; // truncate if too big
    }
    memcpy(buffer, json_buf, len);

    // Set response
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, len);
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