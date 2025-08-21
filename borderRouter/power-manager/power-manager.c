#include "power-manager.h"


time_t last_execution = 0;
float total_power_grid_used = 0;
float solar_remaining = 0;              // Remaining solar power after distribution

void power_manager_update_charging_station(){
    if(vehicle_count == 0){  // NO Vehicle
        return;
    }

    LOG_INFO("Starting power manager update with prediction...\n");

    int elapsed_time = 0;
    time_t now = time(NULL);

    if (last_execution == 0) {   // First execution
        elapsed_time = 0;
    } else {
        elapsed_time = difftime(now, last_execution);
    }
    // Update last execution time
    last_execution = now;

    for (uint8_t i = 0; i < device_count; i++) {
        float energy = EV_charger[i].assigned_power * (elapsed_time / 3600.0); // Energia in kWh

        EV_charger[i].soc_current = EV_charger[i].soc_current + (energy / EV_charger[i].vehicle_max_capacity) * 100;
        if(EV_charger[i].soc_current > EV_charger[i].soc_target) {
            EV_charger[i].is_charging = false;  // Stop charging if SOC is at target
            LOG_INFO("DEBUG: Charger %d, car fully charged, setting SOC to target\n", i);

            continue;
            // !! TODO: Inviare al CLIENT
        }

        // Calculate remaining energy and time
        EV_charger[i].remaining_energy = EV_charger[i].vehicle_max_capacity * (EV_charger[i].soc_target - EV_charger[i].soc_current) / 100;  // kWh

        EV_charger[i].remaining_time_seconds -= elapsed_time;  // seconds
    }

    float solar_available = (power_PV_real < power_PV_pred) ? power_PV_real : power_PV_pred;  // kW
    total_power_grid_used = 0;

    // Sort EV_charger array based on priority and remaining_time_seconds
    qsort(EV_charger, device_count, sizeof(struct charging_station), compare_charging_stations);

    for (int i = 0; i < device_count; i++) {
        if(EV_charger[i].car_registered == false) {
            continue;       // Skip if not registered,
        }

        // !! TODO: (NON DOVREBBE SUCCEDERE MAI) Controllare nei log se necessario controllare che non superi limite colonnina
        float required_power = EV_charger[i].remaining_energy/(EV_charger[i].remaining_time_seconds / 3600); // kW

        LOG_INFO("Charger %u \t soc_current = %.2f %% \t soc_target = %.2f %% \t remaining_energy = %.2f kWh \t remaining_time_secs = %u s \t vehicle_max_capacity= %u kWh \t assigned_power = %.2f kW \t "
            "required_power = %.2f kW \t solar_available = %.2f kW \t power_PV_trend = %d \t power_PV_pred = %.2f kW \t power_PV_real = %.2f kW\n",
            EV_charger[i].id, EV_charger[i].soc_current, EV_charger[i].soc_target, EV_charger[i].remaining_energy, EV_charger[i].remaining_time_seconds, EV_charger[i].vehicle_max_capacity, EV_charger[i].assigned_power, required_power,
            solar_available, power_PV_trend, power_PV_pred, power_PV_real);

        if (solar_available >= required_power) {        // Caso 1: pannelli solari sufficienti
            EV_charger[i].assigned_power = required_power;
            EV_charger[i].is_charging = true;
            solar_available -= required_power;

            LOG_INFO("DEBUG: Charger %u, caso 1\n", EV_charger[i].id);
        } else {                                        // Caso 2: pannelli solari insufficienti
            if (power_PV_trend > 0) {                    // Caso 2.1: previsioni di incremento produzione -> assegno tutto ciò che c'è perchè prevedo aumento
                EV_charger[i].assigned_power = solar_available;
                EV_charger[i].is_charging = true;

                solar_available = 0;

                float max_charging = (EV_charger[i].max_charging_power < EV_charger[i].vehicle_max_charging_power)? EV_charger[i].max_charging_power : EV_charger[i].vehicle_max_charging_power;
                if(max_charging * (EV_charger[i].remaining_time_seconds / 3600.0) < EV_charger[i].remaining_energy) {
                    
                    // If the maximum charging power is not enough to reach the target SOC, use the grid
                    EV_charger[i].grid_power_used = (EV_charger[i].remaining_energy - max_charging * (EV_charger[i].remaining_time_seconds / 3600.0));
                    total_power_grid_used += EV_charger[i].grid_power_used ;                // Use the grid to cover the remaining power
                    EV_charger[i].assigned_power += EV_charger[i].grid_power_used ;
                }

                LOG_INFO("DEBUG: Charger %u, caso 2.1\n", EV_charger[i].id);
            } else {    // 2.2: produzione stabile o in calo -> uso pannelli + rete
                EV_charger[i].assigned_power = solar_available;
                
                EV_charger[i].grid_power_used += (required_power - solar_available);  // Use the grid to cover the remaining power
                total_power_grid_used += EV_charger[i].grid_power_used;
                EV_charger[i].assigned_power += (required_power - solar_available);
                
                solar_available = 0;

                EV_charger[i].is_charging = true;
                LOG_INFO("DEBUG: Charger %u, caso 2.2\n", EV_charger[i].id);
            }
        }


        LOG_INFO("Charger %u \t assigned_power = %.2f kW \t required_power = %.2f kW \t solar_available = %.2f kW \t power_grid_used = %.2f kW\n",
            EV_charger[i].id, EV_charger[i].assigned_power, required_power, solar_available, EV_charger[i].grid_power_used);

        // TODO: Inviare al CLIENT le informazioni aggiornate della colonnina: EV_charger[i].assigned_power e EV_charger[i].grid_power_used
        }
        
    solar_remaining = solar_available;
    LOG_INFO("Total power used from grid: %.2f kW \t Remaining solar power: %.2f kW\n", total_power_grid_used, solar_remaining);

}



// Comparator function for qsort
int compare_charging_stations(const void *a, const void *b) {
    const struct charging_station *cs1 = (const struct charging_station *)a;
    const struct charging_station *cs2 = (const struct charging_station *)b;

    // Sort by priority (1 comes before 0)
    if (cs1->priority != cs2->priority) {
        return (cs2->priority - cs1->priority);  
        // Return positive if cs2 > cs1 (so priority=1 comes first)
    }

    // If priority is the same, sort by remaining_time_seconds (descending)
    if (cs1->remaining_time_seconds < cs2->remaining_time_seconds) return 1;
    if (cs1->remaining_time_seconds > cs2->remaining_time_seconds) return -1;

    return 0; // Equal priority and duration → keep same order
}
