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

#include "NDP_loadModel.h"
#include "microSDUtils.h"
#include "../../src/config.h"

#define strideSamples 384                                   // 24ms stride * 16K/s 

uint32_t startingFWAddress;                                 // Address for the tank pointer is extracted from NDP
uint32_t tankRead;                                          // Local variable to hold value of value read from NDP
uint32_t currentPointer;                                    // Current pointer address
char dataFileName[100];                                     // String for file to be saved 
String dataFileNumber;                                      // For automatic name generation
File myFile;                                                // file for saving data

void saveAudioInferences(uint8_t numberOfClassifiers, uint32_t sofMaxStartingAddress){    // routine for saving softMax data before posterior filter
   if (SD.begin(SDCARD_SS_PIN)) {                           // Check if SD card inserted
      myFile = SD.open("audioInferenceData.csv", FILE_WRITE);
      myFile.println("");
      for (int i=0; i<numberOfClassifiers; i++){
         myFile.print(indirectRead(sofMaxStartingAddress+i*4));
         myFile.print(",");
      }
      startingFWAddress = indirectRead(0x1fffc0c0);
      myFile.print(indirectRead(startingFWAddress));
      myFile.print(",");
      myFile.close();
   }
}

void savePosteriorFilterDecision(int classifier, int currentFile){    // routine for saving softMax data before posterior filter
   if (SD.begin(SDCARD_SS_PIN)) {                           // Check if SD card inserted
      myFile = SD.open("audioInferenceData.csv", FILE_WRITE);
      myFile.print(classifier);
      myFile.print(",");
      myFile.print(currentFile);
      myFile.print(",");
      myFile.close();
   }
}

void saveLongAudioData(int dataSavePeriodInSeconds, int currentFile, uint32_t tankAddress, uint32_t tankSize){    // routine for saving data 
   if (SD.begin(SDCARD_SS_PIN)) {                           // Check if SD card inserted
      myFile = SD.open(dataFileName, FILE_WRITE);
      myFile.remove(); // delete old file                   // Deleteting the file if already existed to avoid appending it
      myFile = SD.open(dataFileName, FILE_WRITE);
      myFile.rewind(); // go to start of file
      Serial.print("SD file opened ");
      Serial.println(dataFileName);
   }
   else{
      Serial.println("SD file not opened ");
   }
   int samplesSaved =dataSavePeriodInSeconds* 16000;                     // Sampling rate is fixed at 16K
   Serial.println(" ");   
   startingFWAddress = indirectRead(0x1fffc0c0);
   Serial.print(" Starting FW Address  ");
   Serial.println(startingFWAddress,HEX);                  // This is pointer to Tank pointer
   Serial.print("Samples to be saved ");                    // length of the data to be saved
   Serial.println(samplesSaved);
   currentPointer = indirectRead(startingFWAddress);        // location of current pointer in the circular buffer
   Serial.print("Current Pointer before stridePaddingAfter delay "); //pointer before waiting
   Serial.println(currentPointer);
   int n = sprintf(dataFileName, "audioData_%d.csv",currentFile);
   int16_t sampleValue;
   int32_t startingPoint = (currentPointer - 1600) ;// Saving data from before, *2 because 1 sample = 2 bytes
   if (startingPoint < 0){
    startingPoint = (currentPointer - 1600) + tankSize; //Due to circular buffer nature, the tarting point could be negative and should be adjusted
   }  
   Serial.print("Starting point (adjusting for curcular buffer if needed) ");
   Serial.println(startingPoint);
   for (int i = 0; i < (samplesSaved*2); i += 4){             // *2 becuase there are two bytes, 4 is because the read is done for 4 bytes at a time
      tankRead = indirectRead(tankAddress + (( startingPoint + i) % tankSize));
      sampleValue = tankRead & 0xffff;                        // Saving lower two bytes
      myFile.println(sampleValue);
      sampleValue = (tankRead >> 16) & 0xffff;                // Saving upper two bytes
      myFile.println(sampleValue);
      if ( (i % 1600) == 0){
         currentPointer = indirectRead(startingFWAddress); 
         //Serial.print(currentPointer);
         //Serial.print(" ");
         //Serial.print( ( startingPoint + i) % tankSize );      
         int diff =  currentPointer -  (( startingPoint + i) % tankSize);
         if (diff < 0)
            diff = diff + tankSize;
         //Serial.print(" ");
         //Serial.println( diff );
         if (diff<160){
            Serial.println(" Warning only 0.01 Second FIFO left");
         }
         if (diff<1600){
            delay(100);
         //Serial.println("waited");
         }
         if (diff> (tankSize-1600) ){
            Serial.println(" Warning terminating circular FIFO to be overwritten, margin left 0.1S");
            break;
         }
         if ( (i % 32000) == 0) {

            Serial.print("Second starting ");
            Serial.println( i / 32000);
         }
      }
    }
    myFile.close();
    Serial.println("SD file Closed ");
    currentPointer = indirectRead(startingFWAddress);         // estimating delay during the data saving process, pointer difference / 2 * 24 = elapsed time in ms
    Serial.print("Current Pointer after savind data ");
    Serial.println(currentPointer);
}

void saveAudioData(uint8_t classifier, int extrStides, int currentFile, uint32_t tankAddress, uint32_t tankSize){    // routine for saving data when invoked
   int32_t samplesSaved = strideSamples*(extrStides + 39) + 512;  // 15488 samples are required for one tensor or 0.968ms
   Serial.println(" ");   
   startingFWAddress = indirectRead(0x1fffc0c0);
   Serial.print(" Starting FW Address  ");
   Serial.println(startingFWAddress,HEX);                  // This is pointer to Tank pointer
   Serial.print("Samples to be saved ");                    // length of the data to be saved
   Serial.println(samplesSaved);
   currentPointer = indirectRead(startingFWAddress);        // location of current pointer in the circular buffer
   Serial.print("Current Pointer before stridePaddingAfter delay "); //pointer before waiting
   Serial.println(currentPointer);
   int n = sprintf(dataFileName, "data_classifier%d_%d.csv",classifier,currentFile);
   int16_t sampleValue;
   if (SD.begin(SDCARD_SS_PIN)) {                           // Check if SD card inserted
      myFile = SD.open(dataFileName, FILE_WRITE);
      myFile.remove(); // delete old file                   // Deleteting the file if already existed to avoid appending it
      myFile = SD.open(dataFileName, FILE_WRITE);
      myFile.rewind(); // go to start of file
      Serial.print("SD file opened ");
      Serial.println(dataFileName);
   }
   else{
      Serial.println("SD file not opened ");
   }
   int32_t startingPoint = (currentPointer - samplesSaved*2) ;// Saving data from before, *2 because 1 sample = 2 bytes
   if (startingPoint < 0){
    startingPoint = (currentPointer - samplesSaved*2) + tankSize; //Due to circular buffer nature, the tarting point could be negative and should be adjusted
   }
   Serial.print("Starting point (adjusting for curcular buffer if needed) ");
   Serial.println(startingPoint);
   for (int i = 0; i < (samplesSaved*2); i += 4){             // *2 becuase there are two bytes, 4 is because the read is done for 4 bytes at a time
      tankRead = indirectRead(tankAddress + (( startingPoint + i) % tankSize));
      sampleValue = tankRead & 0xffff;                        // Saving lower two bytes
      myFile.println(sampleValue);
      sampleValue = (tankRead >> 16) & 0xffff;                // Saving upper two bytes
      myFile.println(sampleValue);
    }
    myFile.close();
    Serial.println("SD file Closed ");
    currentPointer = indirectRead(startingFWAddress);         // estimating delay during the data saving process, pointer difference / 2 * 24 = elapsed time in ms
    Serial.print("Current Pointer after savind data ");
    Serial.println(currentPointer);
}

/*
  Sensor data IMU files are named like this:
  sensorData_1_2023_11_10_16_56_00
  SensorData, file count(epoch), Year, Month, Date, Hr, Min, Seconds
*/
void saveLongSensorData(int sensorAxes, int dataSavePeriodInSeconds, int currentFile, String currentFile_timestamp, uint32_t tankAddress, uint32_t tankSize){    // routine for saving data 
   
   // create the filename using the file count and timestamp
   String dataFileName = "SensorData_" + String(currentFile) + "_" + currentFile_timestamp + ".csv";

   if (SD.begin(SDCARD_SS_PIN, SPI_HALF_SPEED)) {                           // Check if SD card inserted
      myFile = SD.open(dataFileName, FILE_WRITE);
      myFile.rewind(); // go to start of file

      // write the first row which has the CSV heading
      myFile.print(SENSORDATA_CSVFILE_FIRST_ROW);
      myFile.println("");

      Serial.print("SD file opened : ");
      Serial.println(dataFileName);
   }
   else{
      Serial.println("SD file not opened!");
   }
   int samplesSaved = dataSavePeriodInSeconds* 100 * sensorAxes;  // Sampling rate is fixed at 100Hz
   int samplesFIFOMargin = 10 * sensorAxes; // Margin before wait or termination is executed                  
 
   startingFWAddress = indirectRead(0x1fffc0c0);
   Serial.print(" Starting FW Address  ");
   Serial.println(startingFWAddress,HEX);                  // This is pointer to Tank pointer
   Serial.print("Samples to be saved ");                    // length of the data to be saved
   Serial.println(samplesSaved);
   currentPointer = indirectRead(startingFWAddress);        // location of current pointer in the circular buffer
   Serial.print("Current Pointer before stridePaddingAfter delay "); //pointer before waiting
   Serial.println(currentPointer);
  
   int16_t sampleValue;
   int32_t startingPoint = (currentPointer - 10) ;// Saving data from before, *2 because 1 sample = 2 bytes
   if (startingPoint < 0){
    startingPoint = (currentPointer - 10) + tankSize; //Due to circular buffer nature, the starting point could be negative and should be adjusted
   }  
   Serial.print("Starting point (adjusting for circular buffer if needed) ");
   Serial.println(startingPoint);

   uint32_t startTime = millis(); // Timestamp for the start of data collection
   // each row is started after 3 consecutive for loops rounds. When a new row has started we first save the sampling milliseconds
   int round_count = 0; // store timestamp values after 3 for loops

   int axesFormatingCounter = 0;

   for (int i = 0; i < (samplesSaved*2); i += 4){             // *2 because there are two bytes, 4 is because the read is done for 4 bytes at a time
      //Serial.print("i = ");
      //Serial.println(i);
      
      if (!digitalRead(SD_CARD_SENSE))
      {
         Serial.println("No SD card found!");
         //delay(100);
         return;
      }

      // new rows start after saving lower 2 bytes & saving upper 2 bytes, three times
      if(round_count == 0 ){
         //Serial.println("[Starting a new CSV row..]");
         // Add the timestamp of the sensor collection to the CSV file
         uint32_t currentTime = millis() - startTime;
         myFile.print(currentTime); // write the current time in the CSV row, first column
         myFile.print(",");
      }

      tankRead = indirectRead(tankAddress + (( startingPoint + i) % tankSize));
      sampleValue = tankRead & 0xffff;                        // Saving lower two bytes
      myFile.print(sampleValue);
      
      //Serial.println("[SAVING lower 2 bytes]:");
      //Serial.println(sampleValue);

      if (axesFormatingCounter==(sensorAxes-1)){
         axesFormatingCounter = 0;
         myFile.println("");
      }
      else{
         myFile.print(",");
         axesFormatingCounter++;
      }
      sampleValue = (tankRead >> 16) & 0xffff;                // Saving upper two bytes
      myFile.print(sampleValue);

      //Serial.println("[SAVING upper 2 bytes]:");
      //Serial.println(sampleValue);

      if (axesFormatingCounter==(sensorAxes-1)){
         axesFormatingCounter = 0;
         myFile.println("");
      }
      else{
         myFile.print(",");
         axesFormatingCounter++;
      }
      if ( (i % samplesFIFOMargin) == 0){
         currentPointer = indirectRead(startingFWAddress); 
         //Serial.print(currentPointer);
         //Serial.print(" ");
         //Serial.print( ( startingPoint + i) % tankSize );      
         int diff =  currentPointer -  (( startingPoint + i) % tankSize);
         if (diff < 0)
            diff = diff + tankSize;
         //Serial.print(" ");
         //Serial.println( diff );
         if (diff<2){
            Serial.println(" Warning only 0.02 Second FIFO left");
         }
         if (diff<samplesFIFOMargin){
            myFile.sync();
            delay(100); 
         }
         if (diff> (tankSize-samplesFIFOMargin) ){
            Serial.println(" Warning terminating circular FIFO to be overwritten, margin left 0.1S");
            break;
         }
         if ( (i % (20*samplesFIFOMargin) )== 0) {

            Serial.print("Second starting ");
            Serial.println( i / (20*samplesFIFOMargin));
         }
      }

      round_count++;
      if(round_count == 3 ) round_count = 0; // reset to 0, meaning a new row has started
   }
  
    myFile.close();
    //Serial.println("SD file Closed ");
    currentPointer = indirectRead(startingFWAddress);         // estimating delay during the data saving process, pointer difference / 2 * 24 = elapsed time in ms
    Serial.print("Current Pointer after savind data ");
    Serial.println(currentPointer);
         
   //Serial.println(millis());

   // finally, save the current file number count
   if (SD.begin(SDCARD_SS_PIN, SPI_HALF_SPEED)) {  
      File latest_saved_SensorData_file = SD.open(LATEST_SAVED_SENSORDATA_FILE_NUMBER_PATH, FILE_WRITE);
      latest_saved_SensorData_file.rewind(); // go to start of file
      // if the file opened okay, write to it:
      if (latest_saved_SensorData_file) {
         Serial.print("Writing a ");
         Serial.print(currentFile);
         Serial.print(" to ");
         Serial.print(LATEST_SAVED_SENSORDATA_FILE_NUMBER_PATH);
         Serial.print(" ...");

         String file_content = String(currentFile) + "."; //make sure to put a fullstop
         if (latest_saved_SensorData_file.print(file_content)) {
            Serial.println("Done!");
         } else {
            Serial.println("Failed!");
         }
         // close the file:
         latest_saved_SensorData_file.close();
      }
      else {
         // if the file didn't open
         Serial.print("Error opening ");
         Serial.println(LATEST_SAVED_SENSORDATA_FILE_NUMBER_PATH);

         return;
      }
   }
}