#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "time.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

static int energy_from_grid_now = 0;
static int energy_to_grid_now = 0;
static int energy_from_grid_total = 0;
static int energy_to_grid_total = 0;

static int last_execution = 0;

// Forward declarations
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Define the CoAP resource
RESOURCE(res_status_power_grid,
        "title=\"Status Power Grid\";rt=\"status-power-grid\";get_format=\"application/json\";put_format=\"text/plain\"",
        res_get_handler,   // GET handler
        NULL,              // POST handler
        res_put_handler,   // PUT handler
        NULL);             // DELETE handler

// GET handler: returns the status in JSON format
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    int length = snprintf((char *)buffer, preferred_size,
        "{"
        "\"power_from_grid\":%d,"
        "\"power_to_grid\":%d,"
        "\"energy_from_grid_total\":%d,"
        "\"energy_to_grid_total\":%d"
        "}",
        energy_from_grid_now, energy_to_grid_now, energy_from_grid_total, energy_to_grid_total
    );

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
    coap_set_status_code(response, CONTENT_2_05);

    LOG_INFO("Received GET - Status Power Grid - Data sent: power_from_grid=%d W | power_to_grid=%d W| energy_from_grid_total=%d Wh | energy_to_grid_total=%d Wh\n",
        energy_from_grid_now, energy_to_grid_now, energy_from_grid_total, energy_to_grid_total);
}

// PUT handler: updates values from request
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    size_t len = 0;
    const char *text = NULL;

    int elapsed_time = 0;
    time_t now = time(NULL);

    if (last_execution == 0) {   // First execution
        elapsed_time = 0;
    } else {
        elapsed_time = difftime(now, last_execution);
    }
    // Update last execution time
    last_execution = now;

    // Update energy_from_grid_now
    len = coap_get_post_variable(request, "energy_from_grid_now", &text);
    if(len > 0) {
        energy_from_grid_total += energy_from_grid_now * (elapsed_time / 3600.0); //Wh
        energy_from_grid_now = strtof(text, NULL);
    }

    // Update energy_to_grid_now
    len = coap_get_post_variable(request, "energy_to_grid_now", &text);
    if(len > 0) {
        energy_to_grid_total += energy_to_grid_now * (elapsed_time / 3600.0);       //Wh
        energy_to_grid_now = strtof(text, NULL);
    }

    LOG_INFO("Received PUT - Status Power Grid - Data updated: power_from_grid=%d W | power_to_grid=%d W | energy_from_grid_total=%d Wh | energy_to_grid_total=%d Wh\n",
        energy_from_grid_now, energy_to_grid_now, energy_from_grid_total, energy_to_grid_total);
}