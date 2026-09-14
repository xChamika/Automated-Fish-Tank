#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6hDqZJ6Eu"
#define BLYNK_TEMPLATE_NAME "Automated Fish Tank"
#define BLYNK_AUTH_TOKEN "AUNwpCmWyuWY4YzMG1XIKC-hM2fT6QnY"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <Preferences.h>

// --- Pin Definitions ---
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int ONE_WIRE_BUS = 4;
const int SERVO_PIN = 13;
const int BUZZER_PIN = 15;
const int BUTTON_PIN = 14;

// --- OLED Display Specs (0.91" 128x32) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Custom Bitmaps (16x width-padded) ---
const unsigned char wifi_icon[] PROGMEM = {
  0x0F, 0xF0, 0x30, 0x0C, 0x42, 0x42, 0x84, 0x21,
  0x00, 0x00, 0x07, 0xE0, 0x18, 0x18, 0x00, 0x00,
  0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00
};

const unsigned char cloud_icon[] PROGMEM = {
  0x00, 0x00, 0x03, 0xC0, 0x0C, 0x30, 0x10, 0x08,
  0x20, 0x04, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char temp_icon[] PROGMEM = {
  0x03, 0xC0, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20,
  0x04, 0x20, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70,
  0x0E, 0x70, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
  0x0E, 0x70, 0x04, 0x20
};

const unsigned char level_icon[] PROGMEM = {
  0x01, 0x80, 0x03, 0xC0, 0x03, 0xC0, 0x07, 0xE0,
  0x07, 0xE0, 0x0F, 0xF0, 0x0F, 0xF0, 0x1F, 0xF8,
  0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF0, 0x0F, 0xF0,
  0x07, 0xE0, 0x03, 0xC0
};

const unsigned char limits_icon[] PROGMEM = {
  0x00, 0x00, 0x7E, 0x7E, 0x18, 0x18, 0x18, 0x18,
  0x3C, 0x3C, 0x18, 0x18, 0x00, 0x00, 0xDB, 0xDB,
  0x18, 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x18,
  0x7E, 0x7E, 0x00, 0x00
};

const unsigned char feed_icon[] PROGMEM = {
  0x07, 0xE0, 0x18, 0x18, 0x20, 0x04, 0x47, 0x82,
  0x44, 0x42, 0x44, 0x42, 0x47, 0x82, 0x40, 0x02,
  0x40, 0x02, 0x20, 0x04, 0x18, 0x18, 0x07, 0xE0,
  0x00, 0x00
};

// --- Constants & Tank Specs ---
const float TANK_MAX_CM = 30.0;
const float TANK_MIN_CM = 5.0;  
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long NETWORK_CHECK_INTERVAL = 10000; 
const unsigned long PAGE_INTERVAL = 5000; // 5 seconds per slide
const unsigned long CONFIG_NOTICE_DURATION = 1500; // 1.5s display confirmation

// --- Globals ---
char auth[] = "AUNwpCmWyuWY4YzMG1XIKC-hM2fT6QnY"; 
char ssid[] = "Chamika Nimsara";
char pass[] = "gggggggg";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo feederServo;
Preferences prefs;

// Runtime variables
float waterTemp = 0.0;
int waterLevelPct = 0;
float tempLow = 22.0, tempHigh = 28.0;
int levelLow = 20, levelHigh = 95;
unsigned long feedIntervalHrs = 12; // Default auto-feed interval in hours
unsigned long lastFeedTime = 0;

unsigned long lastSensorMillis = 0;
unsigned long lastNetworkMillis = 0;
unsigned long lastPageMillis = 0;
unsigned long configChangeMillis = 0;
bool configJustUpdated = false;
String configMsgText = "";

int displayPage = 0; // 0: Temp, 1: Level, 2: Limits, 3: Network, 4: Interval

bool lastButtonState = HIGH;
bool buttonState = HIGH; 
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

bool tempAlarmActive = false; 
bool levelAlarmActive = false; 

// --- Function Declarations ---
void readSensors();
void updateDisplay();
void checkAlarms();
void triggerFeed();
void loadConfig();
void saveConfig();
void handleNetwork();
void checkAutoFeed();

// Blynk sync handlers
BLYNK_CONNECTED() {
  Serial.println("[Blynk] Connected to server! Syncing virtual pins...");
  Blynk.syncVirtual(V3, V4, V5, V6, V7);
}

BLYNK_WRITE(V2) {
  if (param.asInt() == 1) {
    Serial.println("[Blynk] Manual feed triggered via App!");
    triggerFeed();
  }
}

BLYNK_WRITE(V3) { 
  tempLow = param.asFloat(); 
  saveConfig(); 
  displayPage = 2; 
  configJustUpdated = true;
  configChangeMillis = millis();
  configMsgText = "TmpLow -> " + String((int)tempLow) + "C";
  updateDisplay(); 
  lastPageMillis = millis(); 
}

BLYNK_WRITE(V4) { 
  tempHigh = param.asFloat(); 
  saveConfig(); 
  displayPage = 2; 
  configJustUpdated = true;
  configChangeMillis = millis();
  configMsgText = "TmpHigh -> " + String((int)tempHigh) + "C";
  updateDisplay(); 
  lastPageMillis = millis(); 
}

BLYNK_WRITE(V5) { 
  levelLow = param.asInt(); 
  saveConfig(); 
  displayPage = 2; 
  configJustUpdated = true;
  configChangeMillis = millis();
  configMsgText = "LvlLow -> " + String(levelLow) + "%";
  updateDisplay(); 
  lastPageMillis = millis(); 
}

BLYNK_WRITE(V6) { 
  levelHigh = param.asInt(); 
  saveConfig(); 
  displayPage = 2; 
  configJustUpdated = true;
  configChangeMillis = millis();
  configMsgText = "LvlHigh -> " + String(levelHigh) + "%";
  updateDisplay(); 
  lastPageMillis = millis(); 
}

BLYNK_WRITE(V7) { 
  feedIntervalHrs = param.asInt(); 
  if(feedIntervalHrs < 1) feedIntervalHrs = 1; 
  saveConfig(); 
  displayPage = 4; 
  configJustUpdated = true;
  configChangeMillis = millis();
  configMsgText = "Feed -> " + String(feedIntervalHrs) + " hrs";
  updateDisplay(); 
  lastPageMillis = millis(); 
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(21, 22);
  
  // --- Startup Animation Screen ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("[Hardware] SSD1306 OLED failed"));
  } else {
    for (int i = 0; i <= 100; i += 25) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);    
      display.setCursor(28, 4);  
      display.println(F("Tank Init"));
      display.drawRect(14, 18, 100, 8, SSD1306_WHITE);
      display.fillRect(16, 20, map(i, 0, 100, 0, 96), 4, SSD1306_WHITE);
      display.display();
      delay(120);
    }
  }

  sensors.begin();
  feederServo.attach(SERVO_PIN);
  feederServo.write(0); 

  loadConfig();
  lastFeedTime = millis();
  lastPageMillis = millis();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  
  Blynk.config(auth);
}

void loop() {
  // 1. HIGHEST PRIORITY: Physical Button (Runs continuously)
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        Serial.println("[Button] Physical feed button pressed!");
        triggerFeed();
      }
    }
  }
  lastButtonState = reading;

  // Check Automatic Feeding Schedule
  checkAutoFeed();

  // Check Wi-Fi + Blynk Connection Transition
  static bool prevOnline = false;
  bool currentOnline = (WiFi.status() == WL_CONNECTED && Blynk.connected());
  if (!prevOnline && currentOnline) {
    configJustUpdated = true;
    configChangeMillis = millis();
    configMsgText = "WiFi & Blynk Online";
    updateDisplay();
    lastPageMillis = millis();
  }
  prevOnline = currentOnline;

  // 2. HIGH PRIORITY: Sensors, Alarms, and OLED (Runs every 2 seconds)
  if (millis() - lastSensorMillis >= SENSOR_INTERVAL) {
    lastSensorMillis = millis();
    readSensors();
    if (!configJustUpdated || (millis() - configChangeMillis >= CONFIG_NOTICE_DURATION)) {
      updateDisplay();
    }
    checkAlarms();

    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, waterTemp);
      Blynk.virtualWrite(V1, waterLevelPct);
    }
  }

  // Handle Carousel Page Rotation (Every PAGE_INTERVAL = 5000ms across 5 slides, skip if notification active)
  if (millis() - lastPageMillis >= PAGE_INTERVAL) {
    lastPageMillis = millis();
    if (!configJustUpdated || (millis() - configChangeMillis >= CONFIG_NOTICE_DURATION)) {
      displayPage = (displayPage + 1) % 5;
      updateDisplay();
    }
  }

  // Clear expired notice state
  if (configJustUpdated && (millis() - configChangeMillis >= CONFIG_NOTICE_DURATION)) {
    configJustUpdated = false;
    updateDisplay();
  }

  // 3. LOW PRIORITY: Network Management (Runs every 10 seconds)
  if (millis() - lastNetworkMillis >= NETWORK_CHECK_INTERVAL) {
    lastNetworkMillis = millis();
    handleNetwork();
  }

  // 4. BLYNK PROCESSING: Only runs if connection is established
  if (Blynk.connected()) {
    Blynk.run();
  }
}

// --- Background Network Manager with 3s Timeout ---
void handleNetwork() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Network] Wi-Fi disconnected. Attempting to connect...");
    WiFi.begin(ssid, pass); 
  } else if (!Blynk.connected()) {
    Serial.println("[Network] Wi-Fi connected, but Blynk offline. Connecting...");
    Blynk.connect(3000); 
  }
}

void readSensors() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distanceCm = (duration * 0.0343) / 2.0;

  if (distanceCm > 0 && distanceCm <= 50) {
    waterLevelPct = constrain(map((long)(distanceCm * 10), (long)(TANK_MIN_CM * 10), (long)(TANK_MAX_CM * 10), 100, 0), 0, 100);
  }

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    waterTemp = tempC;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Show temporary confirmation/connection overlay if active
  if (configJustUpdated && (millis() - configChangeMillis < CONFIG_NOTICE_DURATION)) {
    display.setTextSize(1);
    display.setCursor(20, 4);
    display.println(F("SYSTEM STATUS"));
    display.setCursor(10, 16);
    display.println(configMsgText);
    display.display();
    return;
  }

  switch (displayPage) {
    case 0: // Full-screen Temperature view with icon
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("WATER TEMP"));
      display.drawBitmap(2, 14, temp_icon, 16, 14, SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(20, 14);
      display.print(waterTemp, 1);
      display.print(F("C"));
      break;

    case 1: // Full-screen Level view with icon
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("WATER LEVEL"));
      display.drawBitmap(2, 14, level_icon, 16, 14, SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(20, 14);
      display.print(waterLevelPct);
      display.print(F("%"));
      break;

    case 2: // Full-screen Limits view with icon
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("LIMITS (L/H)"));
      display.drawBitmap(2, 13, limits_icon, 16, 14, SSD1306_WHITE);
      display.setCursor(20, 12);
      display.print(F("Tmp:")); display.print((int)tempLow); display.print("-"); display.println((int)tempHigh);
      display.setCursor(20, 22);
      display.print(F("Lvl:")); display.print(levelLow); display.print("-"); display.println(levelHigh);
      break;

    case 3: // Full-screen Network Status view (Wi-Fi Bitmap + Cloud Bitmap)
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("CONNECTIVITY"));
      
      display.drawBitmap(0, 13, wifi_icon, 16, 12, SSD1306_WHITE);
      display.setCursor(19, 15);
      display.print(WiFi.status() == WL_CONNECTED ? F("OK") : F("NO"));

      display.drawBitmap(64, 13, cloud_icon, 16, 12, SSD1306_WHITE);
      display.setCursor(83, 15);
      display.print(Blynk.connected() ? F("OK") : F("NO"));
      break;

    case 4: // Full-screen Feed Interval view
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("FEED INTERVAL"));
      display.drawBitmap(2, 14, feed_icon, 16, 14, SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(22, 14);
      display.print(feedIntervalHrs);
      display.setTextSize(1);
      display.setCursor(68, 20);
      display.print(F("hrs"));
      break;
  }
  
  display.display(); 
}

// --- Dual Alarm Logic ---
void checkAlarms() {
  bool isTempAlarm = false;
  bool isLevelAlarm = false;
  String tempMsg = "";
  String levelMsg = "";

  if (waterTemp < tempLow || waterTemp > tempHigh) {
    isTempAlarm = true;
    tempMsg = "Temp Alert: " + String(waterTemp) + "C";
  }
  if (waterLevelPct < levelLow || waterLevelPct > levelHigh) {
    isLevelAlarm = true;
    levelMsg = "Level Alert: " + String(waterLevelPct) + "%";
  }

  if (isTempAlarm || isLevelAlarm) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (isTempAlarm) {
    if (!tempAlarmActive) { 
      if (Blynk.connected()) Blynk.logEvent("temp_alert", tempMsg);
      tempAlarmActive = true; 
    }
  } else {
    tempAlarmActive = false; 
  }

  if (isLevelAlarm) {
    if (!levelAlarmActive) { 
      if (Blynk.connected()) Blynk.logEvent("level_alert", levelMsg);
      levelAlarmActive = true; 
    }
  } else {
    levelAlarmActive = false; 
  }
}

void checkAutoFeed() {
  unsigned long intervalMs = feedIntervalHrs * 3600000UL;
  if (feedIntervalHrs > 0 && (millis() - lastFeedTime >= intervalMs)) {
    lastFeedTime = millis();
    Serial.println("[AutoFeed] Scheduled interval reached. Feeding...");
    triggerFeed();
  }
}

void triggerFeed() {
  Serial.println("[Feeder] Activating servo motor with animation...");
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(24, 2);
  display.println(F("FEEDING FISH"));
  display.drawRect(18, 16, 92, 10, SSD1306_WHITE);
  display.fillRect(20, 18, 88, 6, SSD1306_WHITE);
  display.display();

  feederServo.write(75); 
  delay(400);
  feederServo.write(0);   
  
  lastFeedTime = millis();
  updateDisplay();         
}

void loadConfig() {
  prefs.begin("fishtank", true);
  tempLow = prefs.getFloat("tempLow", 22.0);
  tempHigh = prefs.getFloat("tempHigh", 28.0);
  levelLow = prefs.getInt("levelLow", 20);
  levelHigh = prefs.getInt("levelHigh", 95);
  feedIntervalHrs = prefs.getULong("feedHrs", 12);
  prefs.end();
}

void saveConfig() {
  prefs.begin("fishtank", false);
  prefs.putFloat("tempLow", tempLow);
  prefs.putFloat("tempHigh", tempHigh);
  prefs.putInt("levelLow", levelLow);
  prefs.putInt("levelHigh", levelHigh);
  prefs.putULong("feedHrs", feedIntervalHrs);
  prefs.end();
}