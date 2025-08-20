#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "os/dev/leds.h"

// Helper macros to handle LEDs differently in Cooja vs real hardware
#ifdef COOJA
  #define LED_ON(led) leds_on(LEDS_NUM_TO_MASK(led))
  #define LED_OFF(led) leds_off(LEDS_NUM_TO_MASK(led))
  #define LED_SINGLE_ON(led) leds_single_on(led)
  #define LED_SINGLE_OFF(led) leds_single_off(led)
#else
  #define LED_ON(led) leds_on(led)
  #define LED_OFF(led) leds_off(led)
  #define LED_SINGLE_ON(led) leds_single_on(led)
  #define LED_SINGLE_OFF(led) leds_single_off(led)
#endif

static float charging_power = 0;
static bool energy_renewable = false;
static bool charging_complete = false;

// Forward declaration of the PUT handler
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Define the CoAP resource
RESOURCE(res_charging_status,
         "title=\"Charging Status\";rt=\"charging-status\"",
         NULL,   // GET handler
         NULL,   // POST handler
         res_put_handler, // PUT handler
         NULL);  // DELETE handler

static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    size_t len = 0;
    const char *text = NULL;

    // Get chargingPower from the request
    len = coap_get_post_variable(request, "chargingPower", &text);
    if(len > 0) {
        charging_power = strtof(text, NULL);
    }

    // Get energyRenewable flag
    len = coap_get_post_variable(request, "energyRenewable", &text);
    if(len > 0) {
        energy_renewable = (text[0] == '1');
    }

    // Get chargingComplete flag
    len = coap_get_post_variable(request, "chargingComplete", &text);
    if(len > 0) {
        charging_complete = (text[0] == '1');
    }

    LED_OFF(LEDS_ALL);
    LED_SINGLE_OFF(LEDS_YELLOW);

    if(charging_complete) {
        LED_ON(LEDS_GREEN);
    } else {
        if(charging_power == 0) {
            LED_ON(LEDS_BLUE);
        } else {
            LED_ON(LEDS_RED);
        }

        if(energy_renewable) {
            LED_SINGLE_ON(LEDS_YELLOW);
        }
    }
}