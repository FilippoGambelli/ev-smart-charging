#include "contiki.h"
#include "coap-engine.h"
#include "sys/etimer.h"
#include "coap-blocking-api.h"


// Log module configuration
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define CLOUD_APPLICATION_EP "coap://[fd00::1]:5683/"

#define RES_CLOUD_REGISTER_URI "/registration"

// Timer intervals (in seconds)
#define SOLAR_DATA_INTERVAL    20
#define REAL_POWER_INTERVAL     5

static coap_endpoint_t cloud_application_ep;

// CoAP resources
extern coap_resource_t res_solar_obs;
extern coap_resource_t res_real_power_obs;
extern coap_resource_t res_pred_power;

// Callback function to handle the cloud application's response to the sensor PV device
static void reg_handler(coap_message_t *response);

PROCESS(sensorPV, "Sensor PV");
AUTOSTART_PROCESSES(&sensorPV);

PROCESS_THREAD(sensorPV, ev, data){
    static char buffer[128];
    static coap_message_t request[1];

    // Event timers
    static struct etimer e_timer_solar_data;
    static struct etimer e_timer_real_power_data;

    PROCESS_BEGIN();

    LOG_INFO("Starting Sensor PV\n");

    // Activate CoAP resources
    coap_activate_resource(&res_solar_obs, "solar_obs");
    coap_activate_resource(&res_real_power_obs, "real_power_obs");
    coap_activate_resource(&res_pred_power, "pred_power");
    LOG_INFO("CoAP resources activated\n");


    LOG_INFO("Sending registration to cloud application....\n");

    coap_init_message(request, COAP_TYPE_CON, COAP_POST, coap_get_mid());
    coap_set_header_uri_path(request, RES_CLOUD_REGISTER_URI);

    // Request to cloud application
    coap_endpoint_parse(CLOUD_APPLICATION_EP, strlen(CLOUD_APPLICATION_EP), &cloud_application_ep);

    // Build payload (JSON)
    snprintf(buffer, sizeof(buffer),
            "{"
            "\"nodeType\":\"sensorPV\","
            "\"requiredNodes\":[]"
            "}"
        );

    coap_set_header_content_format(request, APPLICATION_JSON);
    coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));

    COAP_BLOCKING_REQUEST(&cloud_application_ep, request, reg_handler);

    // Set timers for periodic events
    etimer_set(&e_timer_solar_data, CLOCK_SECOND * SOLAR_DATA_INTERVAL);
    etimer_set(&e_timer_real_power_data, CLOCK_SECOND * REAL_POWER_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT();

        // Handle solar data timer event
        if(ev == PROCESS_EVENT_TIMER && data == &e_timer_solar_data){
            LOG_INFO("Event triggered - Solar Data\n");
            res_solar_obs.trigger();      // Notify observers of solar data
            etimer_reset(&e_timer_solar_data);
        }
        // Handle real power data timer event
        else if(ev == PROCESS_EVENT_TIMER && data == &e_timer_real_power_data){
            LOG_INFO("Event triggered - Real Power Data\n");
            res_real_power_obs.trigger(); // Notify observers of real power
            etimer_reset(&e_timer_real_power_data);
        }
    }

    PROCESS_END();
}


// Callback function to handle the cloud application's response
static void reg_handler(coap_message_t *response) {
    if(response) {
        uint8_t code = response->code;

        if(code == CREATED_2_01) {
            LOG_INFO("Registration successful\n");
        } else {
            LOG_INFO("Registration failed\n");
        }
    }
}