#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(21, 22); // SDA = 21, SCL = 22
  Serial.println("\n[I2C] Scanning for I2C devices...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("[I2C] Device found at address 0x%02X!\n", address);
      nDevices++;
    } else if (error == 4) {
      Serial.printf("[I2C] Unknown error at address 0x%02X\n", address);
    }
  }
  
  if (nDevices == 0) {
    Serial.println("[I2C] No I2C devices found. Check wiring!");
  } else {
    Serial.println("[I2C] Scan complete.\n");
  }
  
  delay(5000);
}