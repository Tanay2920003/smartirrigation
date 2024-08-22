#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "thingProperties.h"

// Define a pin for the relay and soil moisture sensor
const int relayPin = 5;       // Change this pin number to match your setup
const int soilMoisturePin = A0; // Analog pin for soil moisture sensor

void setup() {
  // Initialize serial communication with Arduino Uno
  Serial.begin(9600);

  // Set the relay pin as output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Ensure relay is off initially

  // Initialize properties and IoT Cloud connection
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Initialize the soil moisture sensor pin
  pinMode(soilMoisturePin, INPUT);
}

void loop() {
  ArduinoCloud.update(); // Keep the cloud connection active

  // Control the relay based on the Cloud property value
  digitalWrite(relayPin, relayControl ? HIGH : LOW);

  // Read and update soil moisture value
  updateSoilMoisture();

  // Check if data is available from the Arduino Uno
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    processData(data);
  }
}

// Read the soil moisture value and update the cloud property
void updateSoilMoisture() {
  int rawValue = analogRead(soilMoisturePin); // Read the analog value
  float moisturePercentage = map(rawValue, 0, 1023, 0, 100); // Map the value to percentage (0-100)
  soilMoisture = moisturePercentage; // Update the cloud property
}

// Process incoming data from the Arduino Uno
void processData(String data) {
  // Example format: "S:5.2,T:23.1,H:55.3,M:68"
  int solarIndex = data.indexOf("S:");
  int tempIndex = data.indexOf("T:");
  int humidityIndex = data.indexOf("H:");
  int moistureIndex = data.indexOf("M:");

  if (solarIndex != -1 && tempIndex != -1 && humidityIndex != -1 && moistureIndex != -1) {
    solarVoltage = data.substring(solarIndex + 2, data.indexOf(",", solarIndex)).toFloat();
    temperature = data.substring(tempIndex + 2, data.indexOf(",", tempIndex)).toFloat();
    humidity = data.substring(humidityIndex + 2, data.indexOf(",", humidityIndex)).toFloat();
    soilMoisture = data.substring(moistureIndex + 2, data.indexOf(",", moistureIndex)).toFloat();
  }
}
