/*
 * DMX_ESPNOW.h
 * * DMX512 over ESP-NOW for ESP32
 * * Copyright (c) 2024 Your Name
 * MIT License
 */

#ifndef DMX_ESPNOW_H
#define DMX_ESPNOW_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define DMX_UNIVERSE_SIZE 512
#define ESPNOW_MAX_PAYLOAD 250
#define DMX_PACKET_COUNT 3

// Packet structure
typedef struct {
  uint8_t universeId;
  uint8_t packetNum;
  uint8_t sequenceNum;
  uint16_t startChannel;
  uint8_t channelCount;
  uint8_t data[239];
} DMXPacket;

// Callback function types
typedef void (*DMXFrameCallback)(uint8_t universeId);
typedef void (*DMXSendCallback)(bool success);

class DMX_ESPNOW {
public:
  DMX_ESPNOW();
  
  // Initialize as sender (Modified: Removed channel parameter)
  bool beginSender(uint8_t universeId);
  
  // Initialize as receiver (Modified: Removed channel parameter)
  bool beginReceiver(DMXFrameCallback callback = nullptr);
  
  // Utility: Set the universe ID this receiver instance should listen to
  void setReceiveUniverseId(uint8_t universeId);
  
  // Sender methods
  void setChannel(uint16_t channel, uint8_t value);
  void setChannels(uint16_t startChannel, uint8_t* values, uint16_t count);
  void sendFrame();
  void setRefreshRate(uint16_t hz);
  void update();
  
  // Receiver methods
  uint8_t getChannel(uint16_t channel);
  void getChannels(uint16_t startChannel, uint8_t* buffer, uint16_t count);
  void setFrameCallback(DMXFrameCallback callback);
  
  // Statistics
  uint32_t getFrameCount();
  uint32_t getLostFrames();
  float getLossRate();
  unsigned long getLastFrameTime();
  void printStatistics();
  void resetStatistics();
  
  // Utility
  uint8_t* getDMXBuffer();
  
private:
  uint8_t dmxData[DMX_UNIVERSE_SIZE];
  uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  
  // The ID of the universe this instance is sending/receiving
  uint8_t currentUniverseId;
  
  uint8_t frameSequence;
  bool isSender;
  
  // Sender variables
  uint16_t refreshRate;
  unsigned long lastSendTime;
  
  // Receiver variables
  uint8_t lastSequence;
  bool packetReceived[3];
  unsigned long lastFrameTime;
  uint32_t frameCount;
  uint32_t lostFrames;
  DMXFrameCallback frameCallback;
  
  // Static instance for callbacks
  static DMX_ESPNOW* instance;
  
  // Static callback wrappers
  static void onDataSentWrapper(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void onDataRecvWrapper(const uint8_t *mac, const uint8_t *incomingData, int len);
  
  // Internal methods
  void handleReceivedPacket(const DMXPacket* packet);
  void onFrameComplete();
};

#endif // DMX_ESPNOW_H
