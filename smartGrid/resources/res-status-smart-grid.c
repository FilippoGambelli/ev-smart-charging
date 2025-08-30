#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "time.h"
#include "os/dev/leds.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

// Helper macros to handle LEDs differently in Cooja vs real hardware
#ifdef COOJA
  #define LED_ON(led) leds_on(LEDS_NUM_TO_MASK(led))
  #define LED_OFF(led) leds_off(LEDS_NUM_TO_MASK(led))
#else
  #define LED_ON(led) leds_on(led)
  #define LED_OFF(led) leds_off(led)
#endif

static float power_from_grid_now = 0.0;
static float power_to_grid_now = 0.0;
static float energy_from_grid_total = 0.0;
static float energy_to_grid_total = 0.0;

static unsigned long last_execution = 0;

// Forward declarations
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Define the CoAP resource
RESOURCE(res_status_smart_grid,
        "title=\"Status Smart Grid\";rt=\"status-smart-grid\";get_format=\"application/json\";put_format=\"text/plain\"",
        res_get_handler,   // GET handler
        NULL,              // POST handler
        res_put_handler,   // PUT handler
        NULL);             // DELETE handler

// GET handler: returns the status in JSON format
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    int length = snprintf((char *)buffer, preferred_size,
        "{"
        "\"power_from_grid\":%d.%04d,"
        "\"power_to_grid\":%d.%04d,"
        "\"energy_from_grid_total\":%d.%04d,"
        "\"energy_to_grid_total\":%d.%04d"
        "}",
        (int)power_from_grid_now, (int)((power_from_grid_now - (int)power_from_grid_now) * 10000),
        (int)power_to_grid_now, (int)((power_to_grid_now - (int)power_to_grid_now) * 10000),
        (int)energy_from_grid_total, (int)((energy_from_grid_total - (int)energy_from_grid_total) * 10000),
        (int)energy_to_grid_total, (int)((energy_to_grid_total - (int)energy_to_grid_total) * 10000)
    );

    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, buffer, length);
    coap_set_status_code(response, CONTENT_2_05);

    LOG_INFO("Received GET - Status Power Grid - Data sent: Power from grid: %d,%04d kW | Power to grid: %d,%04d kW | Energy from grid total: %d,%04d kWh | Energy to grid total: %d,%04d kWh\n",
        (int)power_from_grid_now, (int)((power_from_grid_now - (int)power_from_grid_now) * 10000),
        (int)power_to_grid_now, (int)((power_to_grid_now - (int)power_to_grid_now) * 10000),
        (int)energy_from_grid_total, (int)((energy_from_grid_total - (int)energy_from_grid_total) * 10000),
        (int)energy_to_grid_total, (int)((energy_to_grid_total - (int)energy_to_grid_total) * 10000)
    );
}

// PUT handler: updates values from request
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    size_t len = 0;
    const char *text = NULL;

    unsigned long now = clock_seconds();
    unsigned long elapsed_time;

    if (last_execution == 0) {  // First execution
        elapsed_time = 0;
    } else {
        elapsed_time = now - last_execution;
    }
    last_execution = now;

    // Update power_from_grid_now
    len = coap_get_post_variable(request, "power_from_grid_now", &text);
    if(len > 0) {
        energy_from_grid_total += power_from_grid_now * (elapsed_time / 3600.0); 
        power_from_grid_now = strtof(text, NULL);
    }

    // Update power_to_grid_now
    len = coap_get_post_variable(request, "power_to_grid_now", &text);
    if(len > 0) {
        energy_to_grid_total += power_to_grid_now * (elapsed_time / 3600.0);       
        power_to_grid_now = strtof(text, NULL);
    }

    LED_OFF(LEDS_ALL);

    if(power_to_grid_now > 0) {
        LED_ON(LEDS_GREEN);
    } else if (power_from_grid_now > 0){
        LED_ON(LEDS_RED);
    }

    LOG_INFO("Received PUT - Status Power Grid - Data updated: Power from grid: %d,%04d kW | Power to grid: %d,%04d kW | Energy from grid total: %d,%04d kWh | Energy to grid total: %d,%04d kWh\n",
        (int)power_from_grid_now, (int)((power_from_grid_now - (int)power_from_grid_now) * 10000),
        (int)power_to_grid_now, (int)((power_to_grid_now - (int)power_to_grid_now) * 10000),
        (int)energy_from_grid_total, (int)((energy_from_grid_total - (int)energy_from_grid_total) * 10000),
        (int)energy_to_grid_total, (int)((energy_to_grid_total - (int)energy_to_grid_total) * 10000)
    );
}