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
            UserInterface userInterface = new UserInterface(databaseManager);

            // Main application loop
            while(true) {
                userInterface.displayMainMenu();
                int choice = userInterface.getUserInput();
                logger.info("User select option number {}", choice);
                
                switch (choice) {
                    case 1:
                        logger.info("User selected to add a new table to the database.");
                        userInterface.addNewPlateToDatabase();
                        break;

                    case 2:
                        logger.info("User selected to update the ML prediction interval.");
                        userInterface.updateMLPredInterval();
                        break;

                    case 3:
                        logger.info("User selected to view the smart grid status.");
                        userInterface.viewStatusSmartGrid();
                        break;

                    case 4:
                        logger.info("User selected to view the charging stations status.");
                        userInterface.viewStatusChargingStations();
                        break;

                    case 5:
                        logger.info("User selected to view storade data.");
                        userInterface.viewStoredData();
                        break;

                    case 6:
                        logger.info("User selected to view if the ML model is running.");
                        if(coapObserver.getIfMLRunning()){
                            System.out.println("\nML Model is running!");
                        }
                        else {
                            System.out.println("\nML Model is not running!");
                        }
                        break;
                    
                    case 7:
                        logger.info("User selected to exit the application.");
                        coapObserver.stopAllObservation();
                        server.stop();
                        server.destroy();
                        databaseManager.closeConnection();
                        userInterface.closeScanner();
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