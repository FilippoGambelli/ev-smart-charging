#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> 
#include <time.h>
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#include "../resources/info-charging-station.h"


extern time_t last_execution;

extern float power_PV_real;  // kW        
extern int power_PV_trend;   // 1 -1
extern float power_PV_pred;  // kW

extern float total_power_grid_used;  // kW

extern float solar_remaining;  // kW



// Function called when a notification from sensorPV is received
void power_manager_update_charging_station();
int compare_charging_stations(const void *a, const void *b);