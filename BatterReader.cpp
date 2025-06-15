#include "BatteryReader.h"
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define PWR_CTRL    25
#define SD_CS_PIN   4

extern File logFile;
extern RTC_DS3231 rtc;

const int nodeID = 2;  // Adjust if dynamic node IDs

float readBatteryVoltage() {
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(VOLTAGE_PIN);
        delay(10);
    }
    float average = sum / (float)NUM_SAMPLES;
    float adcVoltage = (average * 3.3) / 4095.0;
    float batteryVoltage = adcVoltage / VBAT_CALIBRATION_FACTOR;
    return batteryVoltage;
}

void updBATtoSDcard() {
    // Power ON sensors and SD card
    pinMode(PWR_CTRL, OUTPUT);
    digitalWrite(PWR_CTRL, HIGH);
    delay(500);

    Wire.begin(21, 22);

    if (!rtc.begin()) {
        Serial.println("❌ RTC not found!");
        digitalWrite(PWR_CTRL, LOW);
        return;
    }
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("⚠️ RTC time set due to power loss.");
    }

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("❌ SD card init failed!");
        digitalWrite(PWR_CTRL, LOW);
        return;
    }
    delay(200);

    float batteryVoltage = readBatteryVoltage();

    DateTime now = rtc.now();
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    char dateStr[12];
    sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

    // Only log valid battery voltage
    if (batteryVoltage >= 0 && batteryVoltage < 5.0) {  // 5V upper bound for sanity
        logFile = SD.open("/Readings.json", FILE_APPEND);
        if (logFile) {
            logFile.print("{\"N\":");
            logFile.print(nodeID);
            logFile.print(", \"D\":\"");
            logFile.print(dateStr);
            logFile.print("\", \"T\":\"");
            logFile.print(timeStr);
            logFile.print("\", \"BAT\":");
            logFile.print(batteryVoltage, 2);
            logFile.println("}");
            Serial.println("✅ BAT data logged to SD");
            logFile.close();
        } else {
            Serial.println("BAT Failed to open /Readings.json");
        }
    } else {
        Serial.println("BAT Skipped logging due to invalid battery voltage.");
    }

    digitalWrite(PWR_CTRL, LOW);

    // (Optional) Print the latest SD log file
    digitalWrite(PWR_CTRL, HIGH);
    delay(300);
    File readFile = SD.open("/Readings.json", FILE_READ);
    if (readFile) {
        Serial.println("📖 BAT Logged Data:");
        while (readFile.available()) {
            Serial.write(readFile.read());
        }
        readFile.close();
    } else {
        Serial.println("Failed to BAT read /Readings.json");
    }
    digitalWrite(PWR_CTRL, LOW);
}

//----------------------------------------

// #include <Arduino.h>
// #include "BatteryReader.h"
// #include <Wire.h>
// #include <RTClib.h>
// #include <SPI.h>
// #include <SD.h>
// #include "TdsReadings.h"


// extern RTC_DS3231 rtc;

// #define PWR_CTRL 25
// #define SD_CS_PIN 4

// const int nodeID = 2;

// #define CALIBRATION_FACTOR 0.686

// float readBatteryVoltage() {
//   long sum = 0;
//   for (int i = 0; i < NUM_SAMPLES; i++) {
//     sum += analogRead(VOLTAGE_PIN);
//     delay(10);
//   }

//   float average = sum / (float)NUM_SAMPLES;
//   float adcVoltage = (average * 3.3) / 4095.0;
//   float batteryVoltage = adcVoltage / VBAT_CALIBRATION_FACTOR * CALIBRATION_FACTOR;

//   return batteryVoltage;
// }

// void updBATtoSDcard() {
//   float batteryVoltage = readBatteryVoltage();

//   pinMode(PWR_CTRL, OUTPUT);
//   digitalWrite(PWR_CTRL, HIGH);
//   delay(500);

//   Wire.begin(21, 22);

//   if (!rtc.begin()) {
//     Serial.println("❌ RTC not found!");
//     return;
//   }

//   if (rtc.lostPower()) {
//     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//     Serial.println("⚠️ RTC time set due to power loss.");
//   }

//   if (!SD.begin(SD_CS_PIN)) {
//     Serial.println("❌ SD card init failed!");
//     return;
//   }
//   delay(500);

//   DateTime now = rtc.now();

//   char timeStr[10];
//   sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

//   char dateStr[12];
//   sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

//   if (batteryVoltage >= 0) {
//     logFile = SD.open("/Readings.json", FILE_APPEND);
//     if (logFile) {
//       logFile.print("{\"N\":");
//       logFile.print(nodeID);
//       logFile.print(", \"D\":\"");
//       logFile.print(dateStr);
//       logFile.print("\", \"T\":\"");
//       logFile.print(timeStr);
//       logFile.print("\", \"BAT\":");
//       logFile.print(batteryVoltage, 2);
//       logFile.println("}");
//       Serial.println("✅ Valid data logged to SD");

//       logFile.close();
//     } else {
//       Serial.println("bat Failed to open /Readings.json");
//     }
//   } else {
//     Serial.println("bat Skipped logging due to invalid battery voltage.");
//   }

//   Serial.println("Reading bat /Readings.json contents:");
//   File readFile = SD.open("/Readings.json", FILE_READ);
//   if (readFile) {
//     while (readFile.available()) {
//       String line = readFile.readStringUntil('\n');
//       Serial.println(line);
//     }
//     readFile.close();
//   } else {
//     Serial.println("Failed bat to read /Readings.json");
//   }
// }
