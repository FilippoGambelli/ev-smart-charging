package it.evsmartcharging;

import org.eclipse.californium.core.CoapServer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CloudApplication {
    
    private static final Logger logger = LoggerFactory.getLogger(CloudApplication.class);
    
    public static void main(String[] args) {
        
        logger.info("Starting system ...");

        try {

            // Initialize database manager
            DatabaseManager databaseManager = new DatabaseManager();

            // Initialize and start CoAP observer for all devices            
            CoapObserver coapObserver = new CoapObserver(databaseManager);
            coapObserver.startAllObservation();

            // Initialize CoAP server and add resources
            CoapServer server = new CoapServer();
            server.add(new CoAPResourcePlate("plate", databaseManager));
            server.start();
            logger.info("CoAP server started on default port 5683.");

            // Initialize user interface
            UserInterface userInterface = new UserInterface();

            // Main application loop
            while(true) {
                userInterface.displayMainMenu();
                int choice = userInterface.getUserInput();
                logger.info("User select option number {}", choice);
                
                switch (userInterface.getUserInput()) {
                    case 1:
                        logger.info("User selected to add a new table to the database.");
                        userInterface.addNewPlateToDatabase();
                        break;

                    case 2:
                        logger.info("Option 2 selected - functionality not yet implemented.");
                        // TODO: Implement functionality for option 2
                        break;

                    case 3:
                        logger.info("Option 3 selected - functionality not yet implemented.");
                        // TODO: Implement functionality for option 3
                        break;
                    
                    case 4:
                        logger.info("User selected to exit the application.");
                        coapObserver.stopAllObservation();  // Stop all CoAP observations
                        server.stop();  // Stop CoAP server
                        server.destroy();   // Destroy CoAP server
                        databaseManager.closeConnection();  // Close database connection
                        userInterface.closeScanner();   // Close user interface scanner
                        logger.info("System shutting down. Goodbye!");
                        return;
                
                    default:
                        break;
                }
            }
        } catch (Exception e) {
            logger.error("Failed to start application: {}", e.getMessage(), e);
        }
    }
}