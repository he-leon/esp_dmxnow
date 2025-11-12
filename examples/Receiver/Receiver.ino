/*
 * DMX_ESPNOW Receiver Example
 * 
 * This example shows how to receive DMX data and display it on Serial.
 */

#include <DMX_ESPNOW.h>

DMX_ESPNOW dmx;

// This function is called when a complete DMX frame is received
void onDMXFrame() {
  // Read some channels
  uint8_t ch0 = dmx.getChannel(0);
  uint8_t ch1 = dmx.getChannel(1);
  uint8_t ch2 = dmx.getChannel(2);
  
  // Do something with the data
  // For example: control LEDs, servos, relays, etc.
  
  // Print values (not every frame, that would be too fast)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    Serial.printf("Ch0: %3d | Ch1: %3d | Ch2: %3d\n", ch0, ch1, ch2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("DMX ESP-NOW Receiver Example");
  
  // Initialize as receiver on WiFi channel 1 (must match sender)
  if (!dmx.beginReceiver(1, onDMXFrame)) {
    Serial.println("Initialization failed!");
    while(1) delay(1000);
  }
  
  Serial.println("Receiver ready! Waiting for DMX data...");
}

void loop() {
  // Print statistics every 5 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats >= 5000) {
    lastStats = millis();
    dmx.printStatistics();
  }
  
  // Check for timeout
  if (millis() - dmx.getLastFrameTime() > 3000 && dmx.getLastFrameTime() > 0) {
    static unsigned long lastWarning = 0;
    if (millis() - lastWarning > 5000) {
      lastWarning = millis();
      Serial.println("WARNING: No data received for 3 seconds!");
    }
  }
  
  delay(100);
}
