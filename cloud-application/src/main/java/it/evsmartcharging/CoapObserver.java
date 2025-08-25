package it.evsmartcharging;

import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class CoapObserver {

    private static final Logger logger = LoggerFactory.getLogger(CoapObserver.class);

    private volatile boolean ML_RUNNING = true;
    
    private final DatabaseManager databaseConnection;

    // Observation relations for solar and real power data
    private CoapObserveRelation relationSolarData;
    private CoapObserveRelation relationRealPowerData;
    
    public CoapObserver(DatabaseManager databaseManager) {
        this.databaseConnection = databaseManager;
        logger.info("CoapObserver initialized with DatabaseManager");
    }

    // Start all CoAP observations
    public void startAllObservation(){
        logger.info("Starting all CoAP observations...");
        startObservationSolarData();
        startObservationRealPowerData();
    }

    // Stop all CoAP observations
    public void stopAllObservation(){
        logger.info("Stopping all CoAP observations...");
        stopObservationRealPowerData();
        stopObservationSolarData();
    }

    // Start observing solar data
    public void startObservationSolarData(){
        String uri = Config.SENSOR_PV_EP + "/solar_obs";
        logger.info("Initializing CoAP client for solar data at {}", uri);

        CoapClient client = new CoapClient(uri);

        // Start observing the solar resource
        relationSolarData = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String content = response.getResponseText();

                try {
                    // Parse the JSON response
                    JsonObject obj = JsonParser.parseString(content).getAsJsonObject();

                    // Parse timestamp if present
                    Timestamp timestamp = null;
                    if (obj.has("Time") && !obj.get("Time").isJsonNull()) {
                        String tsString = obj.get("Time").getAsString();
                        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("dd-MM-yyyy HH:mm:ss");
                        LocalDateTime ldt = LocalDateTime.parse(tsString, formatter);
                        Instant instant = ldt.atZone(java.time.ZoneOffset.UTC).toInstant();
                        timestamp = Timestamp.from(instant);
                    }

                    // Parse solar parameters, default to null if missing
                    Float Gb = obj.has("Gb") && !obj.get("Gb").isJsonNull()
                            ? obj.get("Gb").getAsFloat() : null;

                    Float Gd = obj.has("Gd") && !obj.get("Gd").isJsonNull()
                            ? obj.get("Gd").getAsFloat() : null;

                    Float Gr = obj.has("Gr") && !obj.get("Gr").isJsonNull()
                            ? obj.get("Gr").getAsFloat() : null;

                    Float HSun = obj.has("HSun") && !obj.get("HSun").isJsonNull()
                            ? obj.get("HSun").getAsFloat() : null;

                    Float T = obj.has("T") && !obj.get("T").isJsonNull()
                            ? obj.get("T").getAsFloat() : null;

                    Float WS = obj.has("WS") && !obj.get("WS").isJsonNull()
                            ? obj.get("WS").getAsFloat() : null;

                    // Insert the parsed data into the database
                    databaseConnection.insertSolarData(timestamp, Gb, Gd, Gr, HSun, T, WS);

                } catch (Exception e) {
                    logger.error("Error parsing JSON for solar data: {}", e.getMessage(), e);
                }
            }

            @Override
            public void onError() {
                logger.error("Error during solar observation!");
            }
        });

        logger.info("Started observation for /solar_obs");
    }

    // Stop observing solar data
    public void stopObservationSolarData(){
        if (relationSolarData != null) {
            relationSolarData.proactiveCancel();
            logger.info("Observation canceled for /solar_obs");
            relationSolarData = null;
        } else {
            logger.warn("No active solar observation to cancel");
        }
    }

    // Start observing real power data
    public void startObservationRealPowerData() {
        String uri = Config.SENSOR_PV_EP + "/real_power_obs";
        logger.info("Initializing CoAP client for real power data at {}", uri);

        CoapClient client = new CoapClient(uri);

        // Start observing the real power resource
        relationRealPowerData = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String content = response.getResponseText();

                try {
                    // Example content format: "timestamp=1692541800&realPV=123.45"
                    String[] params = content.split("&");
                    Timestamp timestamp = null;
                    Float realPV = null;

                    // Parse key-value pairs from response
                    for (String param : params) {
                        String[] keyValue = param.split("=", 2);
                        if (keyValue.length != 2) continue;

                        String key = keyValue[0].trim();
                        String value = keyValue[1].trim();

                        switch (key) {
                            case "timestamp":
                                long epochSeconds = Long.parseLong(value);
                                Instant instant = Instant.ofEpochSecond(epochSeconds);
                                timestamp = Timestamp.from(instant);
                                break;

                            case "realPV":
                                realPV = Float.parseFloat(value);
                                break;

                            default:
                                logger.warn("Unknown parameter received: {}", key);
                                break;
                        }
                    }

                    // Insert valid data into the database
                    if (timestamp != null && realPV != null) {
                        databaseConnection.insertRealPowerData(timestamp, realPV);
                    } else {
                        logger.warn("Incomplete real power data received: {}", content);
                    }

                    if(!ML_RUNNING && realPV > 0){
                        // startMLModel(); [Only for demonstration]
                    }
                    else if (ML_RUNNING && databaseConnection.countTrailingZerosInRealPower() >= 360) {       // 30 minutes of zeros
                        stopMLModel();
                    }
                } catch (Exception e) {
                    logger.error("Error parsing real power observation data: {}", e.getMessage(), e);
                }
            }

            @Override
            public void onError() {
                logger.error("Error during observation for /real_power_obs!");
            }
        });

        logger.info("Started observation for /real_power_obs");
    }

    public void startMLModel() {
        if (ML_RUNNING) {
            return;
        }

        try {
            String mlUri = Config.CENTRAL_NODE_EP + "/ml_pred_interval";
            CoapClient mlClient = new CoapClient(mlUri);

            String payload = "runMLModel=1";
            logger.info("Sending PUT request to {} with payload: {}", mlUri, payload);

            CoapResponse mlResponse = mlClient.put(payload, 0);

            if (mlResponse != null) {
                if (mlResponse.isSuccess()) {
                    ML_RUNNING = true;
                    logger.info("ML model started successfully. Response code: {}, payload: {}", mlResponse.getCode(), mlResponse.getResponseText());
                } else {
                    ML_RUNNING = false;
                    logger.warn("Failed to start ML model. Response code: {}, payload: {}", mlResponse.getCode(), mlResponse.getResponseText());
                }
            } else {
                logger.error("No response received from {}", mlUri);
            }

        } catch (Exception e) {
            logger.error("Error while sending PUT to ML prediction interval: {}", e.getMessage(), e);
        }
    }

    public void stopMLModel() {
        if (!ML_RUNNING) {
            return;
        }

        try {
            String mlUri = Config.CENTRAL_NODE_EP + "/ml_pred_interval";
            CoapClient mlClient = new CoapClient(mlUri);

            String payload = "runMLModel=0";
            logger.info("Sending PUT request to {} with payload: {}", mlUri, payload);

            CoapResponse mlResponse = mlClient.put(payload, 0);

            if (mlResponse != null) {
                if (mlResponse.isSuccess()) {
                    ML_RUNNING = false;
                    logger.info("ML model stopped successfully. Response code: {}, payload: {}", mlResponse.getCode(), mlResponse.getResponseText());
                } else {
                    logger.warn("Failed to stop ML model. Response code: {}, payload: {}", mlResponse.getCode(), mlResponse.getResponseText());
                }
            } else {
                logger.error("No response received from {}", mlUri);
            }

        } catch (Exception e) {
            logger.error("Error while sending PUT to ML prediction interval: {}", e.getMessage(), e);
        }
    }

    // Stop observing real power data
    public void stopObservationRealPowerData(){
        if (relationRealPowerData != null) {
            relationRealPowerData.proactiveCancel();
            logger.info("Observation canceled for /real_power_obs");
            relationRealPowerData = null;
        } else {
            logger.warn("No active real power observation to cancel");
        }
    }

    public boolean getIfMLRunning(){
        return ML_RUNNING;
    }
}