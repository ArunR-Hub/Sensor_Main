#ifndef LORA_NEIGHBOUR_H
#define LORA_NEIGHBOUR_H

void runNeighborDiscovery();
void listenForTimeSync();
void listenAndLogPingsOnly();
void LoraNodeDiscovery();
void evaluateLoRaNeighbours();

void saveOrUpdateLine(String line); // Optional: if used from outside
String extractValue(String line, String key); // Optional

#endif
