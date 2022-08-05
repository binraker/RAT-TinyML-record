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
#include <RTCCounter.h>
#include <time.h>


#define DSP_CONFIG_TANKADDR 0x4000c0b0
#define DSP_CONFIG_TANK 0x4000c0a8
#define softMaxAddressStart 0x1fffd41c
uint32_t tankAddress;                                       // tankAddress extracted from NDP
uint32_t tankSize;                                          // tank size extracted from NDP
int currentClassifierCount0 = 0;                            // individual counter for each classifier
int currentClassifierCount1 = 0;
int match = 0;                                                // This variable indicates which class has matched

void setup(void) {
  SAMD21_init(0);                                          // Setting up SAMD21 
  NDP_init("sensor_data_collection_model.bin",1);                                             // Setting up NDP, filename and Transducer type microphone=0, Sensor=1
  tankAddress = indirectRead(DSP_CONFIG_TANKADDR);        // The value stored in the DSP_CONFIG_TANKADDR has the starting address
  tankSize = indirectRead(DSP_CONFIG_TANK) >> 4;          // The tankSize is stored in the location DSP_CONFIG_TANK
  Serial.print(" Tank address and size   ");
  Serial.print(tankAddress, HEX);
  Serial.print(" ,");
  Serial.println(tankSize);
  rtcCounter.begin();
}
int filecounter = 0;

void printDateTime()
{
  // Get time as an epoch value and convert to tm struct
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);

  // Format and print the output
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%02d %02d:%02d:%02d", t->tm_year - 100, 
      t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

  Serial.println(buffer);
}

void setDateTime(uint16_t year, uint8_t month, uint8_t day, 
  uint8_t hour, uint8_t minute, uint8_t second)
{
  // Use the tm struct to convert the parameters to and from an epoch value
  struct tm tm;

  tm.tm_isdst = -1;
  tm.tm_yday = 0;
  tm.tm_wday = 0;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  rtcCounter.setEpoch(mktime(&tm));
}


void loop() {
  digitalRead(SD_CARD_SENSE) ? Serial.println("card in") : Serial.println("card out");
  printDateTime();
  if ( (filecounter % 2) == 0){
    digitalWrite(LED_BLUE,LOW);
    digitalWrite(LED_GREEN,HIGH);
  }
  else{
    digitalWrite(LED_BLUE,HIGH);
    digitalWrite(LED_GREEN,LOW);
  }
  //delay(23); //24ms is between samples, allowing 1ms for rest of the operations
  saveLongSensorData(6, 10, filecounter, tankAddress, tankSize);
  // Serial2.print("l");
  // delay(10);
  // Serial.println(Serial2.available());
  // Serial.println(Serial2.read());
  //Serial2.print("loop\n");
  if (Serial2.available())
  {
    while (Serial2.available() && Serial2.peek() != '$')
    {
      //Serial.print((char)Serial2.read());
      Serial2.read();
    }
    if (Serial2.available() >= 22)
    {
      String timeSt = Serial2.readString();
      Serial.print("Time recieved: ");
      Serial.println(timeSt);
      setDateTime(timeSt.substring(2,6).toInt(), timeSt.substring(7,9).toInt(), timeSt.substring(10,12).toInt(), timeSt.substring(13,15).toInt(), timeSt.substring(16,18).toInt(), timeSt.substring(19,21).toInt());
    }
  }
  Serial2.print("file written no ");
  Serial2.print(filecounter);
  Serial2.print("\n");
  
  filecounter++;
}
