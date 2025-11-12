/*
 * DMX_ESPNOW Sender Example
 * 
 * This example shows how to send DMX data with automatic refresh rate.
 * Creates a simple fade animation on channel 0.
 */

#include <DMX_ESPNOW.h>

DMX_ESPNOW dmx;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("DMX ESP-NOW Sender Example");
  
  // Initialize as sender on WiFi channel 1
  if (!dmx.beginSender(1)) {
    Serial.println("Initialization failed!");
    while(1) delay(1000);
  }
  
  // Set automatic refresh rate to 50 Hz (typical DMX refresh rate)
  dmx.setRefreshRate(50);
  
  Serial.println("Sender ready! Broadcasting DMX data...");
}

void loop() {
  // Simple fade animation on channel 0
  static uint8_t brightness = 0;
  static int8_t direction = 1;
  
  // Set channel 0 to current brightness
  dmx.setChannel(0, brightness);
  
  // Set some other channels for testing
  dmx.setChannel(1, 255);       // Channel 1 always full
  dmx.setChannel(2, 128);       // Channel 2 always half
  dmx.setChannel(3, 255 - brightness);  // Channel 3 inverse of channel 0
  
  // Update brightness for fade effect
  brightness += direction * 5;
  if (brightness >= 250 || brightness <= 5) {
    direction *= -1;
  }
  
  // This handles automatic sending at the configured refresh rate
  dmx.update();
  
  // Optional: Print status every second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.printf("Sending DMX - Ch0: %3d, Ch3: %3d\n", dmx.getChannel(0), dmx.getChannel(3));
  }
}
