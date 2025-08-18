package it.evsmartcharging;

import org.eclipse.californium.core.CoapServer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CloudApplication {
    
    private static final Logger logger = LoggerFactory.getLogger(CloudApplication.class);
    
    public static void main(String[] args) {
        
        logger.info("Starting system ...");

        try {
            DatabaseManager databaseManager = new DatabaseManager();
            CoapObserver coapObserver = new CoapObserver(databaseManager);
            coapObserver.startAllObservation();

            CoapServer server = new CoapServer();
            server.add(new CoAPResourcePlate("plate", databaseManager));
            server.start();
            logger.info("CoAP server avviato sulla porta predefinita (5683).");

            // Keep the application running to continue receiving notifications
            try {
                Thread.sleep(5 * 60 * 1000); // 5 minutes, adjust as needed
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                // Stop observations before exiting
                coapObserver.stopAllObservation();
            }
        } catch (Exception e) {
            logger.error("Failed to start application: {}", e.getMessage(), e);
        }
    }
}