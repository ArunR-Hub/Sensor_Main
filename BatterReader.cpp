
#include "BatteryReader.h"
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define PWR_CTRL    25
#define SD_CS_PIN   4
#define V_CAL_VALUE 1.26934

extern File logFile;
extern RTC_DS3231 rtc;

//const int nodeID = 2;  // Adjust if dynamic node IDs
extern String NODE_ID;

float readBatteryVoltage() {
    // long sum = 0;
    // for (int i = 0; i < NUM_SAMPLES; i++) {
    //     sum += analogRead(VOLTAGE_PIN);
    //     delay(10);
    // }
    // float average = sum / (float)NUM_SAMPLES;
    float adcVoltage = (analogRead(VOLTAGE_PIN) * 3.3) / 4095.0;

    // Log the scaled voltage to Serial
    Serial.print("📉 Vadc at GPIO"); Serial.print(VOLTAGE_PIN);
    Serial.print(": "); Serial.print(adcVoltage, 3); Serial.println(" V");

    // Convert to actual battery voltage
    float batteryVoltage = adcVoltage * ((R1 + R2) / R2) * 0.7511 ;
    Serial.print("🔋 Calculated Battery Voltage: ");
    Serial.print(batteryVoltage, 2); Serial.println(" V");

    return batteryVoltage;
}

void updBATtoSDcard() {
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

    if (batteryVoltage > 0.1 && batteryVoltage < 20.0) {
        logFile = SD.open("/Readings.json", FILE_APPEND);
        if (logFile) {
            logFile.print("{\"N\":");
            logFile.print(NODE_ID);
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
        Serial.print("⚠️ Invalid voltage reading: ");
        Serial.println(batteryVoltage, 2);
        Serial.println("BAT Skipped logging due to out-of-range voltage.");
    }

    digitalWrite(PWR_CTRL, LOW);

    // Optional: print latest battery log
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
