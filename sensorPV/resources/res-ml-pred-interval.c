#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "os/dev/leds.h"
#include "../config-ml/config-ml.h"

// Forward declaration of the PUT handler
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Define the CoAP resource
RESOURCE(res_ml_pred_interval,
         "title=\"ML Prediction Interval\";rt=\"control\"",
         NULL,              // GET handler
         NULL,              // POST handler
         res_put_handler,   // PUT handler
         NULL);             // DELETE handler

static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    size_t len = 0;
    const char *text = NULL;

    // Get the "mlPredInterval" variable from the PUT request
    len = coap_get_post_variable(request, "mlPredInterval", &text);

    if(len > 0 && text != NULL) {
        int new_interval = atoi(text);

        if(new_interval < ml_min_pred_interval) {
            // Respond with error if below the minimum allowed value
            char error_msg[64];
            snprintf(error_msg, sizeof(error_msg), "Interval too small. Must be >= %d", ml_min_pred_interval);

            coap_set_payload(response, error_msg, strlen(error_msg));
            coap_set_status_code(response, BAD_REQUEST_4_00);
        } else {
            // Update the global variable
            ml_pred_interval = new_interval;

            char success_msg[64];
            snprintf(success_msg, sizeof(success_msg), "ML prediction interval set to %d seconds", ml_pred_interval);

            coap_set_payload(response, success_msg, strlen(success_msg));
            coap_set_status_code(response, CHANGED_2_04);
        }
    } else {
        // No variable found in the request
        const char *error_msg = "Missing mlPredInterval parameter.";
        coap_set_payload(response, error_msg, strlen(error_msg));
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}