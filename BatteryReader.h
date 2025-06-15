// #ifndef BATTERY_READER_H
// #define BATTERY_READER_H

// #define VOLTAGE_PIN 32
// #define VBAT_CALIBRATION_FACTOR 0.1995 // Based on 30kΩ:7.5kΩ voltage divider
// #define NUM_SAMPLES 10


// float readBatteryVoltage();
// void updBATtoSDcard();
// #include <SD.h>
// extern File logFile;

// #endif
#ifndef BATTERY_READER_H
#define BATTERY_READER_H

#include <Arduino.h>
#include <SD.h>

// Pin definitions and calibration
#define VOLTAGE_PIN 32
#define VBAT_CALIBRATION_FACTOR 0.1995 // For 30k:7.5k voltage divider (adjust if needed)
#define NUM_SAMPLES 10

// Extern reference to logFile (must be defined in .ino or main.cpp)
extern File logFile;

// Function declarations
float readBatteryVoltage();
void updBATtoSDcard();

#endif // BATTERY_READER_H
