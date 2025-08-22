#include "power-manager.h"

coap_endpoint_t charging_station_ep;
coap_endpoint_t smart_grid_ep;


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
        if(EV_charger[i].car_registered == false) {
            continue;
        }

        float energy = EV_charger[i].assigned_power * (elapsed_time / 3600.0); // Energia in kWh

        EV_charger[i].soc_current = EV_charger[i].soc_current + (energy / EV_charger[i].vehicle_max_capacity) * 100;

        if(EV_charger[i].soc_current > EV_charger[i].soc_target) {
            EV_charger[i].is_charging = false;
            send_charging_status(&EV_charger[i].addr, 0.0, 0.0, 1);

            continue;
        } else{
            EV_charger[i].is_charging = true;
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
        if(EV_charger[i].car_registered == false || EV_charger[i].is_charging == false) {
            continue;       // Skip if not registered,
        }

        // !! TODO: (NON DOVREBBE SUCCEDERE MAI) Controllare nei log se necessario controllare che non superi limite colonnina
        float remaining_time_hours = EV_charger[i].remaining_time_seconds / 3600.0f;
        float required_power = EV_charger[i].remaining_energy / remaining_time_hours;

        LOG_INFO("\n Charger %u \n soc_current = %.2f \n soc_target = %.2f \n remaining_energy = %.2f kWh \n remaining_time_secs = %u s \n vehicle_max_capacity= %u kWh \n "
            "required_power = %.2f kW \n\n power_PV_real = %.2f kW\n power_PV_pred = %.2f kW \n solar_available = %.2f kW \n power_PV_trend = %d \n\n",
            EV_charger[i].id, EV_charger[i].soc_current, EV_charger[i].soc_target, EV_charger[i].remaining_energy, EV_charger[i].remaining_time_seconds, EV_charger[i].vehicle_max_capacity, required_power,
            power_PV_real, power_PV_pred, solar_available, power_PV_trend);

        if (solar_available >= required_power) {        // Caso 1: pannelli solari sufficienti
            EV_charger[i].assigned_power = required_power;
            solar_available -= required_power;

            // LOG_INFO("\n Charger %u, caso 1 \n", EV_charger[i].id);  // TODO: remove this log
        } else {                                        // Caso 2: pannelli solari insufficienti
            if (power_PV_trend > 0) {                    // Caso 2.1: previsioni di incremento produzione -> assegno tutto ciò che c'è perchè prevedo aumento
                EV_charger[i].assigned_power = solar_available;

                solar_available = 0;

                float max_charging = (EV_charger[i].max_charging_power < EV_charger[i].vehicle_max_charging_power)? EV_charger[i].max_charging_power : EV_charger[i].vehicle_max_charging_power;
                if(max_charging * (EV_charger[i].remaining_time_seconds / 3600.0) < EV_charger[i].remaining_energy) {
                    
                    // If the maximum charging power is not enough to reach the target SOC, use the grid
                    EV_charger[i].grid_power_used = (EV_charger[i].remaining_energy - max_charging * (EV_charger[i].remaining_time_seconds / 3600.0));
                    total_power_grid_used += EV_charger[i].grid_power_used ;                // Use the grid to cover the remaining power
                    EV_charger[i].assigned_power += EV_charger[i].grid_power_used ;
                }

                // LOG_INFO("\n Charger %u, caso 2.1 \n", EV_charger[i].id);   // TODO: remove this log
            } else {    // 2.2: produzione stabile o in calo -> uso pannelli + rete
                EV_charger[i].assigned_power = solar_available;
                
                EV_charger[i].grid_power_used = (required_power - solar_available);  // Use the grid to cover the remaining power
                total_power_grid_used += EV_charger[i].grid_power_used;
                EV_charger[i].assigned_power += EV_charger[i].grid_power_used;
                
                solar_available = 0;

                // LOG_INFO("Charger %u, caso 2.2 \n", EV_charger[i].id);   // TODO: remove this log
            }
        }

        // TODO: remove this log
        // LOG_INFO("\n Charger %u \n assigned_power = %.2f kW \n solar_available = %.2f kW \n power_grid_used = %.2f kW\n",
        //    EV_charger[i].id, EV_charger[i].assigned_power, solar_available, EV_charger[i].grid_power_used);

        send_charging_status(&EV_charger[i].addr, EV_charger[i].assigned_power, EV_charger[i].assigned_power-EV_charger[i].grid_power_used, 0);
    }
        
    solar_remaining = solar_available;

    if(solar_remaining > 0){
        send_grid_status(solar_remaining, 1);
    } else if(total_power_grid_used > 0){
        send_grid_status(total_power_grid_used, -1);
    }

}


void send_charging_status(uip_ipaddr_t *addr, float assigned_power, float renewable_energy, uint8_t charging_complete) {
    static coap_message_t request[1];          
    char buffer_req[128];                          
    static coap_callback_request_state_t request_state;

    // Copy the IP address and set the CoAP port (5683)
    memcpy(&charging_station_ep.ipaddr, addr, sizeof(uip_ipaddr_t));
    charging_station_ep.port = UIP_HTONS(5683);

    // Initialize the CoAP message (CON type, PUT method, message ID 0)
    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);

    coap_set_header_uri_path(request, RES_CHARGING_STATUS_URI);

    snprintf(buffer_req, sizeof(buffer_req),
             "chargingPower=%.4f&energyRenewable=%.4f&chargingComplete=%d",
             assigned_power, renewable_energy, charging_complete);

    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));

    coap_send_request(&request_state, &charging_station_ep, request, response_handler);
}


void send_grid_status(float power_grid, int grid_direction) {
    static coap_message_t request[1];          
    char buffer_req[32];                          
    static coap_callback_request_state_t request_state;
    

    // Initialize the CoAP message (CON type, PUT method, message ID 0)
    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);

    // Set the endpoint for the smart grid resource
    coap_endpoint_parse(SMART_GRID_EP, strlen(SMART_GRID_EP), &smart_grid_ep);


    // Set the target URI path
    coap_set_header_uri_path(request, RES_SMART_GRID_URI);

    // Prepare the payload: power value and direction (-1 = input, 1 = output)
    if(grid_direction == 1)
        snprintf(buffer_req, sizeof(buffer_req), "energy_to_grid_now=%.4f",  power_grid);
    else
        snprintf(buffer_req, sizeof(buffer_req), "energy_from_grid_now=%.4f",  power_grid);

    // Attach payload to the CoAP message
    coap_set_payload(request, (uint8_t *)buffer_req, strlen(buffer_req));

    // Send the request to the grid endpoint, response handled
    coap_send_request(&request_state, &smart_grid_ep, request, response_handler);
}




void response_handler(coap_callback_request_state_t *callback_state) {
    // Get the CoAP response from the callback state
    coap_message_t *response = callback_state->state.response;
    if(!response) {
        // TODO: add LOG_INFO
        return;
    }
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

    return 0; // Equal priority and duration → keep same order
}
