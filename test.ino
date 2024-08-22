#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Servo.h>

// Define I2C LCD parameters
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 to your LCD's I2C address if different

// Define DHT11 sensor parameters
#define DHTPIN 5        // DHT11 data pin
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// Define button parameters
#define BUTTON_PIN 6    // Button connected to digital pin 6
int currentDisplay = 0; // Variable to track the current display (0: Solar Voltage, 1: Temp, 2: Humidity, 3: Soil Moisture)

// Constants for Voltage Divider
#define R1 10000.0 // 10kΩ
#define R2 10000.0 // 10kΩ

// Define Soil Moisture Sensor parameters
#define MOISTURE_PIN A1 // Analog pin for soil moisture sensor

// Define Relay, Buzzer, LDR, and Servo pins
#define RELAY_PIN 7    // Pin connected to the relay
#define BUZZER_PIN 8   // Pin connected to the buzzer
#define LDR_DO_PIN 10  // Digital pin for LDR's DO
#define SERVO_PIN 9    // Pin connected to the servo motor

// Declare global variables
unsigned long lastUpdate = 0; // Variable to store the last update time
unsigned long buttonPressStart = 0; // Time when button press started
bool buttonPressed = false; // Button press state
bool pumpState = false; // false = OFF, true = ON
bool manualMode = false; // false = Automatic mode, true = Manual mode

Servo tankLid; // Create a Servo object for the tank lid

void setup() {
  // Initialize LCD
  lcd.begin(16, 2); // Initialize with 16 columns and 2 rows
  lcd.backlight();
  
  // Initialize DHT11 sensor
  dht.begin();
  
  // Initialize button pin as input with pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize relay, buzzer, and servo pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_DO_PIN, INPUT); // Set LDR DO pin as input
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially
  
  // Initialize servo
  tankLid.attach(SERVO_PIN);
  tankLid.write(90); // Set the servo to its initial position (closed)
  
  // Start serial communication for debugging
  Serial.begin(9600);
  
  // Display the initial screen
  displayData();
}

void loop() {
  unsigned long currentMillis = millis();

  // Read the button state
  bool buttonState = digitalRead(BUTTON_PIN);

  // Check if the button is pressed
  if (buttonState == LOW) {
    if (!buttonPressed) {
      buttonPressStart = currentMillis;
      buttonPressed = true;
    } else if (currentMillis - buttonPressStart >= 3000) { // 3 seconds press
      switchControlMode();
      buttonPressed = false; // Reset button state after action
    }
  } else {
    if (buttonPressed) {
      // Button was released after being pressed
      buttonPressed = false;
      if (currentMillis - buttonPressStart < 3000) {
        // Button was not pressed for 3 seconds, so switch display
        currentDisplay = (currentDisplay + 1) % 4; // Cycle through 0 to 3
        displayData();
      }
    }
  }

  // Update display data every second
  if (currentMillis - lastUpdate >= 1000) {
    lastUpdate = currentMillis;
    if (!buttonPressed) { // Update display only if not in button press state
      displayData();
    }
    
    // Control tank lid based on humidity and LDR value
    controlTankLid();

    // Automatic pump control based on temperature, soil moisture, and humidity
    if (!manualMode) {
      autoControlPump();
    } else {
      // In manual mode, check if soil moisture is greater than 60% and turn off pump if needed
      checkSoilMoisture();
    }
    
    // Send data to ESP32
    if (Serial.availableForWrite()) {
      Serial.print("S:");
      Serial.print(readSolarVoltage());
      Serial.print(",");
      Serial.print("T:");
      Serial.print(dht.readTemperature());
      Serial.print(",");
      Serial.print("H:");
      Serial.print(dht.readHumidity());
      Serial.print(",");
      Serial.print("M:");
      int moistureValue = analogRead(MOISTURE_PIN);
      Serial.print(map(moistureValue, 0, 1023, 100, 0)); // Convert to percentage
      Serial.print(",");
      Serial.print("P:");
      Serial.print(pumpState ? "ON" : "OFF"); // Relay state
      Serial.println();
    }
  }
}

void displayData() {
  lcd.clear();
  
  switch (currentDisplay) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Solar Voltage:");
      lcd.setCursor(0, 1);
      lcd.print(readSolarVoltage());
      lcd.print(" V");
      break;
      
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(dht.readTemperature());
      lcd.print(" C");
      break;
      
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Humidity:");
      lcd.setCursor(0, 1);
      lcd.print(dht.readHumidity());
      lcd.print(" %"); // Display humidity with percentage on next line
      break;

    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Soil Moisture:");
      lcd.setCursor(0, 1);
      int moistureValue = analogRead(MOISTURE_PIN);
      // Inverted mapping: higher value means drier soil, so we invert it
      float moisturePercentage = map(moistureValue, 0, 1023, 100, 0); // Inverted
      lcd.print(moisturePercentage);
      lcd.print(" %");
      break;
  }
}

float readSolarVoltage() {
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (6.0 / 1023.0); // Convert to voltage (based on 6V reference)
  return voltage * ((R1 + R2) / R2); // Adjust for voltage divider
}

void togglePump() {
  pumpState = !pumpState;
  
  if (pumpState) {
    // Turn on pump
    digitalWrite(RELAY_PIN, HIGH);
    displayMessage("Pump On");
    beep(1);
  } else {
    // Turn off pump
    digitalWrite(RELAY_PIN, LOW);
    displayMessage("Pump Off");
    beep(2);
  }
}

void displayMessage(const char* message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  delay(2000); // Show the message for 2 seconds
  displayData(); // Return to previous menu
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100); // Beep duration
    digitalWrite(BUZZER_PIN, LOW);
    delay(100); // Pause between beeps
  }
}

void autoControlPump() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int moistureValue = analogRead(MOISTURE_PIN);
  float moisturePercentage = map(moistureValue, 0, 1023, 100, 0); // Inverted

  // Define thresholds
  float tempThreshold = 25.0; // Threshold for temperature
  float moistureThreshold = 40.0; // Threshold for soil moisture
  float humidityThreshold = 50.0; // Upper threshold for humidity (now 50%)
  
  if (temperature > tempThreshold && moisturePercentage < moistureThreshold && humidity < humidityThreshold) {
    // Conditions met to turn on the pump
    if (!pumpState) {
      togglePump(); // Turn on pump
    }
  } else {
    // Conditions not met to keep the pump off
    if (pumpState) {
      togglePump(); // Turn off pump
    }
  }
}

void controlTankLid() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int ldrValue = digitalRead(LDR_DO_PIN);

  if (humidity > 80) {
    // Open the tank lid if humidity is above 80%
    tankLid.write(0); // Open position
  } else if (ldrValue == LOW) { // Daytime
    if (humidity <= 80) {
      // Ensure the lid is closed if daytime and humidity is not high
      tankLid.write(90); // Close position
    }
  } else { // Nighttime
    if (temperature < 20 && humidity >= 80) {
      // Open the tank lid at night if temperature is below 20°C and humidity is 80% or greater
      tankLid.write(0); // Open position
    } else {
      // Ensure the lid is closed if night and conditions are not met
      tankLid.write(90); // Close position
    }
  }
}

void checkSoilMoisture() {
  int moistureValue = analogRead(MOISTURE_PIN);
  float moisturePercentage = map(moistureValue, 0, 1023, 100, 0); // Inverted

  if (moisturePercentage < 60) {
    // Turn off pump if soil moisture is greater than 60%
    if (pumpState) {
      togglePump(); // Turn off pump
    }
  }
}

void switchControlMode() {
  manualMode = !manualMode;
  if (manualMode) {
    displayMessage("Manual Mode");
  } else {
    displayMessage("Auto Mode");
  }
}
