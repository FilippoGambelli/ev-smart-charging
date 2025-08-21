#include "contiki.h"
#include "coap-engine.h"

// Log module configuration
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

// CoAP resources
extern coap_resource_t res_status_power_grid;

PROCESS(smart_grid, "Smart Grid");
AUTOSTART_PROCESSES(&smart_grid);

PROCESS_THREAD(smart_grid, ev, data){

    PROCESS_BEGIN();

    LOG_INFO("Starting Smart Grid\n");

    // Activate CoAP resources
    coap_activate_resource(&res_status_power_grid, "res_status_power_grid");

    while(1) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}