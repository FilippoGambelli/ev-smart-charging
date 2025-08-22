#!/bin/bash

# Percorso base del progetto
BASE_DIR="$HOME/contiki-ng/examples/ev-smart-charging"

# Porte seriali
PORT_BORDER="/dev/ttyACM0"
PORT_SENSOR="/dev/ttyACM1"
PORT_GRID="/dev/ttyACM2"
PORT_CHARGER="/dev/ttyACM3"

# Apro login in un nuovo terminale con titolo
# gnome-terminal --title="Border Router" \

# Apro login in un nuovo terminale con titolo
gnome-terminal --title="Sensor PV" \
  -- bash -c "cd $BASE_DIR/sensorPV && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_SENSOR; exec bash"

gnome-terminal --title="Smart Grid" \
  -- bash -c "cd $BASE_DIR/smartGrid && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_GRID; exec bash"

gnome-terminal --title="Charging Station" \
  -- bash -c "cd $BASE_DIR/chargingStation && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_CHARGER; exec bash"

gnome-terminal --title="Cloud Application" \
  -- bash -c "cd $BASE_DIR/cloud-application && java -Dcooja=false -jar target/cloud-application-1.0-SNAPSHOT.jar; exec bash"

echo ">>> Tutto pronto! Sono stati aperti due terminali per il login."