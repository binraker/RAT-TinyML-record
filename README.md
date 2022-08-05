# RAT-TinyML-record
 RAT TinyML backpack code. recording only
 
 This is the code for the Syntiant TinyML board
 
 The base code for logging accelerometer data has been modified to communicate with an ESP32 over the 2nd serial port. The TinyMl can communicate messages to the ESP32 which will then be forwarded to the user on the viewing webpage and whenever the ESP32 receives a time update it sends a message to the TinyML to update the RTC so that both systems will end up with synchronously time stamped log files.

