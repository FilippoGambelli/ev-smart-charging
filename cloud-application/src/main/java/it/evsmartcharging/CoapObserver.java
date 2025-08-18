package it.evsmartcharging;

import java.sql.Timestamp;
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
    
    private final DatabaseManager databaseConnection;

    private CoapObserveRelation relationSolarData;
    private CoapObserveRelation relationRealPowerData;
    
    public CoapObserver(DatabaseManager databaseManager) {
        this.databaseConnection = databaseManager;
    }


    public void startAllObservation(){
        startObservationSolarData();
        startObservationRealPowerData();
    }

    public void stopAllObservation(){
        stopObservationRealPowerData();
        stopObservationSolarData();
    }

    public void startObservationSolarData(){
        String uri = "coap://[fd00::202:2:2:2]:5683/res_solar_obs";

        // Create the CoAP client
        CoapClient client = new CoapClient(uri);

        // Start observation
        relationSolarData = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String content = response.getResponseText();
                try {
                    JsonObject obj = JsonParser.parseString(content).getAsJsonObject();

                    Timestamp timestamp = null;
                    if (obj.has("Time") && !obj.get("Time").isJsonNull()) {
                        String tsString = obj.get("Time").getAsString();
                        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("dd-MM-yyyy HH:mm:ss");
                        LocalDateTime ldt = LocalDateTime.parse(tsString, formatter);
                        timestamp = Timestamp.valueOf(ldt);
                    }

                    float Gb = obj.has("Gb") && !obj.get("Gb").isJsonNull()
                            ? obj.get("Gb").getAsFloat() : null;

                    float Gd = obj.has("Gd") && !obj.get("Gd").isJsonNull()
                            ? obj.get("Gd").getAsFloat() : null;

                    float Gr = obj.has("Gr") && !obj.get("Gr").isJsonNull()
                            ? obj.get("Gr").getAsFloat() : null;

                    float HSun = obj.has("HSun") && !obj.get("HSun").isJsonNull()
                            ? obj.get("HSun").getAsFloat() : null;

                    float T = obj.has("T") && !obj.get("T").isJsonNull()
                            ? obj.get("T").getAsFloat() : null;

                    float WS = obj.has("WS") && !obj.get("WS").isJsonNull()
                            ? obj.get("WS").getAsFloat() : null;

                    databaseConnection.insertSolarData(timestamp, Gb, Gd, Gr, HSun, T, WS, 0);
                } catch (Exception e) {
                    logger.error("Error parsing JSON: " + e.getMessage());
                }
            }

            @Override
            public void onError() {
                logger.error("Error during observation!");
            }
        });

        logger.info("Start observation for /res_solar_obs");
    }

    public void stopObservationSolarData(){
        if (relationSolarData != null) {
            relationSolarData.proactiveCancel();
            logger.info("Observation canceled for /res_solar_obs");
            relationSolarData = null;
        }
    }

    public void startObservationRealPowerData(){
        String uri = "coap://[fd00::202:2:2:2]:5683/res_real_power_obs";

        // Create the CoAP client
        CoapClient client = new CoapClient(uri);

        // Start observation
        relationRealPowerData = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String content = response.getResponseText();
                try {
                    JsonObject obj = JsonParser.parseString(content).getAsJsonObject();

                    Timestamp timestamp = null;
                    if (obj.has("Timestamp") && !obj.get("Timestamp").isJsonNull()) {
                        String tsString = obj.get("Timestamp").getAsString();
                        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("dd-MM-yyyy HH:mm:ss");
                        LocalDateTime ldt = LocalDateTime.parse(tsString, formatter);
                        timestamp = Timestamp.valueOf(ldt);
                    }

                    float P = obj.has("P") && !obj.get("P").isJsonNull()
                            ? obj.get("P").getAsFloat() : null;

                    databaseConnection.insertRealPowerData(timestamp, P);
                } catch (Exception e) {
                    logger.error("Error parsing JSON: " + e.getMessage());
                }
            }

            @Override
            public void onError() {
                logger.error("Error during observation!");
            }
        });

        logger.info("Start observation for /res_real_power_obs");
    }

    public void stopObservationRealPowerData(){
        if (relationRealPowerData != null) {
            relationRealPowerData.proactiveCancel();
            logger.info("Observation canceled for /res_real_power_obs");
            relationRealPowerData = null;
        }
    }
}