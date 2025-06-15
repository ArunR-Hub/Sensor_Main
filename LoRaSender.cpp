// // #include <SPI.h>
// // #include <SD.h>
// // #include <LoRa.h>
// // #include "LoRaSender.h"

// // // SPI pins
// // #define SCK     18
// // #define MISO    19
// // #define MOSI    23

// // // LoRa pins
// // #define LORA_SS     5
// // #define LORA_RST    16
// // #define LORA_DIO0   2

// // // SD card
// // #define SD_CS       4
// // #define SD_PWR_EN   25

// // const String NODE_ID = "01";
// // #define MAX_RETRIES 3
// // #define ACK_TIMEOUT 2000

// // bool sendViaLoRaWithAck(String msg);

// // void transmitToGateway() {
// //   // Power ON SD card
// //   pinMode(SD_PWR_EN, OUTPUT);
// //   digitalWrite(SD_PWR_EN, HIGH);
// //   delay(300);

// //   SPI.begin(SCK, MISO, MOSI, SD_CS);

// //   if (!SD.begin(SD_CS, SPI)) {
// //     Serial.println("SD init failed");
// //     return;
// //   }
// //   Serial.println("SD init OK");

// //   LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
// //   if (!LoRa.begin(433E6)) {
// //     Serial.println("LoRa init failed");
// //     return;
// //   }
// //   Serial.println("LoRa init OK");

// //   LoRa.receive();

// //   while (true) {
// //     // Step 1: Read all lines from SD
// //     File dataFile = SD.open("/Readings.json", FILE_READ);
// //     if (!dataFile) {
// //       Serial.println("❌ Failed to open /Readings.json");
// //       return;
// //     }

// //     std::vector<String> lines;
// //     while (dataFile.available()) {
// //       lines.push_back(dataFile.readStringUntil('\n'));
// //     }
// //     dataFile.close();

// //     if (lines.empty()) {
// //       Serial.println("📂 No data left in /Readings.json");
// //       break;
// //     }

// //     // Step 2: Send first line only
// //     String firstLine = lines[0];
// //     String payload = NODE_ID + ":" + firstLine;

// //     if (sendViaLoRaWithAck(payload)) {
// //       Serial.println("✅ ACK received, removing line from file");

// //       // Step 3: Rewrite file excluding first line
// //       lines.erase(lines.begin());
// //       SD.remove("/Readings.json");
// //       delay(100); // File system settle delay
// //       File writeFile = SD.open("/Readings.json", FILE_WRITE);
// //       if (writeFile) {
// //         for (String line : lines) {
// //           writeFile.println(line);
// //         }
// //         writeFile.close();
// //       } else {
// //         Serial.println("❌ Failed to rewrite /Readings.json");
// //         break;
// //       }
// //     } else {
// //       Serial.println("⚠️ ACK not received — stopping to retry later");
// //       break;
// //     }

// //     delay(500);
// //   }

// //   Serial.println("📤 File transmission completed");
// // }

// // bool sendViaLoRaWithAck(String msg) {
// //   int retries = 0;
// //   bool ackReceived = false;

// //   while (retries < MAX_RETRIES && !ackReceived) {
// //     LoRa.beginPacket();
// //     LoRa.print(msg);
// //     LoRa.endPacket();
// //     Serial.println("📡 Sent: " + msg);

// //     unsigned long startTime = millis();
// //     while (millis() - startTime < ACK_TIMEOUT) {
// //       int packetSize = LoRa.parsePacket();
// //       if (packetSize > 0) {
// //         String incoming = "";
// //         while (LoRa.available()) {
// //           incoming += (char)LoRa.read();
// //         }

// //         if (incoming == ("ACK:" + NODE_ID)) {
// //           Serial.println("✅ ACK received");
// //           ackReceived = true;
// //           break;
// //         } else {
// //           Serial.println("❌ Unexpected reply: " + incoming);
// //         }
// //       }
// //     }

// //     if (!ackReceived) {
// //       retries++;
// //       Serial.println("🔁 Retry " + String(retries) + "/" + String(MAX_RETRIES));
// //     }
// //   }

// //   if (!ackReceived) {
// //     Serial.println("❌ Failed to receive ACK after retries");
// //   }

// //   return ackReceived;
// // }

// // ------------------------------------------------------------------

// #include <SPI.h>
// #include <SD.h>
// #include <LoRa.h>
// #include "LoRaSender.h"

// // SPI pins
// #define SCK     18
// #define MISO    19
// #define MOSI    23

// // LoRa pins
// #define LORA_SS     5
// #define LORA_RST    16
// #define LORA_DIO0   2

// // SD card
// #define SD_CS       4
// #define SD_PWR_EN   25

// // Node and destination configuration
// const String NODE_ID = "01";     // This sensor node ID
// const String GATEWAY_ID = "200"; // Gateway node ID

// #define MAX_RETRIES 3
// #define ACK_TIMEOUT 2000  // ms

// bool sendViaLoRaWithAck(String msg);

// void transmitToGateway() {
//   // Power ON SD card
//   pinMode(SD_PWR_EN, OUTPUT);
//   digitalWrite(SD_PWR_EN, HIGH);
//   delay(300);

//   SPI.begin(SCK, MISO, MOSI, SD_CS);

//   if (!SD.begin(SD_CS, SPI)) {
//     Serial.println("SD init failed");
//     return;
//   }
//   Serial.println("SD init OK");

//   LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
//   if (!LoRa.begin(433E6)) {
//     Serial.println("LoRa init failed");
//     return;
//   }
//   Serial.println("LoRa init OK");

//   LoRa.receive();

//   while (true) {
//     // Step 1: Read all lines from SD card
//     File dataFile = SD.open("/Readings.json", FILE_READ);
//     if (!dataFile) {
//       Serial.println("❌ Failed to open /Readings.json");
//       return;
//     }

//     std::vector<String> lines;
//     while (dataFile.available()) {
//       lines.push_back(dataFile.readStringUntil('\n'));
//     }
//     dataFile.close();

//     if (lines.empty()) {
//       Serial.println("📂 No data left in /Readings.json");
//       break;
//     }

//     // Step 2: Prepare and send first line to gateway
//     String firstLine = lines[0];
//     String payload = NODE_ID + "->" + GATEWAY_ID + ":" + firstLine;

//     if (sendViaLoRaWithAck(payload)) {
//       Serial.println("✅ ACK received, removing line from file");

//       // Step 3: Rewrite file excluding first line
//       lines.erase(lines.begin());
//       SD.remove("/Readings.json");
//       delay(100);
//       File writeFile = SD.open("/Readings.json", FILE_WRITE);
//       if (writeFile) {
//         for (String line : lines) {
//           writeFile.println(line);
//         }
//         writeFile.close();
//       } else {
//         Serial.println("❌ Failed to rewrite /Readings.json");
//         break;
//       }
//     } else {
//       Serial.println("⚠️ ACK not received — stopping to retry later");
//       break;
//     }

//     delay(500); // Wait before sending next
//   }

//   Serial.println("📤 File transmission completed");
// }

// bool sendViaLoRaWithAck(String msg) {
//   int retries = 0;
//   bool ackReceived = false;

//   while (retries < MAX_RETRIES && !ackReceived) {
//     LoRa.beginPacket();
//     LoRa.print(msg);
//     LoRa.endPacket();
//     Serial.println("📡 Sent: " + msg);

//     unsigned long startTime = millis();
//     while (millis() - startTime < ACK_TIMEOUT) {
//       int packetSize = LoRa.parsePacket();
//       if (packetSize > 0) {
//         String incoming = "";
//         while (LoRa.available()) {
//           incoming += (char)LoRa.read();
//         }

//         if (incoming == ("ACK:" + NODE_ID)) {
//           Serial.println("✅ ACK matched for node " + NODE_ID);
//           ackReceived = true;
//           break;
//         } else {
//           Serial.println("❌ Unexpected reply: " + incoming);
//         }
//       }
//     }

//     if (!ackReceived) {
//       retries++;
//       Serial.println("🔁 Retry " + String(retries) + "/" + String(MAX_RETRIES));
//     }
//   }

//   if (!ackReceived) {
//     Serial.println("❌ Failed to receive ACK after retries");
//   }

//   return ackReceived;
// }

//---------------------------------------------

#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "LoRaSender.h"

// -------------------- SPI Pin Configuration --------------------
#define SCK 18   // Serial Clock
#define MISO 19  // Master In Slave Out
#define MOSI 23  // Master Out Slave In

// -------------------- LoRa Module Pins --------------------
#define LORA_SS 5    // LoRa Chip Select
#define LORA_RST 16  // LoRa Reset
#define LORA_DIO0 2  // LoRa DIO0 interrupt pin

// -------------------- SD Card Configuration --------------------
#define SD_CS 4       // SD card Chip Select
#define SD_PWR_EN 25  // MOSFET switch to power ON/OFF SD card

// -------------------- Node & Gateway Configuration --------------------
const String NODE_ID = "02";      // Unique ID of this sender node
const String GATEWAY_ID = "200";  // Target Gateway node ID

// -------------------- LoRa Acknowledgment Settings --------------------
#define MAX_RETRIES 20     // Retry count if ACK is not received
#define ACK_TIMEOUT 3000  // Timeout in milliseconds to wait for ACK

// -------------------- Function Declaration --------------------
bool sendViaLoRaWithAck(String msg);

// ======================================================================
// Task: Transmit data from SD to Gateway via LoRa with ACK confirmation
// ======================================================================
void transmitToGateway() {
  // Task 1: Power ON SD card through MOSFET
  pinMode(SD_PWR_EN, OUTPUT);
  digitalWrite(SD_PWR_EN, HIGH);
  delay(300);  // Give SD card time to power up

  // Task 2: Initialize SPI for SD
  SPI.begin(SCK, MISO, MOSI, SD_CS);
   delay(300);

  // Task 3: Initialize SD card
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("❌ SD init failed");
    return;
  }
  Serial.println("SD init OK");

  // Task 4: Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    return;
  }
  Serial.println("LoRa init OK");



  LoRa.receive();  // Enable LoRa receive mode for ACKs

  // Task 5: Loop to process each line in the SD file
  while (true) {
    // Step 5.1: Open the Readings file
    File dataFile = SD.open("/Readings.json", FILE_READ);
    if (!dataFile) {
      Serial.println("❌ Failed to open /Readings.json");
      return;
    }

    // Step 5.2: Read all lines into memory
    std::vector<String> lines;
    while (dataFile.available()) {
      lines.push_back(dataFile.readStringUntil('\n'));
    }
    dataFile.close();

    // Step 5.3: Stop if no lines to send
    if (lines.empty()) {
      Serial.println("📂 No data left in /Readings.json");
      break;
    }

    // Step 5.4: Transmit first line with ACK check
    String firstLine = lines[0];
    String payload = NODE_ID + "->" + GATEWAY_ID + ":" + firstLine;

    if (sendViaLoRaWithAck(payload)) {
      Serial.println("✅ ACK received, removing line from file");

      // Step 5.5: Remove first line from memory
      lines.erase(lines.begin());

      // Step 5.6: Delete and recreate file with remaining lines
      if (SD.exists("/Readings.json")) {
        SD.remove("/Readings.json");
        delay(300);  // Ensure SD file system stabilizes
      }

      // Step 5.7: Attempt to recreate file and write remaining lines
      File writeFile;
      int retry = 0;
      while (!writeFile && retry < 3) {
        writeFile = SD.open("/Readings.json", FILE_WRITE);
        if (!writeFile) {
          Serial.println("⚠️ Retry opening /Readings.json for writing...");
          delay(200);
          retry++;
        }
      }

      if (writeFile) {
        for (String line : lines) {
          writeFile.println(line);
        }
        writeFile.close();
      } else {
        Serial.println("❌ Failed to rewrite /Readings.json after retries");
        break;
      }

    } else {
      // If ACK not received, stop and retry on next wake cycle
      Serial.println("⚠️ ACK not received — stopping to retry later");
      break;
    }

    delay(500);  // Optional: pacing between transmissions
  }

  Serial.println("📤 File transmission completed");
  digitalWrite(SD_PWR_EN, LOW);
}

// ======================================================================
// Task: Send data via LoRa and wait for ACK:01
// ======================================================================
bool sendViaLoRaWithAck(String msg) {
  int retries = 0;
  bool ackReceived = false;

  while (retries < MAX_RETRIES && !ackReceived) {
    // Step 1: Send the LoRa message
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();
    Serial.println("📡 Sent: " + msg);

    // Step 2: Wait for ACK response
    unsigned long startTime = millis();
    while (millis() - startTime < ACK_TIMEOUT) {
      int packetSize = LoRa.parsePacket();
      if (packetSize > 0) {
        String incoming = "";
        while (LoRa.available()) {
          incoming += (char)LoRa.read();
        }

        // Step 3: Validate ACK content
        if (incoming == ("ACK:" + NODE_ID)) {
          Serial.println("✅ ACK matched for node " + NODE_ID);
          ackReceived = true;
          break;
        } else {
          Serial.println("❌ Unexpected reply: " + incoming);
        }
      }
    }

    // Step 4: Retry if ACK not received
    if (!ackReceived) {
      retries++;
      Serial.println("🔁 Retry " + String(retries) + "/" + String(MAX_RETRIES));
    }
  }

  if (!ackReceived) {
    Serial.println("❌ Failed to receive ACK after retries");
  }

  return ackReceived;
}


