
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "LoRaSender.h"
#include <RTClib.h>
extern RTC_DS3231 rtc;  // Declares the rtc object defined in main.ino


// -------------------- SPI and LoRa Pins --------------------
#define SCK 18
#define MISO 19
#define MOSI 23
#define LORA_SS 5
#define LORA_RST 16
#define LORA_DIO0 2

// -------------------- SD Card --------------------
#define SD_CS 4
#define SD_PWR_EN 25

// -------------------- Node IDs --------------------
extern String NODE_ID;
String PRIMARY_NODE = "200";

// -------------------- LoRa ACK Settings --------------------
#define MAX_RETRIES 20
#define ACK_TIMEOUT 3000

// -------------------- Function Prototypes --------------------
bool sendViaLoRaWithAck(String msg, String expectedAckNode);
String getBestFallbackNodeSmart();

void transmitToGateway() {
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);

  SPI.begin(SCK, MISO, MOSI, SD_CS);
  delay(300);

  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD init failed");
    return;
  }
  Serial.println("✅ SD init OK");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("✅ LoRa init OK");

  unsigned long startTime = millis();
  const unsigned long duration = 30000;  // 30 seconds limit

  while (millis() - startTime < duration) {
    File dataFile = SD.open("/Readings.json", FILE_READ);
    if (!dataFile) {
      Serial.println("❌ Failed to open /Readings.json");
      break;
    }

    std::vector<String> lines;
    while (dataFile.available()) {
      lines.push_back(dataFile.readStringUntil('\n'));
    }
    dataFile.close();

    if (lines.empty()) {
      Serial.println("📂 No data left in /Readings.json");
      delay(1000);  // Wait a bit before next check
      continue;     // Stay in loop to hold time slot
    }

    // if (lines.empty()) {
    //   Serial.println("📂 No data left in /Readings.json");
    //   break;
    // }

    String firstLine = lines[0];
    String primaryPayload = NODE_ID + "->" + PRIMARY_NODE + ":" + firstLine;

    bool ackReceived = false;

    // Attempt to send to gateway
    if (sendViaLoRaWithAck(primaryPayload, NODE_ID)) {
      Serial.println("✅ ACK from gateway");
      ackReceived = true;
    } else {
      Serial.println("⚠️ No ACK from gateway — trying fallback");

      String fallbackNode = getBestFallbackNodeSmart();
      if (fallbackNode != "") {
        String fallbackPayload = NODE_ID + "->" + fallbackNode + ":" + firstLine;
        if (sendViaLoRaWithAck(fallbackPayload, NODE_ID)) {
          Serial.println("✅ ACK from fallback node");
          ackReceived = true;
        } else {
          Serial.println("❌ Fallback also failed");
        }
      } else {
        Serial.println("❌ No fallback node found");
      }
    }

    if (ackReceived) {
      // Remove first line
      lines.erase(lines.begin());

      if (SD.exists("/Readings.json")) {
        SD.remove("/Readings.json");
        delay(200);
      }

      File writeFile = SD.open("/Readings.json", FILE_WRITE);
      if (writeFile) {
        for (String line : lines) {
          writeFile.println(line);
        }
        writeFile.close();
      } else {
        Serial.println("❌ Failed to write updated file");
      }

    } else {
      Serial.println("⚠️ Retaining data for next cycle");
      break;
    }

    // Exit if time exceeded
    if (millis() - startTime >= duration) {
      Serial.println("⏱️ Time up — exiting transmission");
      break;
    }

    delay(300);  // Small buffer
  }

  Serial.println("📤 Transmission ended (slot expired)");
  LoRa.sleep();
  digitalWrite(SD_PWR_EN, LOW);
}


// ======================================================================
// Send LoRa message and wait for ACK
// ======================================================================
bool sendViaLoRaWithAck(String msg, String expectedAckNode) {
  int retries = 0;
  bool ackReceived = false;

  while (retries < MAX_RETRIES && !ackReceived) {
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();
    Serial.println("📡 Sent: " + msg);

    unsigned long startTime = millis();
    while (millis() - startTime < ACK_TIMEOUT) {
      int packetSize = LoRa.parsePacket();
      if (packetSize > 0) {
        String incoming = "";
        while (LoRa.available()) {
          incoming += (char)LoRa.read();
        }

        if (incoming == ("ACK:" + expectedAckNode)) {
          Serial.println("✅ ACK matched for node " + expectedAckNode);
          ackReceived = true;
          break;
        } else {
          Serial.println("❌ Unexpected reply: " + incoming);
        }
      }
    }

    if (!ackReceived) {
      retries++;
      Serial.println("🔁 Retry " + String(retries) + "/" + String(MAX_RETRIES));
    }
  }

  return ackReceived;
}

// ======================================================================
// Find the best fallback node from /LoRaSummary.json based on SCORE
// ======================================================================

String getBestFallbackNodeSmart() {
  File summaryFile = SD.open("/LoRaSummary.json", FILE_READ);
  if (!summaryFile) {
    Serial.println("❌ Failed to open /LoRaSummary.json");
    return "";
  }

  struct Candidate {
    String viaNode;
    float score1toX;
    float scoreXto200;
  };

  std::vector<Candidate> candidates;
  std::vector<String> lines;

  // === Read all lines into memory
  while (summaryFile.available()) {
    String line = summaryFile.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) lines.push_back(line);
  }
  summaryFile.close();

  // === Step 1: Collect nodes this node (NODE_ID) can reach, excluding gateway
  for (String line : lines) {
    if (!line.startsWith("{N:" + NODE_ID + ",")) continue;

    int fIndex = line.indexOf("F:");
    int scoreIndex = line.indexOf("SCORE:");
    if (fIndex == -1 || scoreIndex == -1) continue;

    String toNode = line.substring(fIndex + 2, line.indexOf(",", fIndex));
    String scoreStr = line.substring(scoreIndex + 6, line.indexOf(",", scoreIndex) != -1 ? line.indexOf(",", scoreIndex) : line.indexOf("}", scoreIndex));
    toNode.trim();
    scoreStr.trim();
    if (toNode == "200") continue;

    float score1toX = scoreStr.toFloat();

    // Look for line N:toNode, F:200
    float scoreXto200 = 0;
    for (String subLine : lines) {
      if (!subLine.startsWith("{N:" + toNode + ",")) continue;
      int f2 = subLine.indexOf("F:");
      int s2 = subLine.indexOf("SCORE:");
      if (f2 == -1 || s2 == -1) continue;

      String fwd = subLine.substring(f2 + 2, subLine.indexOf(",", f2));
      String sVal = subLine.substring(s2 + 6, subLine.indexOf(",", s2) != -1 ? subLine.indexOf(",", s2) : subLine.indexOf("}", s2));
      fwd.trim();
      sVal.trim();

      if (fwd == "200") {
        scoreXto200 = sVal.toFloat();
        break;
      }
    }

    // Only consider if there's a known path from X to 200
    if (scoreXto200 > 0) {
      Candidate c = { toNode, score1toX, scoreXto200 };
      candidates.push_back(c);
    }
  }

  // === Step 2: Pick the node with best combined score
  float bestScore = 0;
  String bestNode = "";

  for (Candidate c : candidates) {
    float combined = c.score1toX + c.scoreXto200;
    Serial.println("🔁 Fallback via " + c.viaNode + " | S1toX: " + String(c.score1toX) + " + Xto200: " + String(c.scoreXto200) + " = " + String(combined));
    if (combined > bestScore) {
      bestScore = combined;
      bestNode = c.viaNode;
    }
  }

  return bestNode;
}

//----------reciever

void runLoRaReceiver() {
  Serial.println("📡 LoRa Receiver starting...");

  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);

  SPI.begin(SCK, MISO, MOSI, SD_CS);

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
  Serial.println("✅ LoRa ready");

  unsigned long startTime = millis();
  const unsigned long timeout = 30000;  // Listen for 30 sec

  while (millis() - startTime < timeout) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) continue;

    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    Serial.println("📨 Received: '" + incoming + "'");

    // Skip empty or whitespace-only packets
    bool allWhitespace = true;
    for (char c : incoming) {
      if (!isspace(c)) {
        allWhitespace = false;
        break;
      }
    }
    if (incoming.length() == 0 || allWhitespace) {
      Serial.println("⚠️ Ignored: empty/whitespace packet");
      continue;
    }

    int sepIndex = incoming.indexOf(':');
    if (sepIndex == -1) {
      Serial.println("❌ Ignored: No ':' in packet");
      continue;
    }

    String header = incoming.substring(0, sepIndex);
    String data = incoming.substring(sepIndex + 1);

    // Trim whitespace from both ends of data
    while (data.length() && isspace(data[0])) data.remove(0, 1);
    while (data.length() && isspace(data[data.length() - 1])) data.remove(data.length() - 1, 1);

    if (data.length() == 0) {
      Serial.println("⚠️ Ignored: JSON payload empty after trim");
      continue;
    }

    int arrowIndex = header.indexOf("->");
    if (arrowIndex == -1) {
      Serial.println("❌ Ignored: Invalid header");
      continue;
    }
    String senderID = header.substring(0, arrowIndex);
    String recipientID = header.substring(arrowIndex + 2);

    if (recipientID != NODE_ID) {
      Serial.println("❌ Ignored: Not for this node (" + NODE_ID + ")");
      continue;
    }

    File dataFile = SD.open("/Received.json", FILE_APPEND);
    if (dataFile) {
      dataFile.println(data);
      dataFile.close();
      Serial.println("✅ Saved to SD: " + data);
    } else {
      Serial.println("❌ Failed to write to SD");
    }

    String ackMsg = "ACK:" + senderID;
    LoRa.beginPacket();
    LoRa.print(ackMsg);
    LoRa.endPacket();
    Serial.println("✅ Sent ACK to " + senderID);
  }

  Serial.println("📴 LoRa receiver session done.");
}

//------------------------
void LoraTXGateway() {
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
      case 0: senderID = "1"; break;
      case 1: senderID = "2"; break;
      case 2:
        senderID = "3";
        break;
        // Add more slots if needed
    }

    if (NODE_ID == senderID) {
      Serial.println("📤 I am the sender for this slot");
      transmitToGateway();
    } else {
      Serial.println("👂 I will listen for this slot");
      runLoRaReceiver();
    }

    delay(1000);  // Optional short gap before next slot
  }
  digitalWrite(SD_PWR_EN, LOW);
  Serial.println("✅ 3-step Neighbor Discovery Complete");
}
