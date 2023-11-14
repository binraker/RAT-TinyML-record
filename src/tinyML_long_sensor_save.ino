/*
 * Copyright (c) 2021 Syntiant Corp.  All rights reserved.
 * Contact at http://www.syntiant.com
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file LICENSE in the main
 * directory of this source tree, or the OpenIB.org BSD license below.  Any
 * code involving Linux software will require selection of the GNU General
 * Public License (GPL) Version 2.
 *
 * OPENIB.ORG BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
  Sensor data IMU files are named like this:
  sensorData_1_2023_11_10_16_56_00
  that is: SensorData, file count(epoch), Year, Month, Date, Hr, Min, Seconds
*/

#include <NDP.h>
#include <NDP_utils.h>
#include <Arduino.h>
#include "TinyML_init.h"
#include "NDP_loadModel.h"
#include "microSDUtils.h"
#include "SAMD21_init.h"
#include "NDP_init.h"
#include "SAMD21_lowpower.h"
#include "PMIC_init.h"
#include <time.h>
#include <RTCZero.h>
#include "config.h"

RTCZero rtc; // create an rtc object

#define DSP_CONFIG_TANKADDR 0x4000c0b0
#define DSP_CONFIG_TANK 0x4000c0a8
#define softMaxAddressStart 0x1fffd41c
uint32_t tankAddress;                                       // tankAddress extracted from NDP
uint32_t tankSize;                                          // tank size extracted from NDP
int currentClassifierCount0 = 0;                            // individual counter for each classifier
int currentClassifierCount1 = 0;
int match = 0;                                                // This variable indicates which class has matched

String manual_set_start_time = "2023:01:01:00:00:00"; // variable to store time when the ESP32 CAM time message is not okay
//                              YY:Month:Date:Hr:Min:Seconds

String received_timestamp_from_ESP32CAM = ""; // stores the message received via Serial2
String current_time = ""; // variable to store time value received from the ESP32 CAM (e.g "[Browser]: 2023:11:09:02:10:48")
int current_year, current_month, current_day, current_hour, current_minute, current_second; // variables to store time values
void set_time_values(String correct_current_time); // function to parse current_time[19] and extract the individual time data

String current_file_save_timestamp = ""; // stores the time when IMU data collection has started
int latest_saved_SensorData_file_number = 0; //variable to store the latest saved file count (epoch)
bool successfull_read_latest_saved_SensorData_file_number = false; // controls whether the IMU data collection can be executed

void setup(void) {
  SAMD21_init(0); // Setting up the SAMD21
  
  turnON_GreenLED();

  NDP_init("sensor_data_collection_model.bin",1);     // Setting up NDP, filename and Transducer type microphone=0, Sensor=1
  tankAddress = indirectRead(DSP_CONFIG_TANKADDR);    // The value stored in the DSP_CONFIG_TANKADDR has the starting address
  tankSize = indirectRead(DSP_CONFIG_TANK) >> 4;       // The tankSize is stored in the location DSP_CONFIG_TANK
  Serial.print("Tank address and size : ");
  Serial.print(tankAddress, HEX);
  Serial.print(" , ");
  Serial.println(tankSize);

  /* ***  First thing, set the time *** */
  while(!Serial2.available()){
    // wait until the ESP32 CAM time data is available
  }

  if (Serial2.available())
  {
    received_timestamp_from_ESP32CAM = Serial2.readString();
  }
  //String received_timestamp_from_ESP32CAM = Serial2.readStringUntil('\n');
  Serial.print("ESP32 CAM message = ");
  Serial.println(received_timestamp_from_ESP32CAM);

  // Check if the message starts with "[NTP]:" or "[Browser]:"
  // e.g [NTP]:2023:11:09:14:19:32
  // e.g [Browser]:2023:11:09:14:19:32

  char expectedNTPPrefix[] = "[NTP]:";
  char expectedBrowserPrefix[] = "[Browser]:";
  if (received_timestamp_from_ESP32CAM.startsWith(expectedNTPPrefix)) {
    //Serial.println("ESP32CAM message starts with [NTP]: ");
    // Remove the first 6 characters
    current_time = received_timestamp_from_ESP32CAM.substring(6);
    // parse the time string and extract the seperate times
    set_time_values(current_time);
  } 
  else if (received_timestamp_from_ESP32CAM.startsWith(expectedBrowserPrefix)) {
    //Serial.println("ESP32CAM message starts with [Browser]:");
    // Remove the first 11 characters
    current_time = received_timestamp_from_ESP32CAM.substring(10);
    // parse the time string and extract the seperate times
    set_time_values(current_time);
  } 
  else {
    //Serial.println("ESP32CAM message does not start with '[NTP]: ' or '[Browser]:' ");
    
    // start timer using the manually defined time
    // in future we can fetch this time from an SD card's file name/content
    current_time = manual_set_start_time;
    set_time_values(current_time);
  }

  Serial.print("current_time = ");
  Serial.println(current_time);
  //prints something like : current_time = 2023:11:09:02:10:48

  rtc.begin(); // initialize RTC

  // Set the time
  rtc.setHours(current_hour);
  rtc.setMinutes(current_minute);
  rtc.setSeconds(current_second);

  // Set the date
  rtc.setDay(current_day);
  rtc.setMonth(current_month);
  int int_two_digit_year = String(current_year).substring(2, 4).toInt();
  rtc.setYear(int_two_digit_year);
  /* *** */
  
  Serial.println("- - - - - - - - - - - - - - - -");
}

void loop() {
 
  digitalRead(SD_CARD_SENSE) ? Serial.println("SD card in") : Serial.println("SD card out!");

  print_RTC();

  // save IMU data
  save_IMU_data();

  delay(1500);
}

/* *** User functions *** */

void set_time_values(String correct_current_time){
  
  String current_time_to_parse = correct_current_time;
  // e.g 2023:11:09:12:06:13
  current_year = current_time_to_parse.substring(0, 4).toInt();
  current_month = current_time_to_parse.substring(5, 7).toInt();
  current_day = current_time_to_parse.substring(8, 10).toInt();
  current_hour = current_time_to_parse.substring(11, 13).toInt();
  current_minute = current_time_to_parse.substring(14, 16).toInt();
  current_second = current_time_to_parse.substring(17, 19).toInt();
  
  Serial.println("Extracted time values.");
  Serial.print("Year: ");
  Serial.println(current_year);
  Serial.print("Month: ");
  Serial.println(current_month);
  Serial.print("Day: ");
  Serial.println(current_day);
  Serial.print("Hour: ");
  Serial.println(current_hour);
  Serial.print("Minute: ");
  Serial.println(current_minute);
  Serial.print("Second: ");
  Serial.println(current_second);
  Serial.println();
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}

void print_time() {
  Serial.print("Time: ");

  // ...and time
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
}

void print_date() {
  Serial.print("\tDate: ");

  // Print date...
  print2digits(rtc.getDay());
  Serial.print("/");
  print2digits(rtc.getMonth());
  Serial.print("/");
  print2digits(rtc.getYear());
  Serial.print(" ");

  Serial.println();
}

void print_RTC() {
  print_time();
  print_date();
}

String get_current_time_as_string(){
  String rtc_current_time = String(current_year) + "_" + String(rtc.getMonth()) + "_" + String(rtc.getDay()) + "_";
  rtc_current_time += String(rtc.getHours()) + "_" + String(rtc.getMinutes()) + "_" + String(rtc.getSeconds());

  // returns, for example, 2023_11_10_18_9_16
  return rtc_current_time;
}

int get_latest_saved_SensorData_file_number(){

  if (SD.begin(SDCARD_SS_PIN, SPI_HALF_SPEED)) {   

    // open the file for reading:
    File latest_saved_SensorData_file = SD.open(LATEST_SAVED_SENSORDATA_FILE_NUMBER_PATH, FILE_READ);
    if (latest_saved_SensorData_file) {

      String file_content = "";
      Serial.print("Latest saved SensorData file number : ");

      // read from the file until there's nothing else in it:
      while (latest_saved_SensorData_file.available()) {
        char currentChar = latest_saved_SensorData_file.read(); 

        // Break if a fullstop is encountered
        if (currentChar == '.') {
          break;
        }

        file_content += currentChar;
      }
      latest_saved_SensorData_file.close(); // close the file

      int int_latest_saved_fileNumber = file_content.toInt();
      Serial.println(int_latest_saved_fileNumber);
        
      successfull_read_latest_saved_SensorData_file_number = true;
      return int_latest_saved_fileNumber;
    } 
    else {
      // if the file didn't open
      successfull_read_latest_saved_SensorData_file_number = false;
      Serial.print("Error opening ");
      Serial.println(LATEST_SAVED_SENSORDATA_FILE_NUMBER_PATH);
    }
  }
}

void save_IMU_data(){
  turnON_BlueLED(); // turn on Blue LED to indicate data collection
  current_file_save_timestamp = get_current_time_as_string();
  //Serial.print("Current file's save timestamp : ");
  //Serial.println(current_file_save_timestamp);

  // get the latest saved SensorData file number on the SD card
  latest_saved_SensorData_file_number = get_latest_saved_SensorData_file_number();
  
  //delay(23); //24ms is between samples, allowing 1ms for rest of the operations
  if (successfull_read_latest_saved_SensorData_file_number){
    // send a message to the ESP32CAM on Serial2 to record a video, give the file name
    // CAUTION! Maintain same format between Data, i.e, no space after ':'
    String send_to_ESP32CAM_Data_FileName = "[Data]:";
    send_to_ESP32CAM_Data_FileName += "_" + String(latest_saved_SensorData_file_number + 1) + "_" + current_file_save_timestamp;
    Serial2.println(send_to_ESP32CAM_Data_FileName);
    // sends something like : [Data]:_15_2023_11_12_14_24_11

    // print what has been sent via Serial
    Serial.print("Message sent to ESP32CAM : ");
    Serial.println(send_to_ESP32CAM_Data_FileName);

    saveLongSensorData(6, 10, latest_saved_SensorData_file_number + 1, current_file_save_timestamp, tankAddress, tankSize);
  }
  Serial.println("- - - - - - - - - - - - - - - -");
  successfull_read_latest_saved_SensorData_file_number = false; // reset variable
  turnON_GreenLED();
}

/* *** Onboard RGB LED control functions *** */
void turnON_BlueLED(){
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
}
void turnON_GreenLED(){
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
}
