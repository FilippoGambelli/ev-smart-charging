#include "contiki.h"
#include "coap-engine.h"
#include "sys/etimer.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

extern coap_resource_t res_solar_obs;

static struct etimer e_timer;

PROCESS(sensor_server, "Sensor Server");
AUTOSTART_PROCESSES(&sensor_server);

PROCESS_THREAD(sensor_server, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Starting Sensor Server");
  LOG_INFO_("\n");

  coap_activate_resource(&res_solar_obs, "res_solar_obs");

  etimer_set(&e_timer, CLOCK_SECOND * 10);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER && data == &e_timer){
        LOG_INFO("Event triggered");
        LOG_INFO_("\n");
        res_solar_obs.trigger();
        etimer_reset(&e_timer);
    }
  }

  PROCESS_END();
}