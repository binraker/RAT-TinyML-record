#!/bin/bash

# Function to detect SD Card mount point
detect_sd_card() {
  # This is a basic way to detect an SD card and might need adjustment
  # based on your specific hardware configuration
  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Typical mount points in Linux
    for mount in /media/$USER/* /run/media/$USER/*; do
      if df -h | grep -q $mount; then
        echo $mount
        return
      fi
    done
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Typical mount point in macOS
    for mount in /Volumes/*; do
      if df -h | grep -q $mount; then
        echo $mount
        return
      fi
    done
  elif [[ "$OSTYPE" == "msys"* ]]; then
    # Windows
    # This will need to be adjusted based on how your system mounts SD cards
    for drive in /d/* /e/* /f/* /g/* /h/*; do
      if df -h | grep -q $drive; then
        echo $drive
        return
      fi
    done
  else
    echo "SD Card not found"
    return 1
  fi
}

# Function to detect ESP32/TinyML board port
detect_board_port() {
  # Check if the operating system is macOS
  if [[ "$OSTYPE" == "darwin"* ]]; then
    # Using lsusb on macOS to find a device with "USB Serial"
    lsusb_output=$(lsusb)
    if echo "$lsusb_output" | grep -q "USB Serial"; then
      # Extracting the device identifier
      usb_device=$(echo "$lsusb_output" | grep "USB Serial" | awk '{print $2 ":" $4}' | sed 's/://g')
      echo "/dev/tty.$usb_device"
      return
    else
      echo "No USB Serial devices found."
      return 1
    fi
  else
    echo "Unsupported operating system for this detection method"
    return 1
  fi
}

# Detect Operating System
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  echo "Running Linux specific commands"
  # Add Linux commands here
elif [[ "$OSTYPE" == "darwin"* ]]; then
  echo "Running macOS specific commands"
  # Add macOS commands here
elif [[ "$OSTYPE" == "msys"* ]]; then
  echo "Running Windows specific commands"
  # Add Windows commands here
else
  echo "Unsupported operating system"
  exit 1
fi

# Install Dependencies
echo "Installing dependencies"
# Add dependency installation commands here

# Clone Repositories
echo "Cloning repositories"
##git clone https://github.com/binraker/RAT-ESP32.git
##git clone https://github.com/binraker/RAT-TinyML-record.git

# Detect and Set Board Port
board_port=$(detect_board_port)
if [ -z "$board_port" ]; then
  echo "Board port not detected"
  exit 1
fi

# Compile and Flash Firmware for Syntiant TinyML
echo "Compiling and flashing firmware for Syntiant TinyML"
# Replace [board_name] with the actual Fully Qualified Board Name (FQBN)
arduino-cli compile --fqbn [board_name] RAT-TinyML-record
arduino-cli upload --port $board_port --fqbn [board_name] RAT-TinyML-record

# Compile and Flash Firmware for ESP32
echo "Compiling and flashing firmware for ESP32"
# Setup ESP-IDF environment
source $HOME/esp/esp-idf/export.sh
idf.py -p $board_port flash

# Detect and Set SD Card Path
sd_card_path=$(detect_sd_card)
if [ -z "$sd_card_path" ]; then
  echo "SD Card path not detected"
  exit 1
fi

# Copy sensor_data_collection_model.bin to SD card
echo "Copying sensor_data_collection_model.bin to SD card"
cp RAT-TinyML-record/sensor_data_collection_model.bin "$sd_card_path"

echo "Finishing setup"
