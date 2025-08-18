package it.evsmartcharging;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CloudApplication {
    
    private static final Logger logger = LoggerFactory.getLogger(CloudApplication.class);
    
    public static void main(String[] args) {
        
        logger.info("Starting system ...");

        CoapObserver coapObserver = new CoapObserver();
        coapObserver.startAllObservation();

        // Keep the application running to continue receiving notifications
        try {
            Thread.sleep(5 * 60 * 1000); // 5 minutes, adjust as needed
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            // Stop observations before exiting
            coapObserver.stopAllObservation();
        }
    }
}