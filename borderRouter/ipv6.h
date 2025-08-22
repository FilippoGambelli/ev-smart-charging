#ifdef COOJA
    #define SMART_GRID_EP "coap://[fd00::203:3:3:3]:5683"
    #define SENSOR_PV_EP "coap://[fd00::202:2:2:2]:5683"
#else
    #define SMART_GRID_EP "coap://[fd00::f6ce:36b5:69d4:b7c7]:5683"
    #define SENSOR_PV_EP "coap://[fd00::f6ce:36a2:c608:75a9]:5683"
#endif