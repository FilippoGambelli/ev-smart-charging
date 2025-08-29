#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"


// Log module configuration
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define CLOUD_APPLICATION_EP "coap://[fd00::1]:5683/"
#define RES_CLOUD_REGISTER_URI "/registration"

static coap_endpoint_t cloud_application_ep;

extern coap_resource_t res_status_smart_grid;

// Callback function to handle the cloud application's response
static void reg_handler(coap_message_t *response);

PROCESS(smart_grid, "Smart Grid");
AUTOSTART_PROCESSES(&smart_grid);

PROCESS_THREAD(smart_grid, ev, data){
    static coap_message_t request[1];
    static char buffer[128];

    PROCESS_BEGIN();

    LOG_INFO("Starting Smart Grid\n");

    // Activate CoAP resources
    coap_activate_resource(&res_status_smart_grid, "status_smart_grid");
    LOG_INFO("CoAP resources activated\n");


    LOG_INFO("Sending registration to cloud application....\n");

    coap_init_message(request, COAP_TYPE_CON, COAP_POST, coap_get_mid());
    coap_set_header_uri_path(request, RES_CLOUD_REGISTER_URI);

    // Request to cloud application
    coap_endpoint_parse(CLOUD_APPLICATION_EP, strlen(CLOUD_APPLICATION_EP), &cloud_application_ep);

    // Build payload (JSON)
    snprintf(buffer, sizeof(buffer),
            "{"
            "\"nodeType\":\"smartGrid\","
            "\"requiredNodes\":[]"
            "}"
        );

    coap_set_header_content_format(request, APPLICATION_JSON);
    coap_set_payload(request, (uint8_t *)buffer, strlen(buffer));

    COAP_BLOCKING_REQUEST(&cloud_application_ep, request, reg_handler);

    while(1) {
        PROCESS_WAIT_EVENT();
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