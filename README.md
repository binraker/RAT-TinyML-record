# RAT-TinyML-record
 RAT TinyML backpack code. recording only
 
 This is the code for the Syntiant TinyML board
 
 The base code for logging accelerometer data has been modified to communicate with an ESP32 over the 2nd serial port (pins 6 and 7). The TinyMl can communicate messages to the ESP32 which will then be forwarded to the user on the viewing webpage
 Whenever the ESP32 receives a time update(either from NPT server or the browser) it sends a message to the TinyML to update the RTC so that both systems will end up with synchronously time stamped log files.

6 pin connections betwixt ESP32 and TinyML board. 
_|_  
---|---
5V|Tx
3V3|Rx
IO0|GND

5V and IO0 are used for programming. 3V3 is used in the backpack and takes power from the 3V3 test pad on the TinyML.

Images of the hardware
![image1](media/20220806_203641.jpg)

The next step is to gather some data and train the ML modle. The TinyML will then generate a message when it detects something in the accelerometer data-stream that looks like an action we have labelled.
