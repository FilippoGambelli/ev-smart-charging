#include "power-manager.h"

static coap_endpoint_t charging_station_ep;
static coap_endpoint_t smart_grid_ep;

time_t last_execution = 0;
int total_power_grid_used = 0;
int solar_available = 0;

void power_manager_update_charging_station(){

    LOG_INFO("Starting power manager logic...\n");

    int elapsed_time = 0;
    time_t now = time(NULL);

    if (last_execution == 0) {   // First execution
        elapsed_time = 0;
    } else {
        elapsed_time = difftime(now, last_execution);
    }
    last_execution = now;       // Update last execution time

    for (int i = 0; i < device_count; i++) {
        if(EV_charger[i].car_registered == false) {
            continue;
        }

        float energy = EV_charger[i].assigned_power * (elapsed_time / 3600.0); // Energia in Wh

        EV_charger[i].soc_current = (EV_charger[i].soc_current + (energy / EV_charger[i].vehicle_max_capacity) * 100);

        if(EV_charger[i].soc_current >= EV_charger[i].soc_target) {  // Check if arrived to target
            EV_charger[i].is_charging = false;
            send_charging_status(&EV_charger[i].addr, 0, 0, 1);
            continue;
        } else{
            EV_charger[i].is_charging = true;
        }

        // Calculate remaining energy and time
        EV_charger[i].remaining_energy = (int)(EV_charger[i].vehicle_max_capacity * (EV_charger[i].soc_target - EV_charger[i].soc_current) / 100);  // Wh
        EV_charger[i].remaining_time_seconds -= elapsed_time;  // seconds
    }

    solar_available = (power_PV_real < power_PV_pred) ? power_PV_real : power_PV_pred;  // W
    total_power_grid_used = 0;

    // Sort EV_charger array based on priority and remaining_time_seconds
    qsort(EV_charger, device_count, sizeof(struct charging_station), compare_charging_stations);

    for (int i = 0; i < device_count; i++) {
        if(EV_charger[i].car_registered == false || EV_charger[i].is_charging == false) {
            continue;
        }

        float remaining_time_hours = EV_charger[i].remaining_time_seconds / 3600.0f;
        float required_power = EV_charger[i].remaining_energy / remaining_time_hours;       // W

        LOG_INFO("\n Charger %u \n soc_current = %.2f %% \n soc_target = %d %% \n remaining_energy = %d Wh \n remaining_time_secs = %d s \n vehicle_max_capacity= %u Wh \n "
            "required_power = %.2f W \n\n power_PV_real = %d kW\n power_PV_pred = %d kW \n solar_available = %d kW \n power_PV_trend = %d \n\n",
            EV_charger[i].id, EV_charger[i].soc_current, EV_charger[i].soc_target, EV_charger[i].remaining_energy, EV_charger[i].remaining_time_seconds, EV_charger[i].vehicle_max_capacity, required_power,
            power_PV_real, power_PV_pred, solar_available, power_PV_trend);

        if (solar_available >= required_power) {    // Sufficient solar panels
            EV_charger[i].assigned_power = (int)required_power;
            solar_available -= (int)required_power;

        } else {                            // Insufficient solar panels
            if (power_PV_trend == 1) {      // Assign the available energy: first use only the solar panel energy, and then the required amount from the grid to reach the target
                EV_charger[i].assigned_power = solar_available;

                solar_available = 0;

                int max_charging = (EV_charger[i].max_charging_power < EV_charger[i].vehicle_max_charging_power)? EV_charger[i].max_charging_power : EV_charger[i].vehicle_max_charging_power;
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

        send_charging_status(&EV_charger[i].addr, EV_charger[i].assigned_power, EV_charger[i].assigned_power-EV_charger[i].grid_power_used, 0);

        LOG_INFO("Charger ID: %u - Power Assigned: %d | Renewable energy: %d | Current SOC: %d.%d\n",
            EV_charger[i].id, EV_charger[i].assigned_power, EV_charger[i].assigned_power - EV_charger[i].grid_power_used,
            (int)EV_charger[i].soc_current, (int)((EV_charger[i].soc_current - (int)EV_charger[i].soc_current) * 10)
        );
    }

    // Update Smart Grid
    if(solar_available > 0){
        send_grid_status(solar_available, 1);
    } else if(total_power_grid_used > 0){
        send_grid_status(total_power_grid_used, -1);
    }

}

void send_charging_status(uip_ipaddr_t *addr, int assigned_power, int renewable_energy, int charging_complete) {
    static coap_message_t request[1];          
    char buffer_req[128];                          
    static coap_callback_request_state_t request_state;

    memcpy(&charging_station_ep.ipaddr, addr, sizeof(uip_ipaddr_t));
    charging_station_ep.port = UIP_HTONS(5683);

    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
    coap_set_header_uri_path(request, RES_CHARGING_STATUS_URI);

    snprintf(buffer_req, sizeof(buffer_req),
             "chargingPower=%d&energyRenewable=%d&chargingComplete=%d",
             assigned_power, renewable_energy, charging_complete);

    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));
    coap_send_request(&request_state, &charging_station_ep, request, response_handler);
}

void send_grid_status(int power_grid, int grid_direction) {
    static coap_message_t request[1];          
    char buffer_req[32];                          
    static coap_callback_request_state_t request_state;
    

    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
    coap_endpoint_parse(SMART_GRID_EP, strlen(SMART_GRID_EP), &smart_grid_ep);

    coap_set_header_uri_path(request, RES_SMART_GRID_URI);

    // Prepare the payload: power value and direction (-1 = input, 1 = output)
    if(grid_direction == 1){
        snprintf(buffer_req, sizeof(buffer_req), "energy_to_grid_now=%d",  power_grid);
        LOG_INFO("Sent PUT - Energy to grid now: %d W\n", power_grid);
    } else {
        snprintf(buffer_req, sizeof(buffer_req), "energy_from_grid_now=%d",  power_grid);
        LOG_INFO("Sent PUT - Energy from grid now: %d W\n", power_grid);
    }

    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));
    coap_send_request(&request_state, &smart_grid_ep, request, response_handler);

}

void response_handler(coap_callback_request_state_t *callback_state) {
    coap_message_t *response = callback_state->state.response;
    if(!response) {
        return;
    }
    LOG_INFO("Update resource correctly\n");
}

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

    return 0; // Equal priority and duration
}
