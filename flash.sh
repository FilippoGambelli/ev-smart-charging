#!/bin/bash

# Percorso base del progetto
BASE_DIR="$HOME/contiki-ng/examples/ev-smart-charging"

# Porte seriali
PORT_BORDER="/dev/ttyACM0"
PORT_SENSOR="/dev/ttyACM1"
PORT_GRID="/dev/ttyACM2"
PORT_CHARGER="/dev/ttyACM3"

echo ">>> Compilo Border Router..."
cd "$BASE_DIR/borderRouter" || { echo "Errore: cartella non trovata"; exit 1; }
#make TARGET=nrf52840 distclean
make TARGET=nrf52840 BOARD=dongle border-router.dfu-upload PORT=$PORT_BORDER

# Apro login in un nuovo terminale
# gnome-terminal -- bash -c "cd $BASE_DIR/borderRouter && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_BORDER; exec bash"

echo ">>> Compilo Sensor PV..."
cd "$BASE_DIR/sensorPV" || { echo "Errore: cartella non trovata"; exit 1; }
# make TARGET=nrf52840 distclean
make TARGET=nrf52840 BOARD=dongle sensor-PV.dfu-upload PORT=$PORT_SENSOR

# Apro login in un nuovo terminale
# gnome-terminal -- bash -c "cd $BASE_DIR/sensorPV && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_SENSOR; exec bash"

echo ">>> Compilo Smart Grid..."
cd "$BASE_DIR/smartGrid" || { echo "Errore: cartella non trovata"; exit 1; }
#make TARGET=nrf52840 distclean
make TARGET=nrf52840 BOARD=dongle smart-grid.dfu-upload PORT=$PORT_GRID

# Apro login in un nuovo terminale
# gnome-terminal -- bash -c "cd $BASE_DIR/sensorPV && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_SENSOR; exec bash"

echo ">>> Compilo Charging Station..."
cd "$BASE_DIR/chargingStation" || { echo "Errore: cartella non trovata"; exit 1; }
#make TARGET=nrf52840 distclean
make TARGET=nrf52840 BOARD=dongle charging-station.dfu-upload PORT=$PORT_CHARGER

# Apro login in un nuovo terminale
# gnome-terminal -- bash -c "cd $BASE_DIR/sensorPV && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_SENSOR; exec bash"

echo ">>> Tutto pronto! Sono stati aperti due terminali per il login."