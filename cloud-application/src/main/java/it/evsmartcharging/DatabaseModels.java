package it.evsmartcharging;

import java.sql.Timestamp;

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
        private final int pReal;

        public RealPower(Timestamp timestamp, int pReal) {
            this.timestamp = timestamp;
            this.pReal = pReal;
        }

        public Timestamp getTimestamp() {
            return timestamp;
        }

        public int getPReal() {
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
        private final int Gb;
        private final int Gd;
        private final int Gr;
        private final int HSun;
        private final int T;
        private final int WS;

        public SolarData(Timestamp timestamp, int Gb, int Gd, int Gr, int HSun, int T, int WS) {
            this.timestamp = timestamp;
            this.Gb = Gb;
            this.Gd = Gd;
            this.Gr = Gr;
            this.HSun = HSun;
            this.T = T;
            this.WS = WS;
        }

        public Timestamp getTimestamp() { return timestamp; }
        public int getGb() { return Gb; }
        public int getGd() { return Gd; }
        public int getGr() { return Gr; }
        public int getHSun() { return HSun; }
        public int getT() { return T; }
        public int getWS() { return WS; }

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