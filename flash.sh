#!/bin/bash
#
# This script automates flashing and login for Contiki-NG EV smart charging.
#
# Usage:
#   ./script.sh clean    -> Perform one flash with male clean
#   ./script.sh flash    -> Perform a single flash (without clean)
#   ./script.sh login    -> Only open login terminals, no flashing
#

# Base project path
BASE_DIR="$HOME/contiki-ng/examples/ev-smart-charging"

# Serial ports
PORT_BORDER="/dev/ttyACM0"
PORT_SENSOR="/dev/ttyACM1"
PORT_GRID="/dev/ttyACM2"
PORT_CHARGER3="/dev/ttyACM3"
PORT_CHARGER4="/dev/ttyACM4"

### FUNCTIONS ###

function build_and_flash() {
    local dir=$1
    local target=$2
    local port=$3
    local do_clean=$4

    echo ">>> Building $dir..."
    cd "$BASE_DIR/$dir" || { echo "Error: folder $dir not found"; exit 1; }

    if [ "$do_clean" = true ]; then
        echo ">>> Running make distclean..."
        make TARGET=nrf52840 distclean
    fi

    while true; do
        make TARGET=nrf52840 BOARD=dongle "$target".dfu-upload PORT=$port

        echo ""
        read -p "Do you want to repeat the flash? (y/n): " choice
        case "$choice" in
            [Yy]* ) echo ">>> Repeating flash...";;
            [Nn]* ) echo ">>> Exiting."; break;;
            * ) echo "Invalid input. Please enter 'y' or 'n'.";;
        esac
    done
}


function flash_all() {
    local do_clean=$1

    build_and_flash "borderRouter" "border-router" "$PORT_BORDER" $do_clean

    build_and_flash "sensorPV" "sensor-PV" "$PORT_SENSOR" $do_clean

    build_and_flash "smartGrid" "smart-grid" "$PORT_GRID" $do_clean

    build_and_flash "chargingStation" "charging-station" "$PORT_CHARGER3" $do_clean

    build_and_flash "chargingStation" "charging-station" "$PORT_CHARGER4" $do_clean
}


function open_logins() {
    read -p "Open Border Router / Central Node? [Press Enter to continue]" _
    gnome-terminal --title="Border Router / Central Node" \
    -- bash -c "cd $BASE_DIR/borderRouter && make TARGET=nrf52840 BOARD=dongle PORT=$PORT_BORDER connect-router; exec bash"

    read -p "Open Cloud Application? [Press Enter to continue]" _
    gnome-terminal --title="Cloud Application" \
    -- bash -c "cd $BASE_DIR/cloud-application && java -Dcooja=false -jar target/cloud-application-1.0-SNAPSHOT.jar; exec bash"

    read -p "Open Sensor PV? [Press Enter to continue]" _
    gnome-terminal --title="Sensor PV" \
    -- bash -c "cd $BASE_DIR/sensorPV && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_SENSOR; exec bash"

    read -p "Open Smart Grid? [Press Enter to continue]" _
    gnome-terminal --title="Smart Grid" \
    -- bash -c "cd $BASE_DIR/smartGrid && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_GRID; exec bash"

    read -p "Open Charging Station 1? [Press Enter to continue]" _
    gnome-terminal --title="Charging Station 1" \
    -- bash -c "cd $BASE_DIR/chargingStation && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_CHARGER3; exec bash"

    read -p "Open Charging Station 2? [Press Enter to continue]" _
    gnome-terminal --title="Charging Station 2" \
    -- bash -c "cd $BASE_DIR/chargingStation && make login TARGET=nrf52840 BOARD=dongle PORT=$PORT_CHARGER4; exec bash"

    echo ">>> All terminals opened, system is running."
}


### MAIN LOGIC ###

case "$1" in
    clean)
        echo ">>> First flash (with clean)..."
        flash_all true
        ;;

    flash)
        echo ">>> Flashing all devices (no clean)..."
        flash_all false
        ;;

    login)
        echo ">>> Opening login terminals only..."
        open_logins
        ;;

    *)
        echo "Usage: $0 {all|flash|login}"
        exit 1
        ;;
esac