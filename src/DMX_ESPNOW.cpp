/*
 * DMX_ESPNOW.cpp
 * * DMX512 over ESP-NOW for ESP32
 * * Copyright (c) 2024 Your Name
 * MIT License
 */

#include "DMX_ESPNOW.h"

DMX_ESPNOW* DMX_ESPNOW::instance = nullptr;

DMX_ESPNOW::DMX_ESPNOW() {
  memset(dmxData, 0, DMX_UNIVERSE_SIZE);
  currentUniverseId = 0; // Initialize to universe 0
  frameSequence = 0;
  isSender = false;
  refreshRate = 0;
  lastSendTime = 0;
  lastSequence = 255;
  lastFrameTime = 0;
  frameCount = 0;
  lostFrames = 0;
  frameCallback = nullptr;
  packetReceived[0] = packetReceived[1] = packetReceived[2] = false;
  instance = this;
}

// Modified: Removed channel parameter
bool DMX_ESPNOW::beginSender(uint8_t universeId) {
  currentUniverseId = universeId; // Store the desired universe ID
  isSender = true;
    
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
    
  esp_now_register_send_cb(onDataSentWrapper);
    
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
    
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
    
  Serial.printf("DMX ESP-NOW Sender initialized for Universe %d\n", currentUniverseId);
  Serial.print("WiFi Channel: ");
  Serial.println(WiFi.channel()); 
  return true;
}

// Modified: Removed channel parameter
bool DMX_ESPNOW::beginReceiver(DMXFrameCallback callback) {
  // currentUniverseId remains at its default (0) or previous set value
  isSender = false;
  frameCallback = callback;
    
  Serial.printf("DMX ESP-NOW Receiver initialized (Listening for Universe %d)\n", currentUniverseId);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("WiFi Channel: ");
  Serial.println(WiFi.channel()); 
    
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
    
  esp_now_register_recv_cb(onDataRecvWrapper);
    
  return true;
}

// Public setter for the receive universe ID
void DMX_ESPNOW::setReceiveUniverseId(uint8_t universeId) {
    currentUniverseId = universeId;
    // Resetting statistics is often good practice when changing the target stream
    resetStatistics(); 
    Serial.printf("DMX ESP-NOW Receiver now listening for Universe %d\n", currentUniverseId);
}


void DMX_ESPNOW::setChannel(uint16_t channel, uint8_t value) {
  if (channel < DMX_UNIVERSE_SIZE) {
    dmxData[channel] = value;
  }
}

void DMX_ESPNOW::setChannels(uint16_t startChannel, uint8_t* values, uint16_t count) {
  for (uint16_t i = 0; i < count && (startChannel + i) < DMX_UNIVERSE_SIZE; i++) {
    dmxData[startChannel + i] = values[i];
  }
}

void DMX_ESPNOW::sendFrame() {
  if (!isSender) return;
    
  DMXPacket packet;
  packet.universeId = currentUniverseId;
    
  // Packet 0: Channels 0-169
  packet.packetNum = 0;
  packet.sequenceNum = frameSequence;
  packet.startChannel = 0;
  packet.channelCount = 170;
  memcpy(packet.data, &dmxData[0], 170);
  esp_now_send(broadcastAddress, (uint8_t*)&packet, sizeof(packet));
  delayMicroseconds(500);
    
  // Packet 1: Channels 170-339
  packet.packetNum = 1;
  packet.sequenceNum = frameSequence;
  packet.startChannel = 170;
  packet.channelCount = 170;
  memcpy(packet.data, &dmxData[170], 170);
  esp_now_send(broadcastAddress, (uint8_t*)&packet, sizeof(packet));
  delayMicroseconds(500);
    
  // Packet 2: Channels 340-511
  packet.packetNum = 2;
  packet.sequenceNum = frameSequence;
  packet.startChannel = 340;
  packet.channelCount = 172;
  memcpy(packet.data, &dmxData[340], 172);
  esp_now_send(broadcastAddress, (uint8_t*)&packet, sizeof(packet));
    
  frameSequence++;
  lastSendTime = millis();
}

void DMX_ESPNOW::setRefreshRate(uint16_t hz) {
  refreshRate = hz;
}

void DMX_ESPNOW::update() {
  if (!isSender || refreshRate == 0) return;
    
  unsigned long interval = 1000 / refreshRate;
  if (millis() - lastSendTime >= interval) {
    sendFrame();
  }
}

uint8_t DMX_ESPNOW::getChannel(uint16_t channel) {
  if (channel < DMX_UNIVERSE_SIZE) {
    return dmxData[channel];
  }
  return 0;
}

void DMX_ESPNOW::getChannels(uint16_t startChannel, uint8_t* buffer, uint16_t count) {
  for (uint16_t i = 0; i < count && (startChannel + i) < DMX_UNIVERSE_SIZE; i++) {
    buffer[i] = dmxData[startChannel + i];
  }
}

void DMX_ESPNOW::setFrameCallback(DMXFrameCallback callback) {
  frameCallback = callback;
}

uint32_t DMX_ESPNOW::getFrameCount() {
  return frameCount;
}

uint32_t DMX_ESPNOW::getLostFrames() {
  return lostFrames;
}

float DMX_ESPNOW::getLossRate() {
  if (frameCount + lostFrames == 0) return 0.0;
  return (float)lostFrames / (frameCount + lostFrames) * 100.0;
}

unsigned long DMX_ESPNOW::getLastFrameTime() {
  return lastFrameTime;
}

void DMX_ESPNOW::printStatistics() {
  Serial.println("\n=== DMX Statistics ===");
  Serial.printf("Total Frames: %d\n", frameCount);
  Serial.printf("Lost Frames: %d\n", lostFrames);
  Serial.printf("Loss Rate: %.2f%%\n", getLossRate());
  if (lastFrameTime > 0) {
    Serial.printf("Last Frame: %lu ms ago\n", millis() - lastFrameTime);
  }
  Serial.println("===================\n");
}

void DMX_ESPNOW::resetStatistics() {
  frameCount = 0;
  lostFrames = 0;
}

uint8_t* DMX_ESPNOW::getDMXBuffer() {
  return dmxData;
}

void DMX_ESPNOW::onDataSentWrapper(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Optional: handle send status
}

void DMX_ESPNOW::onDataRecvWrapper(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (instance && len == sizeof(DMXPacket)) {
    instance->handleReceivedPacket((const DMXPacket*)incomingData);
  }
}

void DMX_ESPNOW::handleReceivedPacket(const DMXPacket* packet) {
  // Filter out packets that don't match our configured universe ID
  if (packet->universeId != currentUniverseId) {
    return;
  }
  
  if (packet->sequenceNum != lastSequence) {
    if (packet->packetNum == 0) {
      uint8_t expectedSeq = (lastSequence + 1) % 256;
      if (lastSequence != 255 && packet->sequenceNum != expectedSeq) {
        uint8_t missed = (packet->sequenceNum - expectedSeq + 256) % 256;
        lostFrames += missed;
      }
      lastSequence = packet->sequenceNum;
      packetReceived[0] = packetReceived[1] = packetReceived[2] = false;
    }
    // If packetNum is not 0, and sequenceNum is new, assume we missed packet 0 and discard this packet
    else if (packet->sequenceNum != lastSequence) {
        return;
    }
  }
    
  if (packet->packetNum < 3) {
    memcpy(&dmxData[packet->startChannel], packet->data, packet->channelCount);
    packetReceived[packet->packetNum] = true;
      
    if (packetReceived[0] && packetReceived[1] && packetReceived[2]) {
      onFrameComplete();
    }
  }
}

void DMX_ESPNOW::onFrameComplete() {
  frameCount++;
  lastFrameTime = millis();
    
  if (frameCallback) {
    // Pass the universe ID to the callback
    frameCallback(currentUniverseId); 
  }
}
