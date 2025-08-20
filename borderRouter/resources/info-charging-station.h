#define MAX_DEVICES 20  // Maximum number of devices that can be registered

// Structure to store information about a registered device
struct charging_station {
    uint8_t id;                                 // Device ID
    uip_ipaddr_t addr;                          // Device IPv6 address
    float max_charging_power;                   // Maximum charging power in kW

    bool car_registered;

    bool is_charging;                           // true => charging , false => no charging
    float assigned_power;                       // Power assigned to the device (kW)

    float vehicle_max_charging_power;           // Maximum charging power supported by the vehicle (kW)
    unsigned int vehicle_max_capacity;          // Vehicle battery capacity (kWh)
    float soc_current;                          // Current state of charge (%)
    float soc_target;                           // Desired state of charge (%)
    char license_plate[12];                     // Vehicle license plate    
    
    int priority;                               // Priority flag (1 for priority, 0 for standard)

    uint32_t estimated_charging_duration;       // Expected parking time (seconds)
    uint32_t remaining_time_seconds;            // Expected parking time (seconds)
    float remaining_energy;                     // Remaining energy to charge (kWh)
};


extern  struct charging_station EV_charger[MAX_DEVICES];    // Array to store registered devices
extern  uint8_t device_count;                               // Current number of registered devices
extern  uint8_t vehicle_count;                               // Current number of registered devices