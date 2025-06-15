// #include <Wire.h>
// #include <SPI.h>
// #include <SD.h>
// #include "I2Cdev.h"
// #include "MPU6050.h"
// #include "RTClib.h"
// #include "WaterHeightLogger.h"
// #include "LoRaSender.h"
// #include "TdsReadings.h"
// #include "BatteryReader.h"
// #include "PhReadings.h"

// const int nodeID = 2;

// File logFile;

// #define SD_CS 4
// #define SD_PWR_EN 25
// #define RTC_INT_PIN 35
// #define uS_TO_S 1000000ULL


// // Define trigger minutes for actions
// int readMinute1 = 7;
// int readMinute2 = 8;
// int sendMinute = 9;

// // Define how long to wait until next wake-up (in minutes)
// int waitAfterRead1 = readMinute2 - readMinute1;     // = 4
// int waitAfterRead2 = sendMinute - readMinute2;      // = 5
// int waitAfterSend = 60 - sendMinute + readMinute1;  // = 51


// MPU6050 mpu(0x69);
// RTC_DS3231 rtc;

// int state;


// void setup() {

//   Serial.begin(115200);
//   delay(200);
//   Serial.println("🔧 Starting setup...");

//   Wire.begin(21, 22);
//   Wire.setClock(400000);

//   //mpu.initialize();
//   //Serial.println("✅ MPU6050 initialized");

//   if (!rtc.begin()) {
//     Serial.println("❌ RTC not found!");
//     while (true)
//       ;
//   }
//   Serial.println("✅ RTC initialized");

//   //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));----only if the time is not sync

//   if (rtc.lostPower()) {
//     Serial.println("⚠️ RTC lost power, resetting time");
//     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//   }

//   rtc.disable32K();
//   rtc.clearAlarm(1);
//   rtc.clearAlarm(2);
//   rtc.writeSqwPinMode(DS3231_OFF);
//   rtc.disableAlarm(2);
//   pinMode(RTC_INT_PIN, INPUT_PULLUP);

//   if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
//     Serial.println("⏰ Woke up from RTC alarm");
//     rtc.clearAlarm(1);
//   }

//   pinMode(SD_PWR_EN, OUTPUT);
//   digitalWrite(SD_PWR_EN, LOW);
//   delay(5000);

// state = digitalRead(25);  // re-read again
// if (state == HIGH) {
//   Serial.print("1GPIO25 is HIGH, state = ");
//   Serial.println(state);
// } else {
//   Serial.print("1GPIO25 is LOW, state = ");
//   Serial.println(state);
// }

//   digitalWrite(SD_PWR_EN, HIGH);
//   delay(5000);

//   //int state = digitalRead(25);
// state = digitalRead(25);  // re-read again
// if (state == HIGH) {
//   Serial.print("2GPIO25 is HIGH, state = ");
//   Serial.println(state);
// } else {
//   Serial.print("2GPIO25 is LOW, state = ");
//   Serial.println(state);
// }


//   SPI.begin(18, 19, 23, SD_CS);
//   if (!SD.begin(SD_CS)) {
//     Serial.println("❌ SD card initialization failed!");
//     while (true)
//       ;
//   }
//   Serial.println("✅ SD card ready");

//   DateTime now = rtc.now();
//   int minute = now.minute();
//   Serial.print("⏰ Current time: ");
//   Serial.println(now.timestamp(DateTime::TIMESTAMP_TIME));

//   //analogReadResolution(12);  // ESP32 supports 12-bit ADC
//   //delay(100);

//   float batteryVoltage = readBatteryVoltage();
//   Serial.print("🔋 Battery Voltage: ");
//   Serial.print(batteryVoltage);
//   Serial.println(" V");

//   //shedule jobs
//   DateTime nextAlarm;

//   if (minute == readMinute1) {
//     logWaterHeight();  // First reading
//     delay(200);
//     readAndLogTDS();
//     delay(200);
//     updBATtoSDcard();
//     delay(200);
//     PhReadings();
//     nextAlarm = now + TimeSpan(0, 0, waitAfterRead1, 0);

//   } else if (minute == readMinute2) {
//     logWaterHeight();  // Second reading
//     delay(200);
//     readAndLogTDS();
//     delay(200);
//     updBATtoSDcard();
//     delay(200);
//     PhReadings();
//     nextAlarm = now + TimeSpan(0, 0, waitAfterRead2, 0);

//   } else if (minute == sendMinute) {
//     Serial.println("📤 Transmitting data to gateway...");
//     //transmitToGateway();
//     logWaterHeight();
//     nextAlarm = now + TimeSpan(0, 0, waitAfterSend, 0);

//   } else if (minute < readMinute1) {
//     nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), readMinute1, 0);

//   } else if (minute < readMinute2) {
//     nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), readMinute2, 0);

//   } else if (minute < sendMinute) {
//     nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), sendMinute, 0);

//   } else {
//     nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour() + 1, readMinute1, 0);
//   }

//   // ==== Set RTC Alarm ====
//   if (!rtc.setAlarm1(nextAlarm, DS3231_A1_Minute)) {
//     Serial.println("❌ Failed to set RTC alarm");
//   } else {
//     Serial.print("🔔 Next alarm set for: ");
//     Serial.println(nextAlarm.timestamp(DateTime::TIMESTAMP_TIME));
//   }

//   // ==== Sleep ====
//   esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);  // SQW falling edge
//   delay(2000);
//   digitalWrite(SD_PWR_EN, LOW);
//   delay(5000);
//   // Set GPIO low
//   //gpio_hold_en(GPIO_NUM_25);  // Hold this state
//   //delay(500);
//   //gpio_deep_sleep_hold_en();

//   //int state = digitalRead(25);
// state = digitalRead(25);  // re-read again
// if (state == HIGH) {
//   Serial.print("3GPIO25 is HIGH, state = ");
//   Serial.println(state);
// } else {
//   Serial.print("3GPIO25 is LOW, state = ");
//   Serial.println(state);
// }

//   delay(5000);
//   Serial.println("😴 Mosfet switched off...");
//   esp_deep_sleep_start();

// state = digitalRead(25);  // re-read again
// if (state == HIGH) {
//   Serial.print("4GPIO25 is HIGH, state = ");
//   Serial.println(state);
// } else {
//   Serial.print("4GPIO25 is LOW, state = ");
//   Serial.println(state);
// }
//   //esp_light_sleep_start();
// }

// void loop() {
//   // Not used due to deep sleep
// }


//------------------------------------------

// //------------light sleep with loop--------------------
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "RTClib.h"
#include "WaterHeightLogger.h"
#include "LoRaSender.h"
#include "TdsReadings.h"
#include "BatteryReader.h"
#include "PhReadings.h"

const int nodeID = 2;
File logFile;

#define SD_CS 4
#define SD_PWR_EN 25
#define RTC_INT_PIN 35  // RTC INT pin
#define uS_TO_S_FACTOR 1000000ULL

// === Configurable Job Times ===
const int readMinute1 = 15;
const int readMinute2 = 45;
const int sendMinute  = 59;

MPU6050 mpu(0x69);
RTC_DS3231 rtc;
RTC_DATA_ATTR int lastExecutedMinute = -1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);
  Wire.setClock(400000);

  if (!rtc.begin()) {
    Serial.println("❌ RTC not found!");
    while (true);
  }

  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC lost power, resetting time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  rtc.disable32K();
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.disableAlarm(2);
  pinMode(RTC_INT_PIN, INPUT_PULLUP);

  pinMode(SD_PWR_EN, OUTPUT);
  SPI.begin(18, 19, 23, SD_CS);

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("🌞 Woke from RTC alarm");
    rtc.clearAlarm(1);
  } else {
    Serial.println("🔁 Power-on or manual reset");
  }
}

void loop() {
  delay(1000);  // Ensure RTC time is updated after wakeup
  Serial.println("🔄 New cycle started");

  DateTime now = rtc.now();
  int minute = now.minute();
  Serial.print("🕒 Current Time: ");
  Serial.println(now.timestamp(DateTime::TIMESTAMP_TIME));

  if ((minute == readMinute1 || minute == readMinute2 || minute == sendMinute) && minute != lastExecutedMinute) {
    // === MOSFET ON ===
    digitalWrite(SD_PWR_EN, HIGH);
    Serial.println("🔌 MOSFET ON");
    delay(1000);

    SD.end();
    if (!SD.begin(SD_CS)) {
      Serial.println("❌ SD card init failed!");
    } else {
      Serial.println("✅ SD card ready");
    }

    if (minute == readMinute1 || minute == readMinute2) {
      Serial.println("📊 Running Job 1 (Sensor logging)");
      logWaterHeight();
      readAndLogTDS();
      updBATtoSDcard();
      PhReadings();
    } else if (minute == sendMinute) {
      Serial.println("📤 Running Job 2 (Transmit data)");
      transmitToGateway();
    }

    digitalWrite(SD_PWR_EN, LOW);
    Serial.println("⚡ MOSFET OFF");
    delay(500);

    lastExecutedMinute = minute;
  } else {
    Serial.println("⏭ No job scheduled at this minute or already executed");
  }

  DateTime nextAlarm;
  if (minute < readMinute1) {
    nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), readMinute1, 0);
  } else if (minute < readMinute2) {
    nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), readMinute2, 0);
  } else if (minute < sendMinute) {
    nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour(), sendMinute, 0);
  } else {
    nextAlarm = DateTime(now.year(), now.month(), now.day(), now.hour() + 1, readMinute1, 0);
  }

  if (!rtc.setAlarm1(nextAlarm, DS3231_A1_Minute)) {
    Serial.println("❌ Failed to set RTC alarm");
  } else {
    Serial.print("🔔 Next alarm set for: ");
    Serial.println(nextAlarm.timestamp(DateTime::TIMESTAMP_TIME));
  }

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  Serial.println("😴 Entering light sleep until RTC alarm...");
  delay(100);
  Serial.flush();
  esp_light_sleep_start();
}

//------------check nebhour-------------
//------------loranebhour

// #include <Wire.h>
// #include <SPI.h>
// #include <SD.h>
// #include <LoRa.h>
// #include <RTClib.h>
// #include "LoRaNeighbour.h"

// // === LoRa Pin Configuration ===
// #define LORA_SS 5
// #define LORA_RST 16
// #define LORA_DIO0 2

// // === Global External Variables for LoRaNeighbour.cpp ===
// RTC_DS3231 rtc;
// String NODE_ID = "1";  // Set uniquely for each node: "1", "2", "3", etc.
// File logFile;

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   // === RTC Setup ===
//   Wire.begin(21, 22);  // SDA, SCL
//   if (!rtc.begin()) {
//     Serial.println("❌ RTC not found!");
//     while (1)
//       ;
//   }

//   if (rtc.lostPower()) {
//     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC on first boot
//     Serial.println("⚠️ RTC time adjusted from compile time");
//   }


//   DateTime now = rtc.now();
//   int minute = now.minute();
//   Serial.print("🕒 Current Time: ");
//   Serial.println(now.timestamp(DateTime::TIMESTAMP_TIME));


//   //listenForTimeSync();
//   //delay(500);
//   //runNeighborDiscovery();
//   //delay(60000);
//   listenAndLogPingsOnly();
//   // === Run Neighbor Discovery ===
//   //runNeighborDiscovery();  // Will send PING, listen, respond, and log RSSI/SNR
// }

// void loop() {
//   // Nothing here – one-time test
//   DateTime now = rtc.now();
//   int minute = now.minute();
//   Serial.print("🕒 Current Time: ");
//   Serial.println(now.timestamp(DateTime::TIMESTAMP_TIME));
//   delay(10000);
// }