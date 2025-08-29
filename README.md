# EV Smart Charging

This project implements an **IoT-based Smart Charging System for Electric Vehicles (EVs)**, integrating renewable energy sources (solar power), real-time data acquisition, predictive machine learning models, and cloud-based management tools.

The goal is to optimize EV charging by prioritizing renewable energy usage, reducing reliance on the traditional power grid, and supporting sustainable mobility.

---

## Features

* **IoT Architecture**

  * **Sensor PV node**: collects solar irradiance, temperature, wind speed, and computes short-term PV energy forecasts.
  * **Charging Station node**: manages EV connections, charging process, and vehicle priorities.
  * **Smart Grid node**: models grid interaction, importing or exporting energy as needed.
  * **Central Node (Border Router)**: manages system logic, charging strategies, and power distribution.

* **Machine Learning Integration**

  * Trained on **PVGIS datasets** (European Commission service).
  * Feed-forward neural network forecasting PV output up to 2 hours ahead.
  * Converted into lightweight C code (via **emlearn**) to run on IoT devices.

* **Cloud Application**

  * Implemented in **Java** with a **MySQL database**.
  * Stores sensor data.
  * Manages vehicle priorities and ML model execution.
  * Provides CLI interface for monitoring and control.

* **Visualization**

  * Preconfigured **Grafana dashboard** (`Grafana.json`) for monitoring solar data.

---

## Repository Structure

```
.
├── borderRouter/        # Code for the central node / border router
├── sensorPV/            # Code for photovoltaic sensor nodes
├── smartGrid/           # Code for smart grid node
├── chargingStation/     # Code for EV charging station nodes
├── ML/                  # Python scripts & Jupyter notebooks for ML model training & conversion
├── cloud-application/   # Java cloud application with CLI & CoAP interfaces
├── evSmartCharging.sql  # MySQL database schema & initial data
├── Grafana.json         # Grafana dashboard configuration
├── flash.sh             # Bash script to flash code to devices
├── simulation.csc       # Cooja simulation setup
└── LICENSE              # Apache 2.0 License
```

---

## Cloud Application

The **cloud-application** folder contains a **Java-based backend** with the following functionalities:

* **Data acquisition & storage** from Sensor PV and Charging Stations (via CoAP).
* **Vehicle priority management** (using license plates).
* **Automatic ML model control** (start/stop model based on solar production).
* **Command-Line Interface (CLI)** for:

  * Database management
  * ML model execution & interval configuration
  * Monitoring charging stations and smart grid status
  * Viewing historical sensor and power data
  * Safe system shutdown

The **database schema** is defined in `evSmartCharging.sql`.

---

## Visualization

Import the provided **Grafana dashboard** (`Grafana.json`) to visualize:

* Solar irradiance, temperature, and wind data
* Real-time photovoltaic power output

---

## How to Run

### 1. Simulation Mode

You can run the system in a simulated environment using **Cooja**:

```bash
# Open Cooja and import the provided simulation
simulation.csc
```


### 2. Deployment on Real Hardware

The system can be deployed on **nRF52840 Dongles** or other supported IoT devices using the provided automation script.
The script handles compilation, flashing, and opening terminal sessions for logs.

You can run it with the following options:

```bash
# Perform a full clean build and flash all devices
./script.sh clean

# Flash all devices without cleaning
./script.sh flash

# Only open terminal windows for each device (no flashing)
./script.sh login
```

---

## License

This project is released under the **[Apache 2.0 License](./LICENSE)**.