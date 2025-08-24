package it.evsmartcharging;

import java.util.List;
import java.util.Scanner;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapResponse;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;

import it.evsmartcharging.DatabaseModels.PlatePriority;
import it.evsmartcharging.DatabaseModels.RealPower;
import it.evsmartcharging.DatabaseModels.SolarData;

public class UserInterface {

    private static final float COST_PER_KWH_FROM_GRID = 0.25f;  // cost €/kWh taken from the grid
    private static final float EARNING_PER_KWH_TO_GRID = 0.15f; // earning €/kWh injected into the grid

    private static final Logger logger = LoggerFactory.getLogger(UserInterface.class);

    private Scanner scanner;
    private int maxOption;

    private DatabaseManager databaseManager;

    public UserInterface(DatabaseManager databaseManager) {
        try {
            scanner = new Scanner(System.in);
            this.databaseManager = databaseManager;
        } catch (Exception e) {
            logger.error("Failed to start application: {}", e.getMessage(), e);
        }
    }

    public void displayMainMenu() {
        int i = 0;
    
        System.out.println("\n==== EV SMART CHARGING SYSTEM ====");
        System.out.println(++i + ". Add new plate to database");
        System.out.println(++i + ". Update ML prediction interval");
        System.out.println(++i + ". View status smart grid");
        System.out.println(++i + ". View status charging stations");
        System.out.println(++i + ". View Stored Data");
        System.out.println(++i + ". Check if the ML model is running");
        System.out.println(++i + ". Exit");
        System.out.print("Choose an option (1-" + i + "): ");

        maxOption = i;
    }

    public void viewStatusChargingStations() {
        System.out.println("\nCharging Station Status\n");
        try {
            // Create the CoAP client with the IPv6 address of your device
            String uri = Config.CENTRAL_NODE_EP + "/registration/charger";
            CoapClient client = new CoapClient(uri);

            // Perform GET request
            CoapResponse response = client.get();

            if (response != null && response.isSuccess()) {
                String content = response.getResponseText();

                // Parse the JSON response as array
                JsonArray chargers = JsonParser.parseString(content).getAsJsonArray();

                // Print table header
                System.out.printf(
                    "%-3s %-8s %-6s %-6s %-8s %-8s %-8s %-8s %-8s %-8s %-10s %-6s %-8s %-8s %-8s%n",
                    "ID", "maxP", "carR", "isCh", "aP", "gP", "vMaxP", "vCap", "socC", "socT", "plate", "prio", "estDur", "remT", "remE"
                );
                
                // Iterate over each charging station and print row
                for (JsonElement elem : chargers) {
                    System.out.println("-------------------------------------------------------------------------------------------------------------------------------------");
                    JsonObject obj = elem.getAsJsonObject();

                    System.out.printf(
                        "%-3d %-8.2f %-6s %-6s %-8.2f %-8.2f %-8.2f %-8.2f %-8.2f %-8.2f %-10s %-6d %-8d %-8d %-8.2f%n",
                        obj.get("id").getAsInt(),
                        obj.get("maxP").getAsFloat(),
                        obj.get("carReg").getAsBoolean() ? "true" : "false",
                        obj.get("isCh").getAsBoolean() ? "true" : "false",
                        obj.get("aP").getAsFloat(),
                        obj.get("gP").getAsFloat(),
                        obj.get("vMaxP").getAsFloat(),
                        obj.get("vCap").getAsFloat(),
                        obj.get("socCur").getAsFloat(),
                        obj.get("socTgt").getAsFloat(),
                        obj.get("plate").getAsString(),
                        obj.get("prio").getAsInt(),
                        obj.get("estDur").getAsInt(),
                        obj.get("remT").getAsInt(),
                        obj.get("remE").getAsFloat()
                    );
                }

            } else {
                System.err.println("No response or request failed!");
            }

            client.shutdown();

        } catch (Exception e) {
            System.err.println("Error while sending CoAP request: " + e.getMessage());
            e.printStackTrace();
        }
    }

    public void viewStatusSmartGrid() {
        System.out.println("\nSmart Grid Status");
        try {
            // Create the CoAP client with the IPv6 address of your device
            String uri = Config.SMART_GRID_EP + "/status_smart_grid";
            CoapClient client = new CoapClient(uri);

            // Perform GET request
            CoapResponse response = client.get();

            if (response != null && response.isSuccess()) {
                String content = response.getResponseText();

                // Parse the JSON response
                JsonObject obj = JsonParser.parseString(content).getAsJsonObject();

                Float powerFromGridNow = obj.get("power_from_grid").getAsFloat();
                Float powerToGridNow = obj.get("power_to_grid").getAsFloat();
                Float energyFromGridTotal = obj.get("energy_from_grid_total").getAsFloat();
                Float energyToGridTotal = obj.get("energy_to_grid_total").getAsFloat();

                Float totalCost = energyFromGridTotal * COST_PER_KWH_FROM_GRID;
                Float totalEarning = energyToGridTotal * EARNING_PER_KWH_TO_GRID;
                Float totalBalance = totalEarning - totalCost;

                // Print current status
                System.out.println("Current status:");
                System.out.println(" - Power from grid now: " + powerFromGridNow + " kW");
                System.out.println(" - Power to grid now: " + powerToGridNow + " kW");

                // Print total status
                System.out.println("Total status:");
                System.out.println(" - Energy from grid total: " + energyFromGridTotal + " kWh (Total Cost: €" + String.format("%.2f", totalCost) + ")");
                System.out.println(" - Energy to grid total: " + energyToGridTotal + " kWh (Total Earning: €" + String.format("%.2f", totalEarning) + ")");
                System.out.println(" - Total balance: €" + String.format("%.2f", totalBalance));

            } else {
                System.err.println("No response or request failed!");
            }

            client.shutdown();

        } catch (Exception e) {
            System.err.println("Error while sending CoAP request: " + e.getMessage());
            e.printStackTrace();
        }
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

    public void viewStoredData() {
        System.out.println("\nWhat data would you like to see?");
        System.out.println("1 - Real power data");
        System.out.println("2 - Solar irradiation data");
        System.out.println("3 - Plates stored in the database");
        System.out.print("Enter your choice (1, 2, or 3): ");

        String choice = scanner.nextLine().trim();

        if (choice.equals("1")) {
            // Ask how many records to display
            System.out.print("How many real power records would you like to see? ");
            int limit = Integer.parseInt(scanner.nextLine().trim());

            List<RealPower> realPowerData = databaseManager.selectRealPower(limit);
            System.out.println("\n--- Real Power Data ---");
            for (RealPower rp : realPowerData) {
                System.out.printf("Timestamp: %s | Real power: %.4f\n", rp.getTimestamp(), rp.getPReal());
            }

        } else if (choice.equals("2")) {
            // Ask how many records to display
            System.out.print("How many solar irradiation records would you like to see? ");
            int limit = Integer.parseInt(scanner.nextLine().trim());

            List<SolarData> solarDataList = databaseManager.selectSolarData(limit);
            System.out.println("\n--- Solar Irradiation Data ---");
            for (SolarData sd : solarDataList) {
                System.out.printf(
                    "Timestamp: %s | Gb: %.4f | Gd: %.4f | Gr: %.4f | HSun: %.4f | T: %.4f | WS: %.4f\n",
                    sd.getTimestamp(), sd.getGb(), sd.getGd(), sd.getGr(),
                    sd.getHSun(), sd.getT(), sd.getWS()
                );
            }

        } else if (choice.equals("3")) {
            List<PlatePriority> platePriorities = databaseManager.selectAllPlatePriority();
            System.out.println("\n--- Plate Priority Data ---");
            for (PlatePriority pp : platePriorities) {
                System.out.printf("Plate: %s | Priority: %d\n", pp.getPlate(), pp.getPriority());
            }
        } else {
            System.out.println("Invalid choice!");
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
                String uri = Config.CENTRAL_NODE_EP + "/ml_pred_interval";
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