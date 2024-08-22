#ifndef THINGPROPERTIES_H
#define THINGPROPERTIES_H

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "arduino_secrets.h" // Include your secrets

const char DEVICE_LOGIN_NAME[]  = "d19f2e73-60ba-425d-8514-7cbb01774ae2";
const char SSID[]               = SECRET_SSID;    // Ensure this is defined in arduino_secrets.h
const char PASS[]               = SECRET_PASS;    // Ensure this is defined in arduino_secrets.h
const char DEVICE_KEY[]         = SECRET_DEVICE_KEY; // Ensure this is defined in arduino_secrets.h

// Define the properties
CloudBool relayControl;
CloudFloat solarVoltage;
CloudFloat temperature;
CloudFloat humidity;
CloudFloat soilMoisture;
CloudBool pumpState; // Add this line to declare the Cloud property for pump state


void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(relayControl, READWRITE); // Relay control property
  ArduinoCloud.addProperty(solarVoltage, READ);      // Solar voltage property
  ArduinoCloud.addProperty(temperature, READ);        // Temperature property
  ArduinoCloud.addProperty(humidity, READ);           // Humidity property
  ArduinoCloud.addProperty(soilMoisture, READWRITE);  // Soil moisture property
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

#endif
