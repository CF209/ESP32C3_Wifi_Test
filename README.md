I created this project to test out some of the Wifi functions on the ESP32C3 using the Arduino IDE.

The current program starts the ESP as a wifi access point and hosts a webpage at http://esp32.local/

From the webpage you can input a new Wifi SSID and Password that the ESP will save and connect to on startup. If it fails to connect it will default back to access point mode.

From http://esp32.local/update you can update the firmware on the ESP.

I also included buttons to turn on and off an LED.

I'll most likely use this code as a base for future projects
