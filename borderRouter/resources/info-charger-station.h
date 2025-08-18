#define MAX_DEVICES 20  // Maximum number of devices that can be registered

// Structure to store information about a registered device
struct device_info {
    uint8_t id;                        // Device ID
    uip_ipaddr_t addr;                 // Device IPv6 address
    float max_charging_power;          // Maximum charging power in kW

    float vehicle_max_charging_power;  // Maximum charging power supported by the vehicle (kW)
    float soc_current;                 // Current state of charge (%)
    float soc_target;                  // Desired state of charge (%)
    uint32_t parking_time_minutes;     // Expected parking time (minutes)
};


extern  struct device_info devices[MAX_DEVICES];  // Array to store registered devices
extern  uint8_t device_count;                 // Current number of registered devices