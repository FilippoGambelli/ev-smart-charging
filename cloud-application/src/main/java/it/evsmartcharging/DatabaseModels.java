package it.evsmartcharging;

import java.sql.Timestamp;

public class DatabaseModels {

    // Represents a row from the platePriority table
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
        private final Float pReal;

        public RealPower(Timestamp timestamp, Float pReal) {
            this.timestamp = timestamp;
            this.pReal = pReal;
        }

        public Timestamp getTimestamp() {
            return timestamp;
        }

        public Float getPReal() {
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
        private final Float Gb;
        private final Float Gd;
        private final Float Gr;
        private final Float HSun;
        private final Float T;
        private final Float WS;

        public SolarData(Timestamp timestamp, Float Gb, Float Gd, Float Gr, Float HSun, Float T, Float WS) {
            this.timestamp = timestamp;
            this.Gb = Gb;
            this.Gd = Gd;
            this.Gr = Gr;
            this.HSun = HSun;
            this.T = T;
            this.WS = WS;
        }

        public Timestamp getTimestamp() { return timestamp; }
        public Float getGb() { return Gb; }
        public Float getGd() { return Gd; }
        public Float getGr() { return Gr; }
        public Float getHSun() { return HSun; }
        public Float getT() { return T; }
        public Float getWS() { return WS; }

        @Override
        public String toString() {
            return "SolarData{timestamp=" + timestamp +
                   ", Gb=" + Gb +
                   ", Gd=" + Gd +
                   ", Gr=" + Gr +
                   ", HSun=" + HSun +
                   ", T=" + T +
                   ", WS=" + WS +
                   "}";
        }
    }
}