#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "RTClib.h"
#include "LoRaNeighbour.h"

// === Pin Definitions ===
#define SD_CS 4
#define SD_PWR_EN 25
#define LORA_SS 5
#define LORA_RST 16
#define LORA_DIO0 2

extern String NODE_ID;
extern RTC_DS3231 rtc;
extern File logFile;
//----------------------------------------------
void listenForTimeSync() {
  const String SYNC_PREFIX = "SYNC:";
  const String NODE_ID = "1";  // Change this per sensor node

  // === Power ON SD card ===
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);
  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD card init failed");
    return;
  }
  Serial.println("✅ SD card ready");

  // === Initialize LoRa ===
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("📡 Listening for time sync...");

  // === Open Log File ===
  logFile = SD.open("/SyncAckLog.json", FILE_APPEND);
  if (!logFile) {
    Serial.println("❌ Failed to open /SyncAckLog.json");
    return;
  }

  unsigned long startTime = millis();
  bool syncMatched = false;

  while (millis() - startTime < 10000) {  // 10 seconds timeout
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String msg = "";
      while (LoRa.available()) {
        msg += (char)LoRa.read();
      }

      if (msg.startsWith(SYNC_PREFIX)) {
        Serial.println("⏰ Received: " + msg);

        int y, mo, d, h, m, s;
        char idBuf[10];
        int parsed = sscanf(msg.c_str(), "SYNC:%d,%d,%d,%d,%d,%d:%s", &y, &mo, &d, &h, &m, &s, idBuf);
        String targetID = String(idBuf);

        if (parsed == 7 && targetID == NODE_ID) {
          rtc.adjust(DateTime(y, mo, d, h, m, s));
          Serial.println("✅ RTC updated successfully");

          // === Send ACK back to gateway ===
          delay(100);
          String ackMsg = "ACK_SYNC:" + NODE_ID;
          LoRa.beginPacket();
          LoRa.print(ackMsg);
          LoRa.endPacket();
          Serial.println("📤 Sent ACK: " + ackMsg);

          // === Log the ACK to SD card ===
          DateTime now = rtc.now();
          int rssi = LoRa.packetRssi();
          float snr = LoRa.packetSnr();
          String logLine = "{\"T\":\"" + now.timestamp() + "\", \"To\":\"GATEWAY\", \"RSSI\":" + String(rssi) + ", \"SNR\":" + String(snr, 2) + ", \"ACK_SENT\":true}";
          logFile.println(logLine);

          syncMatched = true;
        } else {
          Serial.println("⚠️ Not my SYNC message (for " + targetID + ")");
        }

        break;  // Only process one packet
      }
    }
  }

  if (!syncMatched) {
    DateTime now = rtc.now();
    String timeoutLog = "{\"T\":\"" + now.timestamp() + "\", \"To\":\"GATEWAY\", \"ACK_SENT\":false, \"Reason\":\"TIMEOUT\"}";
    logFile.println(timeoutLog);
  }

  logFile.close();
  LoRa.end();
  digitalWrite(SD_PWR_EN, LOW);
  Serial.println("🛑 Time sync listener done");
}

//-----------------------------------------------

void runNeighborDiscovery() {
  const String BROADCAST_MSG = "PING:" + NODE_ID;
  const int NUM_MESSAGES = 10;
  const int MESSAGE_INTERVAL_MS = 500;  // wait between messages

  SPI.begin(18, 19, 23, SD_CS);

  // === Initialize LoRa ===
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("✅ LoRa ready");

  // === Send broadcast 10 times ===
  for (int i = 0; i < NUM_MESSAGES; i++) {
    LoRa.beginPacket();
    LoRa.print(BROADCAST_MSG);
    LoRa.endPacket();
    Serial.println("📡 Sent broadcast PING: " + BROADCAST_MSG + " (" + String(i + 1) + "/10)");
    delay(MESSAGE_INTERVAL_MS);
  }
}

//-----------------------------------

void listenAndLogPingsOnly() {
  const String PING_PREFIX = "PING:";
  const int LISTEN_TIMEOUT = 10000;

  // === Power ON SD ===
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);

  // === Initialize SPI and SD ===
  SPI.begin(18, 19, 23, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD card init failed");
    return;
  }
  Serial.println("✅ SD card ready");
  delay(500);

  // === Initialize LoRa ===
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("✅ LoRa ready");
  delay(300);
  //--------------------------------
  // Ensure file exists before opening for append
  if (!SD.exists("/LoRaNeighbour.json")) {
    Serial.println("ℹ️ /LoRaNeighbour.json not found. Creating...");
    File createFile = SD.open("/LoRaNeighbour.json", FILE_WRITE);
    if (createFile) {
      createFile.println();  // ensure physical write
      createFile.close();
      Serial.println("✅ File created.");
      delay(100);  // Let SD card flush
    } else {
      Serial.println("❌ Failed to create /LoRaNeighbour.json");
      digitalWrite(SD_PWR_EN, LOW);
      return;
    }
  }

  // Now open safely
  logFile = SD.open("/LoRaNeighbour.json", FILE_APPEND);
  DateTime now = rtc.now();

  if (logFile) {
    unsigned long startTime = millis();
    while (millis() - startTime < LISTEN_TIMEOUT) {
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        String msg = "";
        while (LoRa.available()) {
          msg += (char)LoRa.read();
        }

        if (msg.startsWith(PING_PREFIX)) {
          String senderID = msg.substring(PING_PREFIX.length());
          int rssi = LoRa.packetRssi();

          String jsonLine = "{\"T\":\"" + now.timestamp() + "\", \"From\":\"" + senderID + "\", \"RSSI\":" + rssi + "}";

          Serial.println("📥 " + jsonLine);
          logFile.println(jsonLine);
        }
      }
    }
    logFile.close();
    Serial.println("✅ Neighbor responder completed");
  } else {
    Serial.println("❌ Could not open /LoRaNeighbour.json");
  }
  delay(300);

  File readFile = SD.open("/LoRaNeighbour.json", FILE_READ);
  if (readFile) {
    while (readFile.available()) {
      Serial.println(readFile.readStringUntil('\n'));
    }
    readFile.close();
  } else {
    Serial.println("Failed to read /LoRaNeighbour.json");
  }

  Serial.println("📄 Listening complete. Log saved.");
}