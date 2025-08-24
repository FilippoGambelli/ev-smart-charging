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

    public void insertSolarData(Timestamp timestamp, Float Gb, Float Gd, Float Gr, Float H_sun, Float T2m, Float WS10m) {
        String sql = "INSERT INTO solarData (timestamp, Gb, Gd, Gr, HSun, T, WS) VALUES (?, ?, ?, ?, ?, ?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setFloat(2, Gb);
            pstmt.setFloat(3, Gd);
            pstmt.setFloat(4, Gr);
            pstmt.setFloat(5, H_sun);
            pstmt.setFloat(6, T2m);
            pstmt.setFloat(7, WS10m);
            pstmt.executeUpdate();
            logger.info("Inserted row into solarData: timestamp={}, Gb={}, Gd={}, Gr={}, HSun={}, T={}, WS={}", timestamp, Gb, Gd, Gr, H_sun, T2m, WS10m);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    public void insertRealPowerData(Timestamp timestamp, Float realPower) {
        String sql = "INSERT INTO realPower (timestamp, realPower) VALUES (?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setFloat(2, realPower);
            pstmt.executeUpdate();
            logger.info("Inserted row into realPower: timestamp={}, realPower={}", timestamp, realPower);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    public int getPriorityByPlate(String plate){
        String sql = "SELECT priority FROM platePriority WHERE plate = ?";
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
        String sql = "INSERT INTO platePriority (plate, priority) VALUES (?, ?)";

        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setString(1, plate);
            pstmt.setInt(2, priority);
            pstmt.executeUpdate();
        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    // Select all rows from platePriority
    public List<PlatePriority> selectAllPlatePriority() {
        List<PlatePriority> results = new ArrayList<>();
        String sql = "SELECT plate, priority FROM platePriority ORDER BY plate ASC";
        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            while (rs.next()) {
                String plate = rs.getString("plate");
                int priority = rs.getInt("priority");
                results.add(new PlatePriority(plate, priority));
            }
        } catch (SQLException e) {
            logger.error("Select from platePriority failed: {}", e.getMessage(), e);
        }
        return results;
    }

    // Select the last N rows from realPower ordered by timestamp DESC
    public List<RealPower> selectRealPower(int limit) {
        List<RealPower> results = new ArrayList<>();
        String sql = "SELECT timestamp, realPower FROM realPower ORDER BY timestamp DESC LIMIT ?";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setInt(1, limit); // Set the number of records to retrieve
            try (ResultSet rs = pstmt.executeQuery()) {
                while (rs.next()) {
                    Timestamp timestamp = rs.getTimestamp("timestamp");
                    Float realPower = rs.getFloat("realPower");
                    results.add(new RealPower(timestamp, realPower));
                }
            }
        } catch (SQLException e) {
            logger.error("Select from realPower failed: {}", e.getMessage(), e);
        }
        return results;
    }

    // Select the last N rows from solarData ordered by timestamp DESC
    public List<SolarData> selectSolarData(int limit) {
        List<SolarData> results = new ArrayList<>();
        String sql = "SELECT timestamp, Gb, Gd, Gr, HSun, T, WS FROM solarData ORDER BY timestamp DESC LIMIT ?";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setInt(1, limit); // Set the number of records to retrieve
            try (ResultSet rs = pstmt.executeQuery()) {
                while (rs.next()) {
                    Timestamp timestamp = rs.getTimestamp("timestamp");
                    Float gb = rs.getFloat("Gb");
                    Float gd = rs.getFloat("Gd");
                    Float gr = rs.getFloat("Gr");
                    Float hsun = rs.getFloat("HSun");
                    Float t = rs.getFloat("T");
                    Float ws = rs.getFloat("WS");
                    results.add(new SolarData(timestamp, gb, gd, gr, hsun, t, ws));
                }
            }
        } catch (SQLException e) {
            logger.error("Select from solarData failed: {}", e.getMessage(), e);
        }
        return results;
    }

    public int countTrailingZerosInRealPower() {
        String sql = "SELECT realPower FROM realPower ORDER BY timestamp ASC";
        int trailingZeros = 0;

        try (Statement stmt = connection.createStatement();
            ResultSet rs = stmt.executeQuery(sql)) {

            // Read all values into a list
            List<Float> pValues = new ArrayList<>();
            while (rs.next()) {
                pValues.add(rs.getFloat("realPower"));
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