#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "power-manager/power-manager.h"
#include "ipv6.h"
#include "config-ml/config-ml.h"
#include "coap-callback-api.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define RES_POWER_OBS_URI "real_power_obs"  // Resource on server to observe

// Real dichiaration of extern variables
float power_PV_real;
int power_PV_trend;
float power_PV_pred;

static coap_observee_t *obs = NULL;  // Keeps track of the current observation

extern coap_resource_t res_car_reg;
extern coap_resource_t res_charger_reg;
extern coap_resource_t res_ml_pred_interval;

static void response_handler_pred_power(coap_callback_request_state_t *callback_state);
static void notification_realpower_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag);

PROCESS(border_router, "CoAP Border Router");
AUTOSTART_PROCESSES(&border_router);

PROCESS_THREAD(border_router, ev, data) {
    static coap_endpoint_t sensor_pv_ep;

    PROCESS_BEGIN();

    coap_activate_resource(&res_charger_reg, "registration/charger");
    coap_activate_resource(&res_car_reg, "registration/car");
    coap_activate_resource(&res_ml_pred_interval, "ml_pred_interval");

    LOG_INFO("CoAP resources activated\n");

    
    // Parse server endpoint string into coap_endpoint_t structure
    coap_endpoint_parse(SENSOR_PV_EP, strlen(SENSOR_PV_EP), &sensor_pv_ep);
    
    LOG_INFO("Starting CoAP observation...\n");

    obs = coap_obs_request_registration(&sensor_pv_ep, RES_POWER_OBS_URI, notification_realpower_callback, NULL);
    if(obs == NULL) {
        LOG_INFO("Error registering observation\n");
    }


    etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);

    static coap_message_t request[1];
    static coap_callback_request_state_t request_state;

    while(1) {
        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_TIMER && data == &e_timer_ml_pred){
            if(run_ml_model) {
                coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
                coap_set_header_uri_path(request, "pred_power");
                coap_send_request(&request_state, &sensor_pv_ep, request, response_handler_pred_power);
                LOG_INFO("Sent GET request to obtain Predicted Power\n");
                etimer_set(&e_timer_ml_pred, CLOCK_SECOND * ml_pred_interval);
            }
        }
    }

    PROCESS_END();
}


static void response_handler_pred_power(coap_callback_request_state_t *callback_state) {
    coap_message_t *response = callback_state->state.response;
    if(response != NULL) {
        const uint8_t *payload;
        size_t len = coap_get_payload(response, &payload);

        if(len > 0 && payload != NULL) {

            char *ptr = strstr((char *)payload, "trend=");
            if(ptr != NULL) {
                ptr += strlen("trend=");
                power_PV_trend = atoi(ptr);
            }

            ptr = strstr((char *)payload, "firstPrediction=");
            if(ptr != NULL) {
                ptr += strlen("firstPrediction=");
                power_PV_pred = strtof(ptr, NULL)/1000;      // in W
            }

            LOG_INFO("Received Predicted power - Trend: %d | First prediction: %d,%04d kW\n", power_PV_trend, (int)power_PV_pred, (int)((power_PV_pred - (int)power_PV_pred) * 10000));

            power_manager_update_charging_station();
        }
    }
}

static void notification_realpower_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag) {
    int len = 0;
    const uint8_t *payload = NULL;

    if(notification) {
        len = coap_get_payload(notification, &payload);
    }

    // Handle different notification flags
    switch(flag) {
        case NOTIFICATION_OK:
            if(len > 0 && payload != NULL) {
                
                char *ptr = strstr((char *)payload, "realPV=");
                if(ptr != NULL) {
                    ptr += strlen("realPV=");
                    power_PV_real = strtof(ptr, NULL)/1000;  // In W
                }
                
                time_t timestamp;
                ptr = strstr((char *)payload, "timestamp=");
                if(ptr != NULL) {
                    ptr += strlen("timestamp=");
                    timestamp = (time_t)atoi(ptr);
                }

                struct tm *tm_info = localtime(&timestamp);

                // real power received, update charging stations if real power is less than available solar power
                if(power_PV_real < solar_available) {
                    power_manager_update_charging_station();
                }
                
                LOG_INFO("NOTIFICATION OK - Timestamp: %02d-%02d-%04d %02d:%02d:%02d | Real PV: %d,%04d kW\n",
                    tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (int)power_PV_real, (int)((power_PV_real - (int)power_PV_real) * 10000));
            }
            break;

        case OBSERVE_OK:
            if(len > 0 && payload != NULL) {
                
                char *ptr = strstr((char *)payload, "realPV=");
                if(ptr != NULL) {
                    ptr += strlen("realPV=");
                    power_PV_real = strtof(ptr, NULL)/1000;  // In W
                }
                
                time_t timestamp;
                ptr = strstr((char *)payload, "timestamp=");
                if(ptr != NULL) {
                    ptr += strlen("timestamp=");
                    timestamp = (time_t)atoi(ptr);
                }

                struct tm *tm_info = localtime(&timestamp);

                // real power received, update charging stations if real power is less than available solar power
                if(power_PV_real < solar_available) {
                    power_manager_update_charging_station();
                }
                
                LOG_INFO("OBSERVE OK - Timestamp: %02d-%02d-%04d %02d:%02d:%02d | Real PV: %d,%04d kW\n",
                    tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (int)power_PV_real, (int)((power_PV_real - (int)power_PV_real) * 10000));
            }
            break;

        case OBSERVE_NOT_SUPPORTED: // Server does not support observation
            LOG_INFO("OBSERVE_NOT_SUPPORTED - %*s", len, (char *)payload);
            LOG_INFO_("\n");
            obs = NULL;
            break;

        case ERROR_RESPONSE_CODE: // Server returned an error response
            LOG_INFO("ERROR_RESPONSE_CODE - %*s", len, (char *)payload);
            LOG_INFO_("\n");
            obs = NULL;
            break;
            
        case NO_REPLY_FROM_SERVER: // No response from server
            LOG_INFO("NO_REPLY_FROM_SERVER - removing observe registration with token %x%x", obs->token[0], obs->token[1]);
            LOG_INFO_("\n");
            obs = NULL;
            break;
    }
}