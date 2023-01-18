// This code is based on Sensirion's Arduino Snippets
// Check https://github.com/Sensirion/arduino-snippets for the most recent version.

/*
  This code is based on Rui Santos Snippets
  Complete project details at https://RandomNerdTutorials.com/esp32-date-time-ntp-client-server-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include "esp_timer.h"
#include "Sensirion_GadgetBle_Lib.h"
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include "time.h"


// GadgetBle workflow
static int64_t lastMmntTime = 0;
static int mmntIntervalUs = 5000000;
GadgetBle gadgetBle = GadgetBle(GadgetBle::DataType::T_RH_CO2);

SensirionI2CScd4x scd4x;

// wifi setup
// add your wifi info into a config.h file like this:
// const char* ssid     = "myssid";
// const char* password = "mywifipassword";
#include "config.h"

// ntp setup
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);
  // wait for serial connection from PC
  // comment the following line if you'd like the output
  // without waiting for the interface being ready
  while(!Serial);

  // Initialize the GadgetBle Library
  gadgetBle.begin();
  Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
  Serial.println(F(gadgetBle.getDeviceIdString()));
  
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println((ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(F(""));
  Serial.println(F("WiFi connected."));

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // init I2C
  Wire.begin();

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  scd4x.stopPeriodicMeasurement();

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(F(errorMessage));
  }

  Serial.println(F("Waiting for first measurement... (5 sec)"));
  Serial.println(F("CO2(ppm)\tTemperature(degC)\tRelativeHumidity(percent)"));
  delay(5000);
}

void loop() {
  if (esp_timer_get_time() - lastMmntTime >= mmntIntervalUs) {
    measure_and_report();
  }

  printLocalTime();

  gadgetBle.handleEvents();
  delay(3);
}

void measure_and_report() {
  uint16_t error;
  char errorMessage[256];
    
  // Read Measurement
  uint16_t co2;
  float temperature;
  float humidity;

  error = scd4x.readMeasurement(co2, temperature, humidity);
  lastMmntTime = esp_timer_get_time();

  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(F(errorMessage));
    return;
  }

  if (co2 == 0) {
    Serial.println(F("Invalid sample detected, skipping."));
    return;
  }

  Serial.print(co2);
  Serial.print("\t");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.println(F(humidity));

  gadgetBle.writeCO2(co2);
  gadgetBle.writeTemperature(temperature);
  gadgetBle.writeHumidity(humidity);
  gadgetBle.commit();
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println(F("Failed to obtain time"));
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println((&timeinfo, "%Y"));
  Serial.print("Hour: ");
  Serial.println((&timeinfo, "%H"));
  Serial.print("Hour (12 hour format): ");
  Serial.println((&timeinfo, "%I"));
  Serial.print("Minute: ");
  Serial.println((&timeinfo, "%M"));
  Serial.print("Second: ");
  Serial.println((&timeinfo, "%S"));

  Serial.println(F("Time variables"));
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(F(timeHour));
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(F(timeWeekDay));
  Serial.println(F());
}