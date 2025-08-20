#include <time.h>
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-debug.h"
#include "power-manager.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP


time_t last_execution = 0;
float available_energy_accumulator = 0;  // Assumption that the accumulator is initially empty
float power_grid_used = 0;

void power_manager_update_charging_station(){
    if(vehicle_count == 0){  // NO Vehicle
        // TODO: assegnare tutta l'energia ai pannelli solari
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

        // Calculate remaining energy and time
        EV_charger[i].remaining_energy = EV_charger[i].vehicle_max_capacity * (EV_charger[i].soc_target - EV_charger[i].soc_current) / 100;  // kWh

        // !! TODO: Fare il chech se la cmacchina è completamente carica

        EV_charger[i].remaining_time_seconds -= elapsed_time;  // seconds
    }


    // !! TODO: Per adesso sono dati a caso, dopo questa variabili saranno già definite con il dato corretto      
    float power_PV_pred = 150.0;  // kW
    
    LOG_INFO("\n !!Power prediction: %f kW\n", power_PV_real);

    float solar_available = (power_PV_real < power_PV_pred) ? power_PV_real : power_PV_pred;  // kW


    power_grid_used = 0;

    //TO DO: ADESSO FACCIO UN CICLO FOR A CASO MA DEVE ESSERE IN ORDINE DI PRIORITA
    for (int i = 0; i < device_count; i++) {
        float required_power = EV_charger[i].remaining_energy/(EV_charger[i].remaining_time_seconds / 3600); // kW

        LOG_INFO("Charger %d \t soc_current = %f %% \t soc_target = %f %% \t remaining_energy = %f kWh \t remaining_time_secs = %u s \t vehicle_max_capacity= %u kWh \t assigned_power = %f kW \t "
            "required_power = %f kW \t solar_available = %f kW \t available_acc = %f kWh \t power_grid_used = %f kW \t power_PV_tred = %d \t power_PV_pred = %f kW \t power_PV_real = %f kW\n",
            i, EV_charger[i].soc_current, EV_charger[i].soc_target, EV_charger[i].remaining_energy, EV_charger[i].remaining_time_seconds, EV_charger[i].vehicle_max_capacity, EV_charger[i].assigned_power, required_power,
            solar_available, available_energy_accumulator, power_grid_used, power_PV_trend, power_PV_pred, power_PV_real);

        if (solar_available >= required_power) {        // Caso 1: pannelli solari sufficienti
            EV_charger[i].assigned_power = required_power;
            EV_charger[i].is_charging = true;
            solar_available -= required_power;
            LOG_INFO("DEBUG: Charger %d, caso 1\n", i);
        } else {                                        // Caso 2: pannelli solari insufficienti
            if (power_PV_trend > 0) {                    // Caso 2.1: previsioni di incremento produzione -> assegno tutto ciò che c'è perchè prevedo aumento
                EV_charger[i].assigned_power = solar_available;
                EV_charger[i].is_charging = true;

                solar_available = 0;
                // TODO ATTENZIONE QUI DEVO CONTROLLARE CHE ANCHE SE DO MENO ENERGIA DI QUELLA RICHIESTA,
                // DEVO CALCOLARE PER QUANTO TEMPO POSSO ASSEGNARE QUESTA POTENZA (IN BASE AD OGNI QUANTO VA IN ESECUZIONE)
                // E IN BASE A QUELLO DEVO VEDERE CHE DANDO MENO ENERGIA DI QUELLA RICHIESTA COMUNQUE ALLA PROSSIMA ESECUZIONE RIESCO COMUNQUE A RIMANERE
                // NELLA DEADLINE CONSIDERANDO LIMITE DI CARICA DELLA Charger E DELLA MACCHINA
                LOG_INFO("DEBUG: Charger %d, caso 2.1\n", i);
            } else {                                    // 2.2: produzione stabile o in calo -> uso pannelli + accumulatore + rete se necessario
                float power_from_solar_and_acc = solar_available + available_energy_accumulator;
                if (power_from_solar_and_acc >= required_power) {
                    EV_charger[i].assigned_power = required_power;
                    EV_charger[i].is_charging = true;

                    available_energy_accumulator -= (required_power - solar_available);
                    solar_available = 0;
                } else {
                    EV_charger[i].assigned_power = power_from_solar_and_acc;
                    available_energy_accumulator = 0;
                    solar_available = 0;
                    
                    power_grid_used += (required_power - power_from_solar_and_acc);  // Use the grid to cover the remaining power
                    EV_charger[i].assigned_power += power_grid_used;
                    EV_charger[i].is_charging = true;
                }
                LOG_INFO("DEBUG: Charger %d, caso 2.2\n", i);
            }
        }

        LOG_INFO("Charger %d \t assigned_power = %f kW \t required_power = %f kW \t solar_available = %f kW \t available_acc = %f kWh \t power_grid_used = %f kW\n",
            i, EV_charger[i].assigned_power, required_power, solar_available, available_energy_accumulator, power_grid_used);
    }

    // DEFINIRE OGNI QUANTO VA IN ESECUZIONE QUESTO CODICE PERCHE' ALLE COLONNINE DEVO DIRE ANCHE PER QUANTO TEMPO FARLO ESEGUIRE = min(TEMPO ESECUZIONE, TEMPO PER RAGGIUNGERE SOC TARGET)
    // TODO, CONTROLLARE SEMPRE CHE LA POTENZA ASSEGNATA NON SIA SUPERIORE A QUELLA DELLA MACCHINA E DELLA COLONNINA
    // TODO FARE IL CHECK SE E' FINITA LA CARICA DI UNA COLONNINA E INVIARE IL MESSAGGIO AL CLIENT
    // TODO INVIARE AL CLIENT LA CARICA ASSEGNATA PER OGNI COLONNINA E PER QUANTO TEMPO
}