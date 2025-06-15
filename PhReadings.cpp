#include "PhReadings.h"
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define PH_PIN     32      // ADC pin for pH sensor
#define NUM_SAMPLES 10     // Number of samples for averaging
#define PWR_CTRL   25      // MOSFET switch control pin
#define SD_CS_PIN  4

extern File logFile;
extern RTC_DS3231 rtc;

const int nodeID = 2; // Change if needed

// Calibration constants (adjust for your probe!)
const float PH_SLOPE = -5.70;
const float PH_CALIBRATION = 21.34;

void PhReadings() {
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

    // Read pH sensor (averaging only)
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(PH_PIN);
        delay(30);
    }
    float avg = sum / (float)NUM_SAMPLES;
    float voltage = avg * (3.3 / 4095.0);  // 12-bit ADC, 3.3V reference
    float phValue = PH_SLOPE * voltage + PH_CALIBRATION;

    Serial.print("Voltage: ");
    Serial.print(voltage, 3);
    Serial.print(" V,  pH: ");
    Serial.println(phValue, 2);

    DateTime now = rtc.now();
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    char dateStr[12];
    sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

    // Only log valid pH values (change range if needed)
    if (phValue >= 0 && phValue <= 14.0) {
        logFile = SD.open("/Readings.json", FILE_APPEND);
        if (logFile) {
            logFile.print("{\"N\":");
            logFile.print(nodeID);
            logFile.print(", \"D\":\"");
            logFile.print(dateStr);
            logFile.print("\", \"T\":\"");
            logFile.print(timeStr);
            logFile.print("\", \"PH\":");
            logFile.print(phValue, 2);
            logFile.println("}");
            Serial.println("✅ PH data logged to SD");
            logFile.close();
        } else {
            Serial.println("PH Failed to open /Readings.json");
        }
    } else {
        Serial.println("PH Skipped logging due to invalid pH value.");
    }

    digitalWrite(PWR_CTRL, LOW);

    // (Optional) Print the latest SD log file
    digitalWrite(PWR_CTRL, HIGH);
    delay(300);
    File readFile = SD.open("/Readings.json", FILE_READ);
    if (readFile) {
        Serial.println("📖 PH Logged Data:");
        while (readFile.available()) {
            Serial.write(readFile.read());
        }
        readFile.close();
    } else {
        Serial.println("Failed to PH read /Readings.json");
    }
    digitalWrite(PWR_CTRL, LOW);
}



// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// #include <Arduino.h>
// #include "PhReadings.h"
// #include <RTClib.h>
// extern RTC_DS3231 rtc;

// #define PWR_CTRL 25
// #define SD_CS_PIN 4

// const int nodeID = 2;

// // String leeadingZero(int num);

// // ---- Configuration Section ----
// #define PH_PIN 32                    // ADC pin for pH sensor
// #define NUM_SAMPLES 10               // Number of samples for averaging
// const float PH_SLOPE = -5.70;        // Calibration slope (adjust to your probe)
// const float PH_CALIBRATION = 21.34;  // Calibration intercept (adjust to your probe)
// // --------------------------------

// void PhReadings() {
//   long sum = 0;
//   for (int i = 0; i < NUM_SAMPLES; i++) {
//     sum += analogRead(PH_PIN);
//     delay(10);
//   }
//   float avg = sum / (float)NUM_SAMPLES;
//   float voltage = avg * (3.3 / 4095.0);  // ESP32 12-bit ADC
//   float phValue = PH_SLOPE * voltage + PH_CALIBRATION;

//   Serial.print("Voltage: ");
//   Serial.print(voltage, 3);
//   Serial.print(" V,  pH: ");
//   Serial.println(phValue, 2);

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

//   DateTime now = rtc.now();

//   char timeStr[10];
//   sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

//   char dateStr[12];
//   sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

//   if (phValue >= 0) {
//     logFile = SD.open("/Readings.json", FILE_APPEND);
//     if (logFile) {
//       logFile.print("{\"N\":");
//       logFile.print(nodeID);
//       logFile.print(", \"D\":\"");
//       logFile.print(dateStr);
//       logFile.print("\", \"T\":\"");
//       logFile.print(timeStr);
//       logFile.print("\", \"PH\":");
//       logFile.print(phValue, 2);
//       logFile.println("}");
//       Serial.println("✅ Valid data logged to SD");

//       logFile.close();
//     } else {
//       Serial.println("PH Failed to open /Readings.json");
//     }
//   } else {
//     Serial.println("PH Skipped logging due to invalid phValue.");
//   }

//   Serial.println("Reading PH /Readings.json contents:");
//   File readFile = SD.open("/Readings.json", FILE_READ);
//   if (readFile) {
//     while (readFile.available()) {
//       String line = readFile.readStringUntil('\n');
//       Serial.println(line);
//     }
//     readFile.close();
//   } else {
//     Serial.println("Failed to PH read /Readings.json");
//   }
// }
