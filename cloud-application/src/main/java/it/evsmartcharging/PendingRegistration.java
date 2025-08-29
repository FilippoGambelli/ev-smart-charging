package it.evsmartcharging;

import org.eclipse.californium.core.server.resources.CoapExchange;
import com.google.gson.JsonArray;
import java.util.Set;

public class PendingRegistration {
    private final CoapExchange exchange;
    private final Set<String> missingNodes;
    private final String nodeType;
    private final JsonArray requiredNodes;

    public PendingRegistration(CoapExchange exchange, Set<String> missingNodes, String nodeType, JsonArray requiredNodes) {
        this.exchange = exchange;
        this.missingNodes = missingNodes;
        this.nodeType = nodeType;
        this.requiredNodes = requiredNodes;
    }

    public CoapExchange getExchange() { return exchange; }
    public Set<String> getMissingNodes() { return missingNodes; }
    public String getNodeType() { return nodeType; }
    public JsonArray getRequiredNodes() { return requiredNodes; }
}
