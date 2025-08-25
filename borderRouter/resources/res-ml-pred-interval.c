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
    int status_code = CHANGED_2_04;

    len = coap_get_post_variable(request, "mlPredInterval", &text);
    if(len > 0 && text != NULL) {
        int new_interval = atoi(text);

        if(new_interval < ml_min_pred_interval) {
            status_code = BAD_REQUEST_4_00;
        } else {
            ml_pred_interval = new_interval;
        }
    }

    len = coap_get_post_variable(request, "runMLModel", &text);
    if(len > 0 && text != NULL) {
        int runFlag = atoi(text);
        if(runFlag == 0 || runFlag == 1) {
            run_ml_model = runFlag;
        } else {
            status_code = BAD_REQUEST_4_00;
        }
    }

    if(run_ml_model) {
        etimer_stop(&e_timer_ml_pred);
        etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);
        process_post(&border_router, PROCESS_EVENT_TIMER, &e_timer_ml_pred);
        LOG_INFO("ML model RUN with interval %d seconds\n", ml_pred_interval);
    } else {
        etimer_stop(&e_timer_ml_pred);
        process_post(&border_router, PROCESS_EVENT_TIMER, &e_timer_ml_pred);
        LOG_INFO("ML model stopped!\n");
    }

    coap_set_status_code(response, status_code);
}
