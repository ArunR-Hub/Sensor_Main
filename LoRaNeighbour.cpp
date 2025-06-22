#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "RTClib.h"
#include "LoRaNeighbour.h"
#include <ArduinoJson.h>

// === Pin Definitions ===
#define SD_CS 4
#define SD_PWR_EN 25
#define LORA_SS 5
#define LORA_RST 16
#define LORA_DIO0 2

extern String NODE_ID;
extern RTC_DS3231 rtc;
extern File logFile;

void listenForTimeSync() {
  const String SYNC_PREFIX = "SYNC:";
  extern String NODE_ID;

  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);
  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD card init failed");
    return;
  }
  Serial.println("✅ SD card ready");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("📡 Listening for time sync...");

  unsigned long startTime = millis();

  while (millis() - startTime < 60000) {  // Listen for 60 seconds
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
          DateTime before = rtc.now();
          Serial.println("🕒 Time before update: " + before.timestamp());

          rtc.adjust(DateTime(y, mo, d, h, m, s));

          DateTime after = rtc.now();
          Serial.println("🕒 Time after update:  " + after.timestamp());
          Serial.println("✅ RTC updated successfully");

          String ackMsg = "ACK_SYNC:" + NODE_ID;
          LoRa.beginPacket();
          LoRa.print(ackMsg);
          LoRa.endPacket();
          Serial.println("📤 Sent ACK: " + ackMsg);
        } else {
          Serial.println("⚠️ SYNC for another node (" + targetID + "), ignoring.");
        }
      }
    }
  }

  DateTime endTime = rtc.now();
  Serial.println("🛑 Time sync listener done at " + endTime.timestamp());

  LoRa.end();
  digitalWrite(SD_PWR_EN, LOW);
}


void runNeighborDiscovery() {
  const int MESSAGE_INTERVAL_MS = 500;
  const int TOTAL_DURATION_MS = 20000;
  const String BROADCAST_MSG = "PING:" + NODE_ID;

  // === Power ON SD ===
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);

  // === Initialize SPI and SD ===
  SPI.begin(18, 19, 23, SD_CS);
  delay(300);
  SD.end();
  delay(300);
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
  Serial.println("✅ LoRa ready");

  Serial.println("📡 Starting Neighbor Discovery...");

  unsigned long startTime = millis();
  bool summarySent = false;

  // === Repeatedly read and send summary lines until timeout ===
  while (millis() - startTime < TOTAL_DURATION_MS) {
    File summaryFile = SD.open("/LoRaSummary.json", FILE_READ);
    if (summaryFile) {
      while (summaryFile.available() && millis() - startTime < TOTAL_DURATION_MS) {
        String line = summaryFile.readStringUntil('\n');
        line.trim();
        if (line.length() < 10 || !line.startsWith("{")) continue;

        LoRa.beginPacket();
        //LoRa.print(line);
        LoRa.print(BROADCAST_MSG + "|" + line);
        LoRa.endPacket();
        Serial.println("📤 Sent summary: " + BROADCAST_MSG + "|" + line);
        delay(MESSAGE_INTERVAL_MS);

        summarySent = true;
      }
      summaryFile.close();
    } else {
      break; // Stop if file cannot be opened mid-loop
    }
  }

  // === If no summary line sent, fallback to broadcast PING ===
  if (!summarySent) {
    Serial.println("⚠️ No summary found — sending PING broadcast fallback...");

    unsigned long fallbackStart = millis();
    while (millis() - fallbackStart < TOTAL_DURATION_MS) {
      LoRa.beginPacket();
      LoRa.print(BROADCAST_MSG);
      LoRa.endPacket();

      Serial.println("📡 Sent fallback PING: " + BROADCAST_MSG);
      delay(MESSAGE_INTERVAL_MS);
    }
  }

  LoRa.sleep();
  SD.end();
  digitalWrite(SD_PWR_EN, LOW);
  Serial.println("🛑 Neighbor Discovery Broadcast Complete");
}


////-------------------------------------------------

void listenAndLogPingsOnly() {
  const int LISTEN_TIMEOUT = 20000;
  const float alpha = 0.7;
  const float beta = 0.3;

  // === Power ON SD ===
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(500);

  // === Initialize SPI and SD ===
  SPI.begin(18, 19, 23, SD_CS);
  SD.end();
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD card init failed");
    return;
  }
  Serial.println("✅ SD card ready");

  // === Ensure summary file exists ===
  if (!SD.exists("/LoRaSummary.json")) {
    File initFile = SD.open("/LoRaSummary.json", FILE_WRITE);
    if (initFile) {
      initFile.close();
      Serial.println("✅ Created /LoRaSummary.json");
    } else {
      Serial.println("❌ Failed to create /LoRaSummary.json");
      return;
    }
  }

  // === Initialize LoRa ===
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("📡 Listening for PINGs with summaries...");

  unsigned long startTime = millis();

  while (millis() - startTime < LISTEN_TIMEOUT) {
    int packetSize = LoRa.parsePacket();
    if (!packetSize) continue;

    String msg = "";
    while (LoRa.available()) msg += (char)LoRa.read();
    msg.trim();

    if (msg.startsWith("PING:") && msg.indexOf('|') != -1) {
      int sepIndex = msg.indexOf('|');
      String senderID = msg.substring(5, sepIndex);
      String summaryJson = msg.substring(sepIndex + 1);
      summaryJson.trim();

      // === Step 1: Log your own link to sender ===
      int rssi = LoRa.packetRssi();
      float snr = LoRa.packetSnr();
      DateTime now = rtc.now();

      float rssiNorm = constrain((rssi + 120.0) / 90.0, 0.0, 1.0);
      float snrNorm = constrain((snr + 20.0) / 30.0, 0.0, 1.0);
      float linkScore = alpha * rssiNorm + beta * snrNorm;

      String ownLine = "{N:" + NODE_ID + ",F:" + senderID +
                       ",RSSI:" + String(rssi) +
                       ",SNR:" + String(snr, 1) +
                       ",SCORE:" + String(linkScore, 3) +
                       ",T:\"" + now.timestamp() + "\"}";

      saveOrUpdateLine(ownLine);
      Serial.println("✅ Logged own link to sender: " + ownLine);

      // === Step 2: Save foreign summary if valid ===
      if (summaryJson.startsWith("{N:")) {
        saveOrUpdateLine(summaryJson);
        Serial.println("📥 Saved received summary: " + summaryJson);
      }

    } else if (msg.startsWith("PING:")) {
      Serial.println("⚠️ Received raw PING without summary: " + msg);
    }
  }


  File summaryFile = SD.open("/LoRaSummary.json", FILE_READ);
  if (!summaryFile) {
    Serial.println("❌ Failed to open /LoRaSummary.json for reading");
    return;
  }

  Serial.println("📊 --- LoRa Summary ---");
  while (summaryFile.available()) {
    String line = summaryFile.readStringUntil('\n');
    Serial.println(line);
  }
  summaryFile.close();

  Serial.println("📦 LoRa summary display complete.");

  // === Power OFF SD ===
  SD.end();
  digitalWrite(SD_PWR_EN, LOW);

  // === Print RTC system time when done ===
  DateTime done = rtc.now();
  Serial.println("📄 Listening complete at: " + done.timestamp());
  Serial.println("📘 Summary updated and SD powered down.");

}

void saveOrUpdateLine(String newLine) {
  File readFile = SD.open("/LoRaSummary.json", FILE_READ);
  String updatedContent = "";
  bool found = false;
  bool changed = true;

  String newN = extractValue(newLine, "N:");
  String newF = extractValue(newLine, "F:");

  if (readFile) {
    while (readFile.available()) {
      String line = readFile.readStringUntil('\n');
      line.trim();

      String lineN = extractValue(line, "N:");
      String lineF = extractValue(line, "F:");

      if (lineN == newN && lineF == newF) {
        found = true;
        if (line != newLine) {
          updatedContent += newLine + "\n";
        } else {
          updatedContent += line + "\n";
          changed = false;
        }
      } else if (line.length() > 5) {
        updatedContent += line + "\n";
      }
    }
    readFile.close();
  }

  if (!found) updatedContent += newLine + "\n";

  if (found && !changed) return;

  File writeFile = SD.open("/LoRaSummary.json", FILE_WRITE);
  if (writeFile) {
    writeFile.print(updatedContent);
    writeFile.close();
  } else {
    Serial.println("❌ Failed to write to SD");
  }
}


String extractValue(String line, String key) {
  int idx = line.indexOf(key);
  if (idx == -1) return "";
  int end = line.indexOf(",", idx);
  if (end == -1) end = line.indexOf("}", idx);
  return line.substring(idx + key.length(), end);
}


//-----------------------Combined funtion

void LoraNodeDiscovery() {
  DateTime startTime = rtc.now();
  int startSecondSlot = (startTime.minute() * 60 + startTime.second()) / 30;  // slot index (0–119)
  Serial.println("🚀 Starting 3-step Neighbor Discovery at " + startTime.timestamp());

  for (int i = 0; i < 3; i++) {
    int targetSlot = (startSecondSlot + i) % 120;  // 120 half-minute slots in an hour

    // Wait for correct 30-sec slot
    while (true) {
      DateTime now = rtc.now();
      int currentSlot = (now.minute() * 60 + now.second()) / 30;
      if (currentSlot == targetSlot) break;
      delay(500);  // Polling
    }

    Serial.println("⏱️ Running discovery step " + String(i + 1) + " at " + rtc.now().timestamp());

    String senderID;
    switch (i) {
      case 0: senderID = "200"; break;
      case 1: senderID = "1"; break;
      case 2:
        senderID = "2";
        break;
        // Add more slots if needed
    }

    if (NODE_ID == senderID) {
      Serial.println("📤 I am the sender for this slot");
      runNeighborDiscovery();
    } else {
      Serial.println("👂 I will listen for this slot");
      listenAndLogPingsOnly();
    }

    delay(1000);  // Optional short gap before next slot
  }
  digitalWrite(SD_PWR_EN, LOW);
  Serial.println("✅ 3-step Neighbor Discovery Complete");
}


////-------------------------------------

struct NeighborInfo {
  String nodeID;
  float score;
};

NeighborInfo sortedNeighbors[5];
int neighborCount = 0;

void evaluateLoRaNeighbours() {
  Serial.println("** Checking evaluateLoRaNeighbours **");

  String selfID = String(NODE_ID);
  String primaryNode = "None";
  String secondaryNode = "None";

  // === Power ON SD card ===
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(500);

  SPI.begin(18, 19, 23, SD_CS);
  SD.end();
  delay(300);
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card init failed");
    return;
  }
  delay(500);

  File file = SD.open("/LoRaSummary.json", FILE_READ);
  if (!file) {
    Serial.println("❌ Failed to open /LoRaSummary.json");
    return;
  }

  neighborCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) continue;

    String fromID = doc["From"];
    if (fromID == selfID) continue;

    int rssi = doc["RSSI"];
    float snr = doc["SNR"];
    float score = (rssi + 120) * 0.7 + snr * 0.3;

    if (neighborCount < 5) {
      sortedNeighbors[neighborCount++] = { fromID, score };
    }


    // int rssi = doc["RSSI"];
    // float snr = doc["SNR"];
    // float score = (rssi + 120) * 0.7 + snr * 0.3;

    // sortedNeighbors[neighborCount++] = {fromID, score};
    // if (neighborCount >= 5) break;  // Limit to 5 entries
  }
  file.close();

  // === Sort neighbors by descending score ===
  for (int i = 0; i < neighborCount - 1; i++) {
    for (int j = i + 1; j < neighborCount; j++) {
      if (sortedNeighbors[j].score > sortedNeighbors[i].score) {
        NeighborInfo temp = sortedNeighbors[i];
        sortedNeighbors[i] = sortedNeighbors[j];
        sortedNeighbors[j] = temp;
      }
    }
  }

  // === Select top neighbors ===
  if (neighborCount >= 1) primaryNode = sortedNeighbors[0].nodeID;
  if (neighborCount >= 2) secondaryNode = sortedNeighbors[1].nodeID;

  // === Print the results ===
  Serial.println("📶 Neighbor Ranking:");
  for (int i = 0; i < neighborCount; i++) {
    Serial.printf("  ➤ %s | Score: %.2f\n", sortedNeighbors[i].nodeID.c_str(), sortedNeighbors[i].score);
  }

  Serial.println("🔰 Selected Neighbors:");
  Serial.printf("✅ Primary   ➜ %s\n", primaryNode.c_str());
  Serial.printf("✅ Secondary ➜ %s\n", secondaryNode.c_str());

  // === Save to /RoutingPath.json (overwrite old) ===
  if (SD.exists("/RoutingPath.json")) {
    SD.remove("/RoutingPath.json");
  }

  File routeFile = SD.open("/RoutingPath.json", FILE_WRITE);
  if (!routeFile) {
    Serial.println("❌ Failed to create /RoutingPath.json");
    return;
  }

  StaticJsonDocument<128> outDoc;
  outDoc["Node"] = String(NODE_ID);
  outDoc["Primary"] = primaryNode;
  outDoc["Secondary"] = secondaryNode;

  serializeJson(outDoc, routeFile);
  routeFile.println();  // newline for safety
  routeFile.close();
  digitalWrite(SD_PWR_EN, LOW);
  Serial.println("💾 Routing path saved to /RoutingPath.json");
}
