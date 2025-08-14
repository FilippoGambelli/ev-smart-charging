package it.evsmartcharging;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;

public class CoapObserver {

    public static void main(String[] args) {
        // Address of the CoAP server and resource to observe
        String uri = "coap://[fd00::202:2:2:2]:5683/res_solar_obs";

        // Create the CoAP client
        CoapClient client = new CoapClient(uri);

        // Start observation
        CoapObserveRelation relation = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String content = response.getResponseText();
                System.out.println("Notification received: " + content);
            }

            @Override
            public void onError() {
                System.err.println("Error during observation!");
            }
        });

        // Keep the observation for a certain period
        try {
            System.out.println("Observation in progress... press CTRL+C to stop.");
            Thread.sleep(60 * 1000); // 1 minute
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        // Cancel the observation
        relation.proactiveCancel();
        System.out.println("Observation canceled.");
    }
}
