package it.evsmartcharging;

import java.util.Scanner;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapResponse;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class UserInterface {

    private static final Logger logger = LoggerFactory.getLogger(UserInterface.class);

    private Scanner scanner;
    private int maxOption;

    private DatabaseManager databaseManager;

    public UserInterface() {
        try {
            scanner = new Scanner(System.in);
            databaseManager = new DatabaseManager();
        } catch (Exception e) {
            logger.error("Failed to start application: {}", e.getMessage(), e);
        }
    }

    public void displayMainMenu() {
        int i = 0;
    
        System.out.println("\n==== EV SMART CHARGING SYSTEM ====");
        System.out.println(++i + ". Add new table to database");
        System.out.println(++i + ". Update ML prediction interval");
        System.out.println(++i + ". View Stored Data");
        System.out.println(++i + ". View Logs");
        System.out.println(++i + ". Exit");
        System.out.print("Choose an option (1-" + i + "): ");

        maxOption = i;
    }

    public int getUserInput() {
        int choice = -1;
        while (true) {
            choice = Integer.parseInt(scanner.nextLine());
            if (choice < 1 && choice > maxOption) {
                System.out.print("Invalid option. Please enter a number between 1 and " + maxOption + ": ");
                continue;
            }
            return choice;
        }
    }

    public void addNewPlateToDatabase() {
        System.out.print("\nEnter plate number: ");
        String plate = scanner.nextLine().trim();

        int priority = -1;
        while (true) {
            System.out.print("Enter priority (0 = no priority, 1 = has priority): ");
            try {
                priority = Integer.parseInt(scanner.nextLine());
                if (priority == 0 || priority == 1) {
                    break;
                } else {
                    System.out.println("Invalid input. Priority must be 0 or 1.");
                }
            } catch (NumberFormatException e) {
                System.out.println("Invalid input. Please enter a number (0 or 1).");
            }
        }

        System.out.print("Are you sure you want to insert plate '" + plate + "' with priority " + priority + "? (y/n): ");
        String confirm = scanner.nextLine().trim().toLowerCase();

        if (confirm.equals("y")) {
            databaseManager.insertPlatePriorityData(plate, priority);
            System.out.println("Record inserted successfully!");
        } else {
            System.out.println("Insertion cancelled.");
        }
    }

    public void updateMLPredInterval() {
        System.out.print("\nEnter the new ML prediction interval in seconds: ");
        String seconds = scanner.nextLine().trim();

        System.out.print("Are you sure you want to modify the ML prediction interval to " + seconds + " seconds? (y/n): ");
        String confirm = scanner.nextLine().trim().toLowerCase();

        if (confirm.equals("y")) {
            try {
                // Create the CoAP client with the IPv6 address of your device
                String uri = "coap://[fd00::202:2:2:2]:5683/res_ml_pred_interval";
                CoapClient client = new CoapClient(uri);

                // Prepare the request payload in key=value format as expected by the server
                String payload = "mlPredInterval=" + seconds;

                // Content-Format 4 = application/x-www-form-urlencoded
                CoapResponse response = client.put(payload, 4);

                if (response != null) {
                    System.out.println(response.getResponseText());
                } else {
                    System.out.println("No response received from the server.");
                }

                client.shutdown();

            } catch (Exception e) {
                System.err.println("Error while sending CoAP request: " + e.getMessage());
                e.printStackTrace();
            }
        } else {
            System.out.println("Update cancelled.");
        }
    }

    public void closeScanner() {
        if (scanner != null) {
            scanner.close();
            logger.info("Scanner closed.");
        }
    }
}