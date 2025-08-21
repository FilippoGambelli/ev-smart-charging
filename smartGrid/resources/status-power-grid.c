#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "coap-engine.h"

static float energy_from_grid_now = 0;
static float energy_to_grid_now = 0;
static float energy_from_grid_total = 0;
static float energy_to_grid_total = 0;

// Forward declarations
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Define the CoAP resource
RESOURCE(res_status_power_grid,
         "title=\"Status Power Grid\";rt=\"status-power-grid\"",
         res_get_handler,   // GET handler
         NULL,              // POST handler
         res_put_handler,   // PUT handler
         NULL);             // DELETE handler

// GET handler: returns the status in JSON format
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    setlocale(LC_NUMERIC, "C");

    int length = snprintf((char *)buffer, preferred_size,
        "{"
        "\"energy_from_grid_now\":%.2f,"
        "\"energy_to_grid_now\":%.2f,"
        "\"energy_from_grid_total\":%.2f,"
        "\"energy_to_grid_total\":%.2f"
        "}",
        energy_from_grid_now, energy_to_grid_now, energy_from_grid_total, energy_to_grid_total
    );

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
}

// PUT handler: updates values from request
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    size_t len = 0;
    const char *text = NULL;

    // Update energy_from_grid_now
    len = coap_get_post_variable(request, "energy_from_grid_now", &text);
    if(len > 0) {
        energy_from_grid_now = strtof(text, NULL);
        energy_from_grid_total += energy_from_grid_now;
    }

    // Update energy_to_grid_now
    len = coap_get_post_variable(request, "energy_to_grid_now", &text);
    if(len > 0) {
        energy_to_grid_now = strtof(text, NULL);
        energy_to_grid_total += energy_to_grid_now;
    }
}