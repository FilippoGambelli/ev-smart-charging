#include "power-manager.h"

static coap_endpoint_t charging_station_ep;
coap_endpoint_t smart_grid_ep;              // Using the extern variable from the power-manager.h file

unsigned long last_execution = 0;
float total_power_grid_used = 0;
float solar_available = 0;

void power_manager_update_charging_station(){

    LOG_INFO("Starting power manager logic...\n");

    unsigned long now = clock_seconds();
    unsigned long elapsed_time;

    if (last_execution == 0) {   // First execution
        elapsed_time = 0;
    } else {
        elapsed_time = now - last_execution;    
    }
    last_execution = now;       // Update last execution time

    for (int i = 0; i < device_count; i++) {
        if(EV_charger[i].car_registered == false) {
            continue;
        }

        float energy = EV_charger[i].assigned_power * (elapsed_time / 3600.0);

        EV_charger[i].soc_current = (EV_charger[i].soc_current + (energy / EV_charger[i].vehicle_max_capacity) * 100);

        if(EV_charger[i].soc_current >= EV_charger[i].soc_target) {  // Check if arrived to target
            EV_charger[i].is_charging = false;
            send_charging_status(&EV_charger[i].addr, 0, 0, 0, 1);
            continue;
        } else{
            EV_charger[i].is_charging = true;
        }

        // Calculate remaining energy and time
        EV_charger[i].remaining_energy = (EV_charger[i].vehicle_max_capacity * (EV_charger[i].soc_target - EV_charger[i].soc_current) / 100);
        EV_charger[i].remaining_time_seconds -= elapsed_time;
    }

    // if ML model is not running, then predicted power will be 0
    if(run_ml_model != 0){
        solar_available = (power_PV_real < power_PV_pred) ? power_PV_real : power_PV_pred;
    } else {
        power_PV_pred = 0;
        solar_available = power_PV_real;
    }
    
    total_power_grid_used = 0;

    LOG_INFO("Solar power available: %d,%04d kW\n", (int)solar_available, (int)((solar_available - (int)solar_available) * 10000));

    // Sort EV_charger array based on priority and remaining_time_seconds
    qsort(EV_charger, device_count, sizeof(struct charging_station), compare_charging_stations);

    for (int i = 0; i < device_count; i++) {
        if(EV_charger[i].car_registered == false || EV_charger[i].is_charging == false) {
            continue;
        }

        float remaining_time_hours = EV_charger[i].remaining_time_seconds / 3600.0f;
        float required_power = EV_charger[i].remaining_energy / remaining_time_hours;

        if (solar_available >= required_power) {    // Sufficient solar panels
            EV_charger[i].assigned_power = required_power;
            solar_available -= required_power;

        } else {                            // Insufficient solar panels
            if (power_PV_trend == 1) {      // Assign the available energy: first use only the solar panel energy, and then the required amount from the grid to reach the target
                EV_charger[i].assigned_power = solar_available;

                solar_available = 0;

                float max_charging = (EV_charger[i].max_charging_power < EV_charger[i].vehicle_max_charging_power)? EV_charger[i].max_charging_power : EV_charger[i].vehicle_max_charging_power;
                if(max_charging * (EV_charger[i].remaining_time_seconds / 3600.0) < EV_charger[i].remaining_energy) {
                    
                    // The energy required to reach the target with the available max_charging, not considering the energy already supplied by the solar panels
                    EV_charger[i].grid_power_used = max_charging - EV_charger[i].assigned_power;
                    total_power_grid_used += EV_charger[i].grid_power_used;                // Use the grid to cover the remaining power
                    EV_charger[i].assigned_power += EV_charger[i].grid_power_used;
                }
            } else {
                EV_charger[i].assigned_power = solar_available;
                
                EV_charger[i].grid_power_used = (required_power - solar_available);  // Use the grid to cover the remaining power
                total_power_grid_used += EV_charger[i].grid_power_used;
                EV_charger[i].assigned_power += EV_charger[i].grid_power_used;
                
                solar_available = 0;
            }
        }

        send_charging_status(&EV_charger[i].addr, EV_charger[i].assigned_power, EV_charger[i].assigned_power-EV_charger[i].grid_power_used, EV_charger[i].remaining_time_seconds, 0);
        
        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer),
            "Charger %u:\n"
            "\t\t\t SOC current: %d.%04d %% \t\t SOC target: %d.%04d %%\n"
            "\t\t\t Remaining energy: %d.%04d kWh \t\t Remaining time: %d s\n"
            "\t\t\t Vehicle max capacity: %d.%04d kWh \t Required power: %d.%04d kW\n"
            "\t\t\t Power PV real: %d.%04d kW \t\t Power Assigned: %d.%04d kW\n",
            EV_charger[i].id,
            (int)EV_charger[i].soc_current, (int)((EV_charger[i].soc_current - (int)EV_charger[i].soc_current) * 10000),
            (int)EV_charger[i].soc_target, (int)((EV_charger[i].soc_target - (int)EV_charger[i].soc_target) * 10000),
            (int)EV_charger[i].remaining_energy, (int)((EV_charger[i].remaining_energy - (int)EV_charger[i].remaining_energy) * 10000),
            EV_charger[i].remaining_time_seconds,
            (int)EV_charger[i].vehicle_max_capacity, (int)((EV_charger[i].vehicle_max_capacity - (int)EV_charger[i].vehicle_max_capacity) * 10000),
            (int)required_power, (int)((required_power - (int)required_power) * 10000),
            (int)power_PV_real, (int)((power_PV_real - (int)power_PV_real) * 10000),
            (int)EV_charger[i].assigned_power, (int)((EV_charger[i].assigned_power - (int)EV_charger[i].assigned_power) * 10000));

        if (run_ml_model) {
            len += snprintf(buffer + len, sizeof(buffer) - len,
                "\t\t\t Power PV trend: %s \t\t Power PV pred: %d.%04d kW\n",
                power_PV_trend == 1 ? "increasing" : "decreasing",
                (int)power_PV_pred, (int)((power_PV_pred - (int)power_PV_pred) * 10000));
        }

        snprintf(buffer + len, sizeof(buffer) - len,
            "\t\t\t Renewable energy: %d.%04d kW\n\n",
            (int)(EV_charger[i].assigned_power - EV_charger[i].grid_power_used),
            (int)((EV_charger[i].assigned_power - EV_charger[i].grid_power_used - (int)(EV_charger[i].assigned_power - EV_charger[i].grid_power_used)) * 10000));

        LOG_INFO("%s", buffer);
    }

    // Update Smart Grid
    if(solar_available > 0){
        send_grid_status(solar_available, 1);
    } else if(total_power_grid_used > 0){
        send_grid_status(total_power_grid_used, -1);
    }

}

void send_charging_status(uip_ipaddr_t *addr, float assigned_power, float renewable_energy, int time_remaining, int charging_complete) {
    static coap_message_t request[1];          
    char buffer_req[128];                          
    static coap_callback_request_state_t request_state;

    memcpy(&charging_station_ep.ipaddr, addr, sizeof(uip_ipaddr_t));
    charging_station_ep.port = UIP_HTONS(5683);

    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, coap_get_mid());
    coap_set_header_uri_path(request, RES_CHARGING_STATUS_URI);

    snprintf(buffer_req, sizeof(buffer_req),
            "chargingPower=%d.%04d&energyRenewable=%d.%04d&timeRemaining=%d&chargingComplete=%d",
            (int)assigned_power, (int)((assigned_power - (int)assigned_power) * 10000),
            (int)renewable_energy, (int)((renewable_energy - (int)renewable_energy) * 10000),
            time_remaining,
            charging_complete);

    coap_set_header_content_format(request, TEXT_PLAIN);
    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));
    coap_send_request(&request_state, &charging_station_ep, request, response_handler);
}

void send_grid_status(float power_grid, int grid_direction) {
    static coap_message_t request[1];          
    char buffer_req[64];                          
    static coap_callback_request_state_t request_state;
    

    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, coap_get_mid());

    coap_set_header_uri_path(request, RES_SMART_GRID_URI);

    // Prepare the payload: power value and direction (-1 = input, 1 = output)
    if(grid_direction == 1){
        snprintf(buffer_req, sizeof(buffer_req), "power_to_grid_now=%d.%04d&power_from_grid_now=0.0",  (int)power_grid, (int)((power_grid - (int)power_grid) * 10000));
        LOG_INFO("Sent PUT - Power to grid now: %d,%04d kW\n", (int)power_grid, (int)((power_grid - (int)power_grid) * 10000));
    } else {
        snprintf(buffer_req, sizeof(buffer_req), "power_from_grid_now=%d.%04d&power_to_grid_now=0.0",  (int)power_grid, (int)((power_grid - (int)power_grid) * 10000));
        LOG_INFO("Sent PUT - Power from grid now: %d,%04d kW\n", (int)power_grid, (int)((power_grid - (int)power_grid) * 10000));
    }

    coap_set_header_content_format(request, TEXT_PLAIN);
    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));
    coap_send_request(&request_state, &smart_grid_ep, request, response_handler);

}

void response_handler(coap_callback_request_state_t *callback_state) {
    coap_message_t *response = callback_state->state.response;
    if(!response) {
        return;
    }
}

int compare_charging_stations(const void *a, const void *b) {
    const struct charging_station *cs1 = (const struct charging_station *)a;
    const struct charging_station *cs2 = (const struct charging_station *)b;

    // Sort by priority (1 comes before 0)
    if (cs1->priority != cs2->priority) {
        return (cs2->priority - cs1->priority);
    }

    // If priority is the same, sort by remaining_time_seconds (descending)
    if (cs1->remaining_time_seconds < cs2->remaining_time_seconds) return 1;
    if (cs1->remaining_time_seconds > cs2->remaining_time_seconds) return -1;

    return 0; // Equal priority and duration
}
