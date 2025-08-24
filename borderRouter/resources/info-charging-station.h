#ifndef INFO_CHARGING_STATION_H
#define INFO_CHARGING_STATION_H

#define MAX_DEVICES 20  // Maximum number of devices that can be registered

// Structure to store information about a registered device
struct charging_station {
    uint8_t id;                                 // Device ID
    uip_ipaddr_t addr;                          // Device IPv6 address
    int max_charging_power;                     // Maximum charging power in W

    bool car_registered;                        // true => car registered , false => no car registered

    bool is_charging;                           // true => charging , false => no charging
    int assigned_power;                         // Power assigned to the device (W)
    int grid_power_used;                        // Power from the grid (W)

    int vehicle_max_charging_power;             // Maximum charging power supported by the vehicle (W)
    int vehicle_max_capacity;                   // Vehicle battery capacity (Wh)
    float soc_current;                            // Current state of charge (%)
    int soc_target;                             // Desired state of charge (%)
    char license_plate[12];                     // Vehicle license plate    
    
    int priority;                               // Priority flag (1 for priority, 0 for standard)

    int estimated_charging_duration;            // Expected parking time (seconds)
    int remaining_time_seconds;                 // Remaining time (seconds)
    int remaining_energy;                       // Remaining energy to charge (Wh)
};


extern  struct charging_station EV_charger[MAX_DEVICES];    // Array to store registered devices
extern  uint8_t device_count;                               // Current number of registered devices
extern  uint8_t vehicle_count;                              // Current number of registered vehicles

#endif