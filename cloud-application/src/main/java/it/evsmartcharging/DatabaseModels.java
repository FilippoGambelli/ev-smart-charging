package it.evsmartcharging;

import java.sql.Timestamp;

/**
 * DTO classes for database entities used by DatabaseManager
 */
public class DatabaseModels {

    // Represents a row from the plate_priority table
    public static class PlatePriority {
        private final String plate;
        private final int priority;

        public PlatePriority(String plate, int priority) {
            this.plate = plate;
            this.priority = priority;
        }

        public String getPlate() {
            return plate;
        }

        public int getPriority() {
            return priority;
        }

        @Override
        public String toString() {
            return "PlatePriority{plate='" + plate + "', priority=" + priority + "}";
        }
    }

    // Represents a row from the realPower table
    public static class RealPower {
        private final Timestamp timestamp;
        private final float pReal;

        public RealPower(Timestamp timestamp, float pReal) {
            this.timestamp = timestamp;
            this.pReal = pReal;
        }

        public Timestamp getTimestamp() {
            return timestamp;
        }

        public float getPReal() {
            return pReal;
        }

        @Override
        public String toString() {
            return "RealPower{timestamp=" + timestamp + ", pReal=" + pReal + "}";
        }
    }

    // Represents a row from the solarData table
    public static class SolarData {
        private final Timestamp timestamp;
        private final float Gb;
        private final float Gd;
        private final float Gr;
        private final float HSun;
        private final float T;
        private final float WS;
        private final float pPredicted;

        public SolarData(Timestamp timestamp, float Gb, float Gd, float Gr, float HSun, float T, float WS, float pPredicted) {
            this.timestamp = timestamp;
            this.Gb = Gb;
            this.Gd = Gd;
            this.Gr = Gr;
            this.HSun = HSun;
            this.T = T;
            this.WS = WS;
            this.pPredicted = pPredicted;
        }

        public Timestamp getTimestamp() { return timestamp; }
        public float getGb() { return Gb; }
        public float getGd() { return Gd; }
        public float getGr() { return Gr; }
        public float getHSun() { return HSun; }
        public float getT() { return T; }
        public float getWS() { return WS; }
        public float getPPredicted() { return pPredicted; }

        @Override
        public String toString() {
            return "SolarData{timestamp=" + timestamp +
                   ", Gb=" + Gb +
                   ", Gd=" + Gd +
                   ", Gr=" + Gr +
                   ", HSun=" + HSun +
                   ", T=" + T +
                   ", WS=" + WS +
                   ", P_predicted=" + pPredicted + "}";
        }
    }
}