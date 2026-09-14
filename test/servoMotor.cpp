#include <Arduino.h>
#include <ESP32Servo.h>

// --- Pin Definitions ---
const int SERVO_PIN = 13;   // D13
const int SWITCH_PIN = 14;  // D14

Servo testServo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[Test] Starting Servo & Switch Test...");

  // Initialize Switch with internal pull-up
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // Initialize Servo
  testServo.attach(SERVO_PIN);
  testServo.write(0); // Start at rest position
  Serial.println("[Test] Ready. Press the micro switch to trigger servo.");
}

void loop() {
  // Read switch state (LOW when pressed due to INPUT_PULLUP and Ground connection)
  int switchState = digitalRead(SWITCH_PIN);

  if (switchState == LOW) {
    Serial.println("[Test] Micro switch pressed! Rotating servo...");
    
    // Sweep servo to feeding position (e.g., 90 degrees)
    testServo.write(90);
    delay(1000);
    
    // Return servo to resting position (0 degrees)
    testServo.write(0);
    delay(500); // Simple debounce delay to prevent rapid re-triggering
    
    Serial.println("[Test] Servo cycle complete. Waiting for next press...");
  }

  delay(50); // Small loop delay
}