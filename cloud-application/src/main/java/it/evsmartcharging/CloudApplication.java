package it.evsmartcharging;

import java.util.logging.LogManager;
import java.util.logging.Logger;
import org.eclipse.californium.core.CoapServer;
import org.slf4j.LoggerFactory;

public class CloudApplication {
    
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(CloudApplication.class);
    
    public static void main(String[] args) {
        LogManager.getLogManager().reset();
        Logger.getLogger("").setLevel(java.util.logging.Level.OFF);
        
        logger.info("Starting system ...");
        System.out.println("Starting system ...");

        try {

            // Initialize database manager
            DatabaseManager databaseManager = new DatabaseManager();

            // Initialize CoAP server and add resources
            CoapServer server = new CoapServer();
            CoAPResourceRegistration registrationResource = new CoAPResourceRegistration("registration", databaseManager);
            server.add(registrationResource);
            server.add(new CoAPResourcePlate("plate", databaseManager));
            server.start();
            logger.info("CoAP server started on default port 5683.");

            // Wait until all required nodes are registered
            while (registrationResource.getSensorPVEndpoint().equals("0") ||
                registrationResource.getCentralNodeEndpoint().equals("0") ||
                registrationResource.getSmartGridEndpoint().equals("0")) {
                
                logger.warn("Not all nodes are registered yet. Waiting 2 seconds before retrying...");
                try {
                    Thread.sleep(2000); // wait 2 seconds
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    logger.error("Wait interrupted: {}", e.getMessage(), e);
                }
            }

            String CENTRAL_NODE_EP = "coap://[" + registrationResource.getCentralNodeEndpoint() + "]:5683";
            String SENSOR_PV_EP = "coap://[" + registrationResource.getSensorPVEndpoint() + "]:5683";
            String SMART_GRID_EP = "coap://[" + registrationResource.getSmartGridEndpoint() + "]:5683";

            logger.info("All required nodes are registered. Starting observation.");

            // Initialize and start CoAP observer for all devices            
            CoapObserver coapObserver = new CoapObserver(databaseManager, SENSOR_PV_EP, CENTRAL_NODE_EP);
            coapObserver.startAllObservation();

            // Initialize user interface
            UserInterface userInterface = new UserInterface(databaseManager, CENTRAL_NODE_EP, SMART_GRID_EP);

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
                        logger.info("User selected to start/stop ML model");
                        userInterface.startStopMLModel(coapObserver);
                        break;

                    case 3:
                        logger.info("User selected to update the ML prediction interval.");
                        userInterface.updateMLPredInterval();
                        break;

                    case 4:
                        logger.info("User selected to view the smart grid status.");
                        userInterface.viewStatusSmartGrid();
                        break;

                    case 5:
                        logger.info("User selected to view the charging stations status.");
                        userInterface.viewStatusChargingStations();
                        break;

                    case 6:
                        logger.info("User selected to view storade data.");
                        userInterface.viewStoredData();
                        break;

                    case 7:
                        logger.info("User selected to view if the ML model is running.");
                        if(coapObserver.getIfMLRunning()){
                            System.out.println("\nML Model is running!");
                        }
                        else {
                            System.out.println("\nML Model is not running!");
                        }
                        break;

                    case 8:
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