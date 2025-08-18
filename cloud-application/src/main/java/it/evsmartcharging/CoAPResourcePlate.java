package it.evsmartcharging;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.Response;
import org.eclipse.californium.core.server.resources.CoapExchange;

public class CoAPResourcePlate extends CoapResource {
	private final DatabaseManager databaseManager;

    public CoAPResourcePlate(String name, DatabaseManager databaseManager) {
        super(name);
        this.databaseManager = databaseManager;
    }

 	public void handleGET(CoapExchange exchange) {

		// Recupero la targa dal path o query
        String plate = null;

        // Se la targa è passata come query string tipo: coap://server/plate?plate=AB123CD
        plate = exchange.getRequestOptions().getUriQuery().stream()
                        .filter(q -> q.startsWith("plate="))
                        .map(q -> q.substring("plate=".length()))
                        .findFirst()
                        .orElse(null);

		if (plate == null || plate.isEmpty()) {
            exchange.respond(ResponseCode.BAD_REQUEST, "Missing plate parameter");
            return;
        }

		int priority = databaseManager.getPriorityByPlate(plate);

		Response response;
		
		if (priority == -1) {
			// Targa non trovata o errore DB
			response = new Response(ResponseCode.NOT_FOUND);
			response.setPayload("priority not found or error");
		} else {
			// Risposta OK
			response = new Response(ResponseCode.CONTENT);
			response.setPayload("priority=" + priority);
		}

		exchange.respond(response);
 	}
}