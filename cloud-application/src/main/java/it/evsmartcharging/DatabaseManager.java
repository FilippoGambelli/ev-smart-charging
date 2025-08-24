package it.evsmartcharging;

import it.evsmartcharging.DatabaseModels.PlatePriority;
import it.evsmartcharging.DatabaseModels.RealPower;
import it.evsmartcharging.DatabaseModels.SolarData;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.sql.*;

public class DatabaseManager {

    private static final Logger logger = LoggerFactory.getLogger(DatabaseManager.class);

    // Database connection details
    private static final String DB_URL = "jdbc:mysql://localhost:3306/evSmartCharging";
    private static final String DB_USER = "root";
    private static final String DB_PASSWORD = "PASSWORD";

    private Connection connection;

    public DatabaseManager() throws SQLException {
        logger.info("Initializing database connection");
        this.connection = DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
        logger.info("Database connection established successfully");
    }

    public Connection getConnection() throws SQLException {
        return DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
    }

    public void insertSolarData(Timestamp timestamp, int Gb, int Gd, int Gr, int H_sun, int T2m, int WS10m) {
        String sql = "INSERT INTO solarData (timestamp, Gb, Gd, Gr, HSun, T, WS) VALUES (?, ?, ?, ?, ?, ?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setInt(2, Gb);
            pstmt.setInt(3, Gd);
            pstmt.setInt(4, Gr);
            pstmt.setInt(5, H_sun);
            pstmt.setInt(6, T2m);
            pstmt.setInt(7, WS10m);
            pstmt.executeUpdate();
            logger.info("Inserted row into solarData: timestamp={}, Gb={}, Gd={}, Gr={}, HSun={}, T={}, WS={}", timestamp, Gb, Gd, Gr, H_sun, T2m, WS10m);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    public void insertRealPowerData(Timestamp timestamp, int P) {
        String sql = "INSERT INTO realPower (timestamp, P_real) VALUES (?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setInt(2, P);
            pstmt.executeUpdate();
            logger.info("Inserted row into realPower: timestamp={}, P_real={}", timestamp, P);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    public int getPriorityByPlate(String plate){
        String sql = "SELECT priority FROM plate_priority WHERE plate = ?";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setString(1, plate);
            ResultSet rs = pstmt.executeQuery();
            if (rs.next()) {
                return rs.getInt("priority"); // return 0 or 1
            } else {
                return -1; // default if the plate is not present
            }
        } catch (SQLException e) {
            e.printStackTrace();
            return -1;
        }
    }

    public void insertPlatePriorityData(String plate, int priority) {
        String sql = "INSERT INTO plate_priority (plate, priority) VALUES (?, ?)";

        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setString(1, plate);
            pstmt.setInt(2, priority);
            pstmt.executeUpdate();
        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    // Select all rows from plate_priority
    public List<PlatePriority> selectAllPlatePriority() {
        List<PlatePriority> results = new ArrayList<>();
        String sql = "SELECT plate, priority FROM plate_priority ORDER BY plate ASC";
        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            while (rs.next()) {
                String plate = rs.getString("plate");
                int priority = rs.getInt("priority");
                results.add(new PlatePriority(plate, priority));
            }
        } catch (SQLException e) {
            logger.error("Select from plate_priority failed: {}", e.getMessage(), e);
        }
        return results;
    }

    // Select all rows from realPower ordered by timestamp
    public List<RealPower> selectAllRealPower() {
        List<RealPower> results = new ArrayList<>();
        String sql = "SELECT timestamp, P_real FROM realPower ORDER BY timestamp ASC";
        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            while (rs.next()) {
                Timestamp timestamp = rs.getTimestamp("timestamp");
                Integer pReal = rs.getInt("P_real");
                results.add(new RealPower(timestamp, pReal));
            }
        } catch (SQLException e) {
            logger.error("Select from realPower failed: {}", e.getMessage(), e);
        }
        return results;
    }

    // Select all rows from solarData ordered by timestamp
    public List<SolarData> selectAllSolarData() {
        List<SolarData> results = new ArrayList<>();
        String sql = "SELECT timestamp, Gb, Gd, Gr, HSun, T, WS FROM solarData ORDER BY timestamp ASC";
        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            while (rs.next()) {
                Timestamp timestamp = rs.getTimestamp("timestamp");
                int gb = rs.getInt("Gb");
                int gd = rs.getInt("Gd");
                int gr = rs.getInt("Gr");
                int hsun = rs.getInt("HSun");
                int t = rs.getInt("T");
                int ws = rs.getInt("WS");
                results.add(new SolarData(timestamp, gb, gd, gr, hsun, t, ws));
            }
        } catch (SQLException e) {
            logger.error("Select from solarData failed: {}", e.getMessage(), e);
        }
        return results;
    }

    public int countTrailingZerosInRealPower() {
        String sql = "SELECT P_real FROM realPower ORDER BY timestamp ASC";
        int trailingZeros = 0;

        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            // Read all values into a list
            List<Integer> pValues = new ArrayList<>();
            while (rs.next()) {
                pValues.add(rs.getInt("P_real"));
            }

            // Count trailing zeros starting from the end
            for (int i = pValues.size() - 1; i >= 0; i--) {
                if (pValues.get(i) == 0) {
                    trailingZeros++;
                } else {
                    break;
                }
            }

        } catch (SQLException e) {
            logger.error("Select from realPower failed: {}", e.getMessage(), e);
        }

        return trailingZeros;
    }

    public void closeConnection() {
        if (connection != null) {
            try {
                connection.close();
                logger.info("Database connection closed.");
            } catch (SQLException e) {
                logger.error("Error closing database connection: " + e.getMessage());
            }
        }
    }
}