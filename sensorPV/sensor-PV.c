#include "contiki.h"
#include "coap-engine.h"
#include "sys/etimer.h"

// Log module configuration
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

// Timer intervals (in seconds)
#define SOLAR_DATA_INTERVAL    20
#define REAL_POWER_INTERVAL     5

// CoAP resources
extern coap_resource_t res_solar_obs;
extern coap_resource_t res_real_power_obs;
extern coap_resource_t res_pred_power;

PROCESS(sensor_server, "Sensor Server");
AUTOSTART_PROCESSES(&sensor_server);

PROCESS_THREAD(sensor_server, ev, data){

    // Event timers
    static struct etimer e_timer_solar_data;
    static struct etimer e_timer_real_power_data;

    PROCESS_BEGIN();

    LOG_INFO("Starting Sensor Server\n");

    // Activate CoAP resources
    coap_activate_resource(&res_solar_obs, "res_solar_obs");
    coap_activate_resource(&res_real_power_obs, "res_real_power_obs");
    coap_activate_resource(&res_pred_power, "res_pred_power");

    // Set timers for periodic events
    etimer_set(&e_timer_solar_data, CLOCK_SECOND * SOLAR_DATA_INTERVAL);
    etimer_set(&e_timer_real_power_data, CLOCK_SECOND * REAL_POWER_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT();

        // Handle solar data timer event
        if(ev == PROCESS_EVENT_TIMER && data == &e_timer_solar_data){
            LOG_INFO("Event triggered - Solar Data");
            LOG_INFO_("\n");
            res_solar_obs.trigger();      // Notify observers of solar data
            etimer_reset(&e_timer_solar_data);
        }
        // Handle real power data timer event
        else if(ev == PROCESS_EVENT_TIMER && data == &e_timer_real_power_data){
            LOG_INFO("Event triggered - Real Power Data");
            LOG_INFO_("\n");
            res_real_power_obs.trigger(); // Notify observers of real power
            etimer_reset(&e_timer_real_power_data);
        }
    }

    PROCESS_END();
}