package it.evsmartcharging;

public class Config {

    // Flag per distinguere COOJA / hardware reale
    private static final boolean COOJA = Boolean.getBoolean("cooja");

    // Endpoint centrale CoAP
    public static final String CENTRAL_NODE_EP = COOJA
        ? "coap://[fd00::201:1:1:1]:5683"
        : "coap://[fd00::f6ce:36ed:6111:3651]:5683";

    public static final String SENSOR_PV_EP = COOJA
        ? "coap://[fd00::202:2:2:2]:5683"
        : "coap://[fd00::f6ce:36a2:c608:75a9]:5683";

    public static final String SMART_GRID_EP = COOJA
        ? "coap://[fd00::203:3:3:3]:5683"
        : "coap://[fd00::f6ce:36b5:69d4:b7c7]:5683";
}
