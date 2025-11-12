/*
 * DMX_ESPNOW.cpp
 * 
 * DMX512 over ESP-NOW for ESP32
 * 
 * Copyright (c) 2024 Your Name
 * MIT License
 */

#include "DMX_ESPNOW.h"

DMX_ESPNOW* DMX_ESPNOW::instance = nullptr;

DMX_ESPNOW::DMX_ESPNOW() {
  memset(dmxData, 0, DMX_UNIVERSE_SIZE);
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

bool DMX_ESPNOW::beginSender(uint8_t channel) {
  isSender = true;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (channel > 0 && channel <= 13) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }
  
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
    return false;
  }
  
  Serial.println("DMX ESP-NOW Sender initialized");
  Serial.print("WiFi Channel: ");
  Serial.println(channel);
  return true;
}

bool DMX_ESPNOW::beginReceiver(uint8_t channel, DMXFrameCallback callback) {
  isSender = false;
  frameCallback = callback;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (channel > 0 && channel <= 13) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }
  
  Serial.println("DMX ESP-NOW Receiver initialized");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("WiFi Channel: ");
  Serial.println(channel);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
  
  esp_now_register_recv_cb(onDataRecvWrapper);
  
  return true;
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
  packet.startChannel = 170;
  packet.channelCount = 170;
  memcpy(packet.data, &dmxData[170], 170);
  esp_now_send(broadcastAddress, (uint8_t*)&packet, sizeof(packet));
  delayMicroseconds(500);
  
  // Packet 2: Channels 340-511
  packet.packetNum = 2;
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
    frameCallback();
  }
}
