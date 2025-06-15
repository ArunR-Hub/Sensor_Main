// #include <Wire.h>
// #include <MPU6050.h>
// #include <RTClib.h>
// #include <SPI.h>
// #include <SD.h>

// #define SD_CS 4
// #define SD_PWR_EN 25

// extern MPU6050 mpu;
// extern RTC_DS3231 rtc;

// const int nodeID = 2;  // Node ID defined globally

// void logWaterHeight() {
//   Serial.println("Starting water height averaging...");

//   pinMode(SD_PWR_EN, OUTPUT);
//   digitalWrite(SD_PWR_EN, HIGH);
//   delay(500);

//   Wire.begin(21, 22);
//   SPI.begin(18, 19, 23, SD_CS);
//   if (!SD.begin(SD_CS)) {
//     Serial.println("❌ SD card initialization failed!");
//     while (true) ;
//   }


//   float totalWH = 0;
//   const float L = 20.0;
//   const float HT = 10.0;

//   for (int i = 0; i < 10; i++) {
//     int16_t ax, ay, az;
//     mpu.getAcceleration(&ax, &ay, &az);

//     float axg = ax / 16384.0;
//     float ayg = ay / 16384.0;
//     float azg = az / 16384.0;

//     float roll = atan2(ayg, sqrt(axg * axg + azg * azg)) * 180.0 / PI;
//     float radians = roll * PI / 180.0;
//     float WH = HT + L * sin(radians);

//     totalWH += WH;
//     delay(200);
//   }

//   float avgWH = totalWH / 10.0;
//   DateTime now = rtc.now();
//   float temperature = rtc.getTemperature();

//   char timeStr[10];
//   sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

//   char dateStr[12];
//   sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

//   Serial.print("Time: ");
//   Serial.print(dateStr);
//   Serial.print(" ");
//   Serial.print(timeStr);
//   Serial.print(" | Avg WH: ");
//   Serial.print(avgWH, 2);
//   Serial.print(" cm | Temperature: ");
//   Serial.print(temperature, 2);
//   Serial.println(" °C");

//   if (!SD.exists("/Readings.json")) {
//     Serial.println("/Readings.json not found. Creating...");
//     File createFile = SD.open("/Readings.json", FILE_WRITE);
//     if (createFile) {
//       createFile.close();
//       Serial.println("File created.");
//     } else {
//       Serial.println("Failed to create /Readings.json");
//       return;
//     }
//   }

//   File logFile = SD.open("/Readings.json", FILE_APPEND);
//   if (logFile) {
//     // Water height entry
//     logFile.print("{\"N\":");
//     logFile.print(nodeID);
//     logFile.print(", \"D\":\"");
//     logFile.print(dateStr);
//     logFile.print("\", \"T\":\"");
//     logFile.print(timeStr);
//     logFile.print("\", \"WH\":");
//     logFile.print(avgWH, 2);
//     logFile.println("}");

//     // Temperature entry
//     logFile.print("{\"N\":");
//     logFile.print(nodeID);
//     logFile.print(", \"D\":\"");
//     logFile.print(dateStr);
//     logFile.print("\", \"T\":\"");
//     logFile.print(timeStr);
//     logFile.print("\", \"TEM\":");
//     logFile.print(temperature, 2);
//     logFile.println("}");

//     logFile.close();
//     Serial.println("TEM Data logged to SD card.");
//   } else {
//     Serial.println("Failed to open TEM WH /Readings.json for appending.");
//     return;
//   }

//   Serial.println("Reading WH /Readings.json contents:");
//   File readFile = SD.open("/Readings.json", FILE_READ);
//   if (readFile) {
//     while (readFile.available()) {
//       String line = readFile.readStringUntil('\n');
//       Serial.println(line);
//     }
//     readFile.close();
//   } else {
//     Serial.println("TEM  WH Failed to read /Readings.json");
//   }
// }


//-----------------------

#include <Wire.h>
#include <MPU6050.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS 4
#define SD_PWR_EN 25

extern MPU6050 mpu;
extern RTC_DS3231 rtc;
const int nodeID = 2;

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
  const float L = 20.0, HT = 10.0;

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
  logFile.print(nodeID);
  logFile.print(",\"D\":\"");
  logFile.print(dateStr);
  logFile.print("\",\"T\":\"");
  logFile.print(timeStr);
  logFile.print("\",\"WH\":");
  logFile.print(avgWH, 2);
  logFile.println("}");

  logFile.print("{\"N\":");
  logFile.print(nodeID);
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
