#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "Audio.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "Creds.h"

#define I2S_DOUT 12
#define I2S_BCLK 13
#define I2S_LRC 14

#define LCD_ADDRESS 0x3F
#define LCD_COLUMNS 16
#define LCD_ROWS 2

#define PIN_INPUT_1 4
#define PIN_INPUT_2 5
#define PIN_INPUT_3 0

WiFiMulti wifiMulti;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
Audio audio;

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("Parameters");

unsigned long lastPlayTime = 0;
const unsigned long timeoutDuration = 5000;
bool timeoutOccurred = false;
String currentTrack = "";
String type = "";

bool offlineMode = false;
unsigned long buttonPressStart = 0;
const unsigned long longPressDuration = 2000;
bool buttonPreviouslyPressed = false;
String lastLine1 = "";
String lastLine2 = "";

void logToLCD(const String &line1, const String &line2 = "") {
  if (line1 == lastLine1 && line2 == lastLine2)
    return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.length() > 16 ? line1.substring(0, 16) : line1);
  lcd.setCursor(0, 1);
  lcd.print(line2.length() > 16 ? line2.substring(0, 16) : line2);

  lastLine1 = line1;
  lastLine2 = line2;
}

void mainScreen() {
  logToLCD("Place object in", "front of sensor");
}

void logToInfluxDB(const String &track) {
  if (offlineMode) {
    Serial.println("Offline mode: Skipping InfluxDB log.");
    return;
  }

  if (track == "md.mp3") {
    type = "Metal";
  } else if (track == "nmd.mp3") {
    type = "Non-metal";
  }

  sensor.clearFields();
  sensor.clearTags();
  sensor.addField("Detection Type", type);

  if (!client.writePoint(sensor)) {
    String errMsg = client.getLastErrorMessage();
    Serial.print("InfluxDB write failed: ");
    Serial.println(errMsg);
    logToLCD("Influx error", errMsg.substring(0, 16));
  } else {
    Serial.println("Logged to InfluxDB: " + type);
  }

  type = "";
}

void listAllFiles() {
  Serial.println("Listing all files on LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("  - ");
    Serial.print(file.name());
    Serial.print("\t");
    Serial.print(file.size());
    Serial.println(" bytes");
    file = root.openNextFile();
  }
}

void toggleWiFi() {
  if (offlineMode) {
    Serial.println("Toggling Wi-Fi ON...");
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
    logToLCD("Connecting to", "WiFi...");

    unsigned long startAttempt = millis();
    while (wifiMulti.run() != WL_CONNECTED && millis() - startAttempt < 10000) {
      delay(200);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      logToLCD("WiFi connected", WiFi.SSID());
      Serial.println("\nWiFi connected.");
      if (client.validateConnection()) {
        logToLCD("Influx connected", "OK");
        Serial.println("InfluxDB connected.");
        offlineMode = false;
      } else {
        logToLCD("Influx failed", "Check serial");
        Serial.println("Influx connection failed.");
      }
    } else {
      logToLCD("WiFi Failed", "Offline mode");
      offlineMode = true;
      Serial.println("WiFi connection failed.");
    }
  } else {
    Serial.println("Toggling Wi-Fi OFF...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    offlineMode = true;
    logToLCD("Offline mode", "activated");
    delay(500);
  }

  delay(1500);
  mainScreen();
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed!");
    logToLCD("LittleFS", "Mount Failed!");
    return;
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(15);

  Wire.begin(8, 9);
  lcd.init();
  lcd.backlight();
  mainScreen();

  pinMode(PIN_INPUT_1, INPUT_PULLUP);
  pinMode(PIN_INPUT_2, INPUT_PULLUP);
  pinMode(PIN_INPUT_3, INPUT);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  logToLCD("Connecting to", "WiFi...");

  unsigned long startAttempt = millis();
  const unsigned long wifiTimeout = 10000;

  while (wifiMulti.run() != WL_CONNECTED && millis() - startAttempt < wifiTimeout) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    logToLCD("WiFi connected", WiFi.SSID());

    if (client.validateConnection()) {
      Serial.println("InfluxDB connected.");
      logToLCD("Influx connected", "OK");
    } else {
      Serial.println("InfluxDB connection failed.");
      logToLCD("Influx failed", "Check serial");
      offlineMode = true;
    }
  } else {
    Serial.println("\nWiFi connection timeout.");
    logToLCD("WiFi Timeout", "Offline mode");
    offlineMode = true;
  }

  delay(1500);
  mainScreen();
}

void loop() {
  bool buttonPressed = digitalRead(PIN_INPUT_3) == LOW;
  static unsigned long pressStart = 0;
  static bool longPressHandled = false;

  if (buttonPressed) {
    if (!buttonPreviouslyPressed) {
      pressStart = millis();
      buttonPreviouslyPressed = true;
      longPressHandled = false;
    } else if (!longPressHandled && (millis() - pressStart >= longPressDuration)) {
      toggleWiFi();
      longPressHandled = true;
    }
  } else if (buttonPreviouslyPressed) {

    if (!longPressHandled) {

      if (audio.isRunning()) {
        Serial.println("Button press: Stopping audio playback.");
        audio.stopSong();
        logToLCD("Playback stopped");
        lastPlayTime = millis();
        timeoutOccurred = false;
      } else {
        listAllFiles();
      }
    }

    buttonPreviouslyPressed = false;
    longPressHandled = false;
  }

  if (buttonPressed && (millis() - pressStart <= longPressDuration)) {

    return;
  }

  bool input1State = digitalRead(PIN_INPUT_1) == LOW;
  bool input2State = digitalRead(PIN_INPUT_2) == LOW;

  if (input1State && !input2State) {
    if (currentTrack != "nmd.mp3") {
      Serial.println("Non-metal detected.");
      logToLCD("Non-metal", "detected.");
      audio.connecttoFS(LittleFS, "/nmd.mp3");
      currentTrack = "nmd.mp3";
      lastPlayTime = millis();
      timeoutOccurred = false;
    }
  } else if (!input1State && input2State) {
    if (currentTrack != "md.mp3") {
      Serial.println("Metal detected.");
      logToLCD("Metal detected.");
      audio.connecttoFS(LittleFS, "/md.mp3");
      currentTrack = "md.mp3";
      lastPlayTime = millis();
      timeoutOccurred = false;
    }
  }

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.substring(0, 7).equalsIgnoreCase("volume ")) {
      int vol = input.substring(7).toInt();
      vol = constrain(vol, 0, 100);
      audio.setVolume(vol);
      Serial.println("Volume set to: " + String(vol));
      logToLCD("Volume set to:", String(vol));
    } else {

      String filepath = "/" + input;
      if (LittleFS.exists(filepath)) {
        Serial.println("Serial: Playing " + input);
        logToLCD("Serial command:", input);
        audio.connecttoFS(LittleFS, filepath.c_str());
        currentTrack = "";
      } else {
        Serial.println("Serial: File not found - " + filepath);
        logToLCD("File not found:", input);
        currentTrack = "";
      }
    }

    lastPlayTime = millis();
    timeoutOccurred = false;
  }

  audio.loop();

  if (!audio.isRunning() && currentTrack.length() > 0) {
    logToInfluxDB(currentTrack);
    currentTrack = "";
  }

  if (millis() - lastPlayTime >= timeoutDuration && !timeoutOccurred) {
    mainScreen();
    timeoutOccurred = true;
  }
}
