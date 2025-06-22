
#include <Wire.h>
#include <MPU6050.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS 4
#define SD_PWR_EN 25

extern MPU6050 mpu;
extern RTC_DS3231 rtc;
//const int nodeID = 2;
extern String NODE_ID;

void logWaterHeight() {

  Serial.println("Starting water height averaging...");
  // SPI.begin(18, 19, 23, SD_CS);
  // delay(2000);
  // if (!SD.begin(SD_CS)) {
  //   Serial.println("❌ SD card not detected. Re-initialization failed!");
  //   return;
  // }
  // delay(2000);

  float totalWH = 0;
  const float L = 34.0, HT = 34.0;

  for (int i = 0; i < 10; i++) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    float axg = ax / 16384.0, ayg = ay / 16384.0, azg = az / 16384.0;
    float roll = atan2(ayg, sqrt(axg * axg + azg * azg)) * 180.0 / PI;
    float WH = HT + L * sin(roll * PI / 180.0);
    totalWH += WH;
    delay(200);
  }

  float avgWH = totalWH / 10.0;
  DateTime now = rtc.now();
  float temperature = rtc.getTemperature();

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  char dateStr[12];
  sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

  Serial.print("Time: ");
  Serial.print(dateStr);
  Serial.print(" ");
  Serial.print(timeStr);
  Serial.print(" | Avg WH: ");
  Serial.print(avgWH, 2);
  Serial.print(" cm | Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  // Open for append (creates file if doesn't exist)
  File logFile = SD.open("/Readings.json", FILE_APPEND);
  if (!logFile) {
    Serial.println("Failed to open /Readings.json for appending!");
    return;
  }
  // Save as two lines (water height and temperature)
  logFile.print("{\"N\":");
  logFile.print(NODE_ID);
  logFile.print(",\"D\":\"");
  logFile.print(dateStr);
  logFile.print("\",\"T\":\"");
  logFile.print(timeStr);
  logFile.print("\",\"WH\":");
  logFile.print(avgWH, 2);
  logFile.println("}");

  logFile.print("{\"N\":");
  logFile.print(NODE_ID);
  logFile.print(",\"D\":\"");
  logFile.print(dateStr);
  logFile.print("\",\"T\":\"");
  logFile.print(timeStr);
  logFile.print("\",\"TEM\":");
  logFile.print(temperature, 2);
  logFile.println("}");

  logFile.close();
  Serial.println("WH & TEM data logged to SD card.");

  // Optionally: read back file to confirm
  File readFile = SD.open("/Readings.json", FILE_READ);
  if (readFile) {
    while (readFile.available()) {
      Serial.println(readFile.readStringUntil('\n'));
    }
    readFile.close();
  } else {
    Serial.println("Failed to read /Readings.json");
  }
}
