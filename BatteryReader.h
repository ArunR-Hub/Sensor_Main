
// #endif // BATTERY_READER_H
#ifndef BATTERY_READER_H
#define BATTERY_READER_H

#include <Arduino.h>
#include <SD.h>

// Pin definitions
#define VOLTAGE_PIN 32
#define NUM_SAMPLES 10

// Voltage Divider Resistors
#define R1 29700.0  // 30kΩ (top resistor)
#define R2 7400.0   // 7.5kΩ (bottom resistor)

// Extern reference to logFile and RTC
extern File logFile;

float readBatteryVoltage();
void updBATtoSDcard();

#endif // BATTERY_READER_H
