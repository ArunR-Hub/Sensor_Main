#include "TdsReadings.h"
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <MPU6050.h>

#define TdsSensorPin 34
#define VREF 3.3
#define SCOUNT 30

#define PWR_CTRL 25
#define SD_CS_PIN 4


extern File logFile;

const int nodeID = 2;
int analogBuffer[SCOUNT];

float getMedianVoltage(int *buffer, int len);
String leadingZero(int num);

void readAndLogTDS() {
  pinMode(PWR_CTRL, OUTPUT);
  digitalWrite(PWR_CTRL, HIGH);
  delay(500);

  Wire.begin(21, 22);

  if (!rtc.begin()) {
    Serial.println("❌ RTC not found!");
    return;
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("⚠️ RTC time set due to power loss.");
  }

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 not found!");
    return;
  }

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD card init failed!");
    return;
  }

  float temperature = mpu.getTemperature() / 340.00 + 36.53;
  Serial.print("🌡 MPU Temp: ");
  Serial.println(temperature, 2);

  for (int i = 0; i < SCOUNT; i++) {
    analogBuffer[i] = analogRead(TdsSensorPin);
    delay(40);
  }

  float avgVoltage = getMedianVoltage(analogBuffer, SCOUNT);
  float compCoeff = 1.0 + 0.02 * (temperature - 25.0);
  float compVoltage = avgVoltage / compCoeff;

  float TDSreading = (133.42 * pow(compVoltage, 3) - 255.86 * pow(compVoltage, 2) + 857.39 * compVoltage) * 0.5;

  if (TDSreading < 0 || TDSreading > 2000) {
    Serial.print("⚠️ Abnormal TDS value discarded: ");
    Serial.println(TDSreading);
    TDSreading = -1;
  }

DateTime now = rtc.now();
  // DateTime now = rtc.now();
  // String timestamp = String(now.year()) + "-" + leadingZero(now.month()) + "-" + leadingZero(now.day()) + " " +
  //                    leadingZero(now.hour()) + ":" + leadingZero(now.minute()) + ":" + leadingZero(now.second());

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  char dateStr[12];
  sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

  if (TDSreading >= 0) {
    logFile = SD.open("/Readings.json", FILE_APPEND);
    if (logFile) {
      // logFile.print("{\"N\":");
      // logFile.print(nodeID);
      // logFile.print(", \"T\":\"");
      // logFile.print(timestamp);
      // logFile.print("\", \"TDS\":");
      // logFile.print(TDSreading, 2);
      // logFile.println("}");
      // logFile.close();
      // Serial.println("✅ Valid data logged to SD");

      logFile.print("{\"N\":");
      logFile.print(nodeID);
      logFile.print(", \"D\":\"");
      logFile.print(dateStr);
      logFile.print("\", \"T\":\"");
      logFile.print(timeStr);
      logFile.print("\", \"TDS\":");
      logFile.print(TDSreading, 2);
      logFile.println("}");
      Serial.println("✅ Valid data logged to SD");

      logFile.close();

    } else {
      Serial.println("tds Failed to open /Readings.json");
    }
  } else {
    Serial.println("⏭ tds Skipped logging due to invalid TDS value.");
  }

  digitalWrite(PWR_CTRL, LOW);

  // Read back the SD log
  digitalWrite(PWR_CTRL, HIGH);
  delay(300);
  File readFile = SD.open("/Readings.json", FILE_READ);
  if (readFile) {
    Serial.println("📖 Logged Data:");
    while (readFile.available()) {
      Serial.write(readFile.read());
    }
    readFile.close();
  } else {
    Serial.println("tds Cannot open /Readings.json to read");
  }
  digitalWrite(PWR_CTRL, LOW);
}

float getMedianVoltage(int *buffer, int len) {
  int sorted[len];
  memcpy(sorted, buffer, len * sizeof(int));
  for (int i = 0; i < len - 1; i++) {
    for (int j = 0; j < len - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        int temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  int median = (len % 2 == 0)
                 ? (sorted[len / 2 - 1] + sorted[len / 2]) / 2
                 : sorted[len / 2];
  return median * (float)VREF / 4096.0;
}

String leadingZero(int num) {
  return (num < 10) ? "0" + String(num) : String(num);
}
