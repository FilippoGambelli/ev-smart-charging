#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> 
#include <time.h>
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include <stdlib.h>
#include "coap-callback-api.h"
#include "../ipv6.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#include "../resources/info-charging-station.h"

#define RES_CHARGING_STATUS_URI "/res_charging_status"
#define RES_SMART_GRID_URI "/res_status_power_grid"

extern time_t last_execution;

extern float power_PV_real;  // kW        
extern int power_PV_trend;   // 1 -1
extern float power_PV_pred;  // kW

extern float total_power_grid_used;  // kW

extern float solar_remaining;  // kW



// Function called when a notification from sensorPV is received
void power_manager_update_charging_station();
int compare_charging_stations(const void *a, const void *b);
void send_charging_status(uip_ipaddr_t *addr, float assigned_power, float renewable_energy, uint8_t charging_complete);
void response_handler(coap_callback_request_state_t *callback_state);
void send_grid_status(float power_grid, int grid_direction);
