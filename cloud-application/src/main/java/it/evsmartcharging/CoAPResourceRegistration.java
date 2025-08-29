package it.evsmartcharging;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;
import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.server.resources.CoapExchange;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.eclipse.californium.core.coap.CoAP;
import org.eclipse.californium.core.coap.MediaTypeRegistry;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class CoAPResourceRegistration extends CoapResource {

    private static final Logger logger = LoggerFactory.getLogger(CoAPResourceRegistration.class);

    private final DatabaseManager databaseManager;

    private final Map<String, String> registeredNodes = new ConcurrentHashMap<>();

    private final List<PendingRegistration> pendingList = Collections.synchronizedList(new ArrayList<>());

    private int numChargingStation = 0;

    public CoAPResourceRegistration(String name, DatabaseManager databaseManager) {
        super(name);
        this.databaseManager = databaseManager;
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        try {
            String payload = exchange.getRequestText();
            JsonObject json = JsonParser.parseString(payload).getAsJsonObject();

            String nodeType = json.get("nodeType").getAsString();
            JsonArray requiredNodes = json.getAsJsonArray("requiredNodes");

            String endpoint = exchange.getSourceAddress().getHostAddress();

            logger.info("Received registration request from {}: {}", endpoint, json);

            if(nodeType.equals("chargingStation")) {
                databaseManager.insertDevice(numChargingStation, nodeType, endpoint);
                numChargingStation++;
            }
            else {
                databaseManager.insertDevice(0, nodeType, endpoint);
                registeredNodes.put(nodeType, endpoint);
            }

            Set<String> missing = new HashSet<>();
            for (int i = 0; i < requiredNodes.size(); i++) {
                String req = requiredNodes.get(i).getAsString();
                if (!registeredNodes.containsKey(req)) {
                    missing.add(req);
                }
            }
            
            if (missing.isEmpty()) {
                if(requiredNodes.size() == 0){
                    exchange.respond(CoAP.ResponseCode.CREATED);
                }
                String response = buildResponse(nodeType, requiredNodes);
                exchange.respond(CoAP.ResponseCode.CONTENT, response, MediaTypeRegistry.TEXT_PLAIN);
            } else {
                pendingList.add(new PendingRegistration(exchange, missing, nodeType, requiredNodes));
            }

            checkPendingRegistrations(nodeType);

        } catch (Exception e) {
            exchange.respond(CoAP.ResponseCode.BAD_REQUEST, "Invalid JSON: " + e.getMessage(),
                    MediaTypeRegistry.TEXT_PLAIN);
        }
    }

    private void checkPendingRegistrations(String newlyRegisteredNode) {
        synchronized (pendingList) {
            Iterator<PendingRegistration> it = pendingList.iterator();
            while (it.hasNext()) {
                PendingRegistration pr = it.next();
                pr.getMissingNodes().remove(newlyRegisteredNode);

                if (pr.getMissingNodes().isEmpty()) {
                    String response = buildResponse(pr.getNodeType(), pr.getRequiredNodes());
                    pr.getExchange().respond(CoAP.ResponseCode.CONTENT, response, MediaTypeRegistry.TEXT_PLAIN);
                    logger.info("Pending registration for {} completed. Sent response: {}", pr.getNodeType(), response);
                    it.remove();
                }
            }
        }
    }

    private String buildResponse(String selfNode, JsonArray requiredNodes) {
        StringJoiner resp = new StringJoiner("&");
        for (int i = 0; i < requiredNodes.size(); i++) {
            String req = requiredNodes.get(i).getAsString();
            if (registeredNodes.containsKey(req)) {
                resp.add(req + "=" + registeredNodes.get(req));
            }
        }
        return resp.toString();
    }

    public String getSensorPVEndpoint() {
        return registeredNodes.getOrDefault("sensorPV", "0");
    }

    public String getCentralNodeEndpoint() {
        return registeredNodes.getOrDefault("centralNode", "0");
    }

    public String getSmartGridEndpoint() {
        return registeredNodes.getOrDefault("smartGrid", "0");
    }

}