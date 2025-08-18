package it.evsmartcharging;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.*;

/**
 * DatabaseManager provides reusable methods to interact with the evSmartCharging database.
 * It supports inserting, selecting, and deleting data from the solarData table.
 */
public class DatabaseManager {

    private static final Logger logger = LoggerFactory.getLogger(DatabaseManager.class);

    // Database connection details
    private static final String DB_URL = "jdbc:mysql://localhost:3306/evSmartCharging";
    private static final String DB_USER = "root";
    private static final String DB_PASSWORD = "PASSWORD";

    /**
     * Creates and returns a connection to the database
     */
    public Connection getConnection() throws SQLException {
        return DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
    }

    /**
     * Inserts a record into the solarData table
     */
    public void insertSolarData(Connection conn, Timestamp timestamp, float Gb, float Gd, float Gr, float H_sun, float T2m, float WS10m, float P) {
        String sql = "INSERT INTO solarData (timestamp, Gb, Gd, Gr, HSun, T, WS, P_predicted) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        try (PreparedStatement pstmt = conn.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setFloat(2, Gb);
            pstmt.setFloat(3, Gd);
            pstmt.setFloat(4, Gr);
            pstmt.setFloat(5, H_sun);
            pstmt.setFloat(6, T2m);
            pstmt.setFloat(7, WS10m);
            pstmt.setFloat(8, P);
            pstmt.executeUpdate();
            logger.info("Inserted row into solarData: timestamp={}, Gb={}, Gd={}, Gr={}, HSun={}, T={}, WS={}, P_predicted={}", timestamp, Gb, Gd, Gr, H_sun, T2m, WS10m, P);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }

    /**
     * Inserts a record into the realPower table
     */
    public void insertRealPowerData(Connection conn, Timestamp timestamp, float P) {
        String sql = "INSERT INTO realPower (timestamp, P_real) VALUES (?, ?)";
        try (PreparedStatement pstmt = conn.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setFloat(2, P);
            pstmt.executeUpdate();
            logger.info("Inserted row into realPower: timestamp={}, P_real={}", timestamp, P);

        } catch (SQLException e) {
            logger.error("Insert failed: {}", e.getMessage(), e);
        }
    }
}