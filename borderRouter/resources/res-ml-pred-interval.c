#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "os/dev/leds.h"
#include "../config-ml/config-ml.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

extern struct process border_router;

// Forward declaration of the PUT handler
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_ml_pred_interval,
        "title=\"ML Prediction Interval\";rt=\"text/plain\"",
        NULL,              // GET handler
        NULL,              // POST handler
        res_put_handler,   // PUT handler
        NULL);             // DELETE handler


static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    size_t len = 0;
    const char *text = NULL;

    // Get the "runMLModel" flag from the PUT request
    len = coap_get_post_variable(request, "runMLModel", &text);

    if(len > 0 && text != NULL) {
        int runFlag = atoi(text);
        if(runFlag == 0 || runFlag == 1) {
            run_ml_model = runFlag;  // Set the global flag

            if(!run_ml_model) {
                // Stop ML prediction timer if flag is 0
                etimer_stop(&e_timer_ml_pred);
                process_post(&border_router, PROCESS_EVENT_TIMER, &e_timer_ml_pred);

                const char *msg = "ML model stopped.";
                coap_set_header_content_format(response, TEXT_PLAIN);
                coap_set_payload(response, msg, strlen(msg));
                coap_set_status_code(response, CHANGED_2_04);

                LOG_INFO("ML model stopped!\n");
                return;
            }
        } else {
            const char *error_msg = "Invalid runMLModel value. Must be 0 or 1.";
            coap_set_header_content_format(response, TEXT_PLAIN);
            coap_set_payload(response, error_msg, strlen(error_msg));
            coap_set_status_code(response, BAD_REQUEST_4_00);
            return;
        }
    }

    // Only check mlPredInterval if runMLModel is true
    if(run_ml_model) {
        len = coap_get_post_variable(request, "mlPredInterval", &text);

        if(len > 0 && text != NULL) {
            int new_interval = atoi(text);

            if(new_interval < ml_min_pred_interval) {
                char error_msg[64];
                snprintf(error_msg, sizeof(error_msg), "Interval too small. Must be >= %d", ml_min_pred_interval);
                coap_set_header_content_format(response, TEXT_PLAIN);
                coap_set_payload(response, error_msg, strlen(error_msg));
                coap_set_status_code(response, BAD_REQUEST_4_00);
            } else {
                ml_pred_interval = new_interval;
                etimer_stop(&e_timer_ml_pred);
                etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);
                process_post(&border_router, PROCESS_EVENT_TIMER, &e_timer_ml_pred);
                
                char success_msg[64];
                snprintf(success_msg, sizeof(success_msg), "ML prediction interval set to %d seconds", ml_pred_interval);
                coap_set_header_content_format(response, TEXT_PLAIN);
                coap_set_payload(response, success_msg, strlen(success_msg));
                coap_set_status_code(response, CHANGED_2_04);

                LOG_INFO("ML prediction interval set to %d seconds\n", ml_pred_interval);
            }
        }
        else {  // No mlPredInterval provided, just restart the timer with the last interval
            etimer_stop(&e_timer_ml_pred);
            etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);
            process_post(&border_router, PROCESS_EVENT_TIMER, &e_timer_ml_pred);

            char success_msg[64];
            snprintf(success_msg, sizeof(success_msg), "ML model started with interval %d seconds", ml_pred_interval);
            coap_set_header_content_format(response, TEXT_PLAIN);
            coap_set_payload(response, success_msg, strlen(success_msg));
            coap_set_status_code(response, CHANGED_2_04);

            LOG_INFO("ML model started!\n");
        }
    }
}