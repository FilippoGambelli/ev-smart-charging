package it.evsmartcharging;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

    public void insertSolarData(Timestamp timestamp, float Gb, float Gd, float Gr, float H_sun, float T2m, float WS10m, float P) {
        String sql = "INSERT INTO solarData (timestamp, Gb, Gd, Gr, HSun, T, WS, P_predicted) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
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

    public void insertRealPowerData(Timestamp timestamp, float P) {
        String sql = "INSERT INTO realPower (timestamp, P_real) VALUES (?, ?)";
        try (PreparedStatement pstmt = connection.prepareStatement(sql)) {
            pstmt.setTimestamp(1, timestamp);
            pstmt.setFloat(2, P);
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
                return rs.getInt("priority"); // ritorna 0 o 1
            } else {
                return -1; // default se la targa non è presente
            }
        } catch (SQLException e) {
            e.printStackTrace();
            return -1; // fallback in caso di errore
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