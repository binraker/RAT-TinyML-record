#!/bin/bash

# Function to detect SD Card mount point
detect_sd_card() {
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
# Install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
# Install ESP-IDF
#if ubuntu or osx
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

if [[ "$OSTYPE" == "darwin"* ]]; then
  brew install cmake ninja dfu-util
  pip3 install --user -r https://raw.githubusercontent.com/espressif/esp-idf/master/requirements.txt

# Install ESP-IDF if ubuntu or osx
if [[ "$OSTYPE" == "linux-gnu"* ]] || [[ "$OSTYPE" == "darwin"* ]]; then
  mkdir -p ~/esp
  cd ~/esp
  git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git
  cd ~/esp/esp-idf
  ./install.sh esp32
  . $HOME/esp/esp-idf/export.sh
  alias get_idf='. $HOME/esp/esp-idf/export.sh'
  cd ~/esp
  echo "export IDF_PATH=~/esp/esp-idf" >> ~/.bashrc
  cp -r $IDF_PATH/examples/get-started/hello_world .
  cd ~/esp/hello_world
  idf.py set-target esp32
  

# Clone Repositories
echo "Cloning repositories"
git clone https://github.com/binraker/RAT-ESP32.git
git clone https://github.com/binraker/RAT-TinyML-record.git

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

# Configure ESP32 wifi credentials
echo "User needs to manually configure the ESP32 wifi credentials"
idf.py menuconfig

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
