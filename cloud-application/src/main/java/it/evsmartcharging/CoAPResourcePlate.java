package it.evsmartcharging;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.Response;
import org.eclipse.californium.core.server.resources.CoapExchange;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CoAPResourcePlate extends CoapResource {

    private static final Logger logger = LoggerFactory.getLogger(CoAPResourcePlate.class);

    private final DatabaseManager databaseManager;

    public CoAPResourcePlate(String name, DatabaseManager databaseManager) {
        super(name);
        this.databaseManager = databaseManager;
        logger.info("CoAPResourcePlate initialized with name: {}", name);
    }

    @Override
    public void handleGET(CoapExchange exchange) {

        // Retrieve the 'plate' parameter from the query string if present
        String plate = exchange.getRequestOptions().getUriQuery().stream()
                .filter(q -> q.startsWith("plate=")) // Filter for "plate" query
                .map(q -> q.substring("plate=".length())) // Extract plate value
                .findFirst()
                .orElse(null); // Return null if not found

        // Validate plate parameter
        if (plate == null || plate.isEmpty()) {
            logger.warn("GET request missing 'plate' parameter");
            exchange.respond(ResponseCode.BAD_REQUEST, "Missing plate parameter");
            return;
        }

        logger.info("Received GET request for plate: {}", plate);

        // Query the database for priority associated with the plate
        int priority = databaseManager.getPriorityByPlate(plate);

        Response response;

        if (priority == -1) {
            logger.info("Priority not found for plate: {}", plate);
            response = new Response(ResponseCode.CONTENT);
            response.setPayload("priority=0");
        } else {
            // Successful response with priority
            logger.info("Returning priority {} for plate {}", priority, plate);
            response = new Response(ResponseCode.CONTENT);
            response.setPayload("priority=" + priority);
        }

        // Send the response back to the client
        exchange.respond(response);
        logger.info("Response sent for plate {}: {}", plate, response.getCode());
    }
}