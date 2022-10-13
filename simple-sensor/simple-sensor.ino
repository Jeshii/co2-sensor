/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <TimeLib.h>
#include <LiquidCrystal.h>
#include <math.h>

SensirionI2CScd4x scd4x;

// Set up LCD object for shield
LiquidCrystal lcd(8,9,4,5,6,7);

#define LCD_Backlight 10

// Define buttons on shield
#define btnRIGHT    0
#define btnUP       1
#define btnDOWN     2
#define btnLEFT     3
#define btnSELECT   4
#define btnNONE     5

// Define variable to hold current button
int lcd_key = 0;

// Define variabke to hold button analog value
int adc_key_in = 0;

int lcd_level = 16;

//Function to read buttons
int read_LCD_buttons() {
  // read value from sensor
  adc_key_in = analogRead(0);

  // Approximate button values are 0, 144, 329, 504, and 741
  // Add 50 to those values and check to see if we are close
  if (adc_key_in > 1000)   return btnNONE; // No button is pressed
  if (adc_key_in < 50)     return btnRIGHT; // Right button pressed
  if (adc_key_in < 195)    return btnUP; // Right button pressed
  if (adc_key_in < 380)    return btnDOWN; // Right button pressed
  if (adc_key_in < 555)    return btnLEFT; // Right button pressed
  if (adc_key_in < 790)    return btnSELECT; // Right button pressed
  return btnNONE; // if no response
}

//custom characters
byte degC[8] = {B01000,B10100,B01000,B00010,B00101,B00100,B00101,B00010};
byte small2[8] = {B00000,B00000,B01100,B10010,B00010,B00100,B01000,B11110};

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("CO2 Sensor Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

void setup() {

    setTime(9,45,0,13,10,2022);

    lcd.begin(16,2);
    lcd.setCursor(0,0);

    lcd.createChar(1,degC);
    lcd.createChar(2,small2);

    pinMode(LCD_Backlight, OUTPUT);
    analogWrite(LCD_Backlight, (lcd_level*8)-1);

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");
}

void loop() {
    uint16_t error;
    char errorMessage[256];

    switch (lcd_key) {
      case btnUP: {
          if (lcd_level < 30) {
            lcd_level++;
          }
          analogWrite(LCD_Backlight, (lcd_level*8)-1);
          Serial.println(lcd_level);
          break;
      }
      case btnDOWN: {
          if (lcd_level > 2) {
            lcd_level--;  
          }
          analogWrite(LCD_Backlight, (lcd_level*8)-1);
          Serial.println(lcd_level);
          break;
      }
      case btnNONE: {
          Serial.println(lcd_level);
          break;
      }
    }

    int lastSecond=0;  
    if ((second()%2 == 0) && (second() != lastSecond)) {
        //print date and time
        lcd.setCursor(0,0);
        lcd.print(year());
        lcd.print("/");
        if (month()<10) {
            lcd.print("0");
        }
        lcd.print(month());
        lcd.print("/");
        if (day()<10) {
            lcd.print("0");
        }
        lcd.print(day());
        
        lcd.setCursor(11,0);
        if (hour()<10) {
            lcd.print("0");
        }
        lcd.print(hour());
        lcd.print(":");
        if (minute()<10) {
            lcd.print("0");
        }
        lcd.print(minute());

        // Read Measurement
        uint16_t co2 = 0;
        float temperature = 0.0f;
        float humidity = 0.0f;
        bool isDataReady = false;
        error = scd4x.getDataReadyFlag(isDataReady);
        if (error) {
            Serial.print("Error trying to execute readMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
            return;
        }
        if (!isDataReady) {
            return;
        }
        error = scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            Serial.print("Error trying to execute readMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        } else if (co2 == 0) {
            Serial.println("Invalid sample detected, skipping.");
        } else {        
            lcd.setCursor(0,1);
            lcd.print("CO");
            lcd.write(2);
            lcd.print(":");
            if(co2<1000) {
              lcd.print("0");
            }
            lcd.print(co2);
            lcd.setCursor(9,1);
            lcd.print(round(temperature));
            lcd.write(1);
            lcd.print("/");
            lcd.print(round(humidity));
            lcd.print("%");
            Serial.println(lcd_level);
            Serial.println(co2);
        }
      lastSecond=second();
    }
    else if (second()%2 == 1) {
       lcd.setCursor(13,0);
        lcd.print(" ");
    }
}
