#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> 
#include "resources/info-charging-station.h"


extern time_t last_execution;

extern float power_PV_real;  // kW        
extern int power_PV_tred;   // 1 -1
extern float power_PV_pred;  // kW

extern float available_energy_accumulator;  // kWh
extern float power_grid_used;  // kW



// Function called when a notification from sensorPV is received
void power_manager_update_charging_station(char* prediction);