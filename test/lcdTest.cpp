#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 4 line display
LiquidCrystal_I2C lcd(0x27, 16, 4);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Wire.begin(21, 22); // SDA = 21, SCL = 22
  
  lcd.init();          // Initialize the lcd
  lcd.backlight();     // Turn on backlight
  
  lcd.setCursor(0, 0);
  lcd.print("Fish Tank Monitor");
  lcd.setCursor(0, 1);
  lcd.print("LCD Test: OK!");
  lcd.setCursor(0, 2);
  lcd.print("Address: 0x27");
  lcd.setCursor(0, 3);
  lcd.print("NodeMCU-32S Ready");
}

void loop() {
  // Do nothing, just keep display static
}