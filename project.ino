#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// Use HardwareSerial for ESP32
HardwareSerial myHardwareSerial(1);  // Serial1 for DFPlayer
DFRobotDFPlayerMini myDFPlayer;

// Define pins
#define LED_PIN 12
#define RX_PIN 16         // DFPlayer TX
#define TX_PIN 17         // DFPlayer RX
#define IR_SENSOR_PIN 13  // IR sensor input
#define SCANNER_PIN 15    // Scanner control

LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD I2C address

bool isScanning = false;
bool coolDown = false;
unsigned long startScanTime = 0;
unsigned long coolDownStartTime = 0;
const unsigned long scanDuration = 5000;
const unsigned long coolDownDuration = 3000;
const unsigned long clearDelay = 20000;

// LCD display state
unsigned long displayStartTime = 0;
bool isDisplaying = false;

String inputString = "";
String DataScanner = "";
bool stringComplete = false;
bool isDFPlayerReady = false;  // เช็คสถานะ DFPlayer
byte volume = 23;

struct Medicine {
  String code;
  String name;
  int track;
};

// Medicine list
Medicine medicines[] = {
  { "001", "Paracetamol", 2 },
  { "002", "CHLORPHEN", 3 },
  { "003", "Belcid Forte", 4 },
  { "004", "Salol et Menthol Mixture", 5 },
  { "005", "Calamine", 6 },
  { "006", "HongThai", 7 },
  { "007", "Alcohol", 8 },
  { "008", "Normal Saline", 9 },
  { "009", "Apache", 10 },
  { "010", "Menstrual", 11 },
  { "011", "Kanolone", 12 }
};

void setup() {
  Serial.begin(9600);
  myHardwareSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);  // Initialize Serial1
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(SCANNER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.begin();
  lcd.backlight();

  inputString.reserve(200);
  digitalWrite(SCANNER_PIN, HIGH);

  if (!myDFPlayer.begin(myHardwareSerial)) {
    Serial.println(F("DFPlayer ไม่ตอบสนอง กำลังลองใหม่..."));
    delay(1000);
    if (!myDFPlayer.begin(myHardwareSerial)) {
      Serial.println(F("การเริ่มต้น DFPlayer ล้มเหลว กรุณาตรวจสอบการเชื่อมต่อ"));
      isDFPlayerReady = false;
    } else {
      isDFPlayerReady = true;
    }
  } else {
    isDFPlayerReady = true;
  }
}

void handleMedicine(String scannerData) {
  scannerData.trim();
  scannerData.replace("\r", "");  // ลบ carriage return (ถ้ามี)

  Serial.println("Real Data: " + scannerData);

  String cleanData = "";
  for (size_t i = 0; i < scannerData.length(); i++) {
    char c = scannerData[i];
    if (isPrintable(c)) cleanData += c;
  }
  scannerData = cleanData;

  for (auto &med : medicines) {
    if (scannerData == "medicine:" + med.code) {
      Serial.println("Medicine: " + med.name);

      lcd.clear();
      String firstLine, secondLine;
      int maxLineLength = 16;

      if (med.name.length() > maxLineLength) {
        firstLine = med.name.substring(0, maxLineLength);
        secondLine = med.name.substring(maxLineLength);
      } else {
        firstLine = med.name;
        secondLine = "";
      }

      lcd.setCursor(0, 0);
      lcd.print(firstLine);
      lcd.setCursor(0, 1);
      lcd.print(secondLine);

      if (isDFPlayerReady) {
        myDFPlayer.volume(volume);
        myDFPlayer.play(med.track);
      }
      Serial.print("Playing track: ");
      Serial.println(med.track);

      displayStartTime = millis();
      isDisplaying = true;

      return;
    }
  }

  Serial.println("Medicine not found.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Not in system");

  if (isDFPlayerReady) {
    myDFPlayer.volume(volume);
    myDFPlayer.play(1);
  }

  displayStartTime = millis();
  isDisplaying = true;
}

void loop() {
  if (digitalRead(IR_SENSOR_PIN) == LOW && !isScanning && !coolDown) {
    delay(50);  // หน่วงเวลาเพื่อ Debounce
    if (digitalRead(IR_SENSOR_PIN) == LOW) {
      startScanning();
    }
  }

  if (isScanning && millis() - startScanTime > scanDuration) {
    stopScanning();
  }

  if (stringComplete) {
    handleMedicine(DataScanner);
    inputString = "";
    DataScanner = "";
    stringComplete = false;
  }

  if (isDisplaying && millis() - displayStartTime >= clearDelay) {
    lcd.clear();
    isDisplaying = false;
  }

  if (coolDown && millis() - coolDownStartTime >= coolDownDuration) {
    coolDown = false;
  }
}

void startScanning() {
  if (!isScanning) {
    isScanning = true;
    startScanTime = millis();
    digitalWrite(SCANNER_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
    Serial.println(F("Scanning..."));
  }
}

void stopScanning() {
  if (isScanning) {
    isScanning = false;
    digitalWrite(SCANNER_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    Serial.println(F("Scan complete"));
    coolDown = true;
    coolDownStartTime = millis();
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;

    // Check if newline is received
    if (inChar == '\n') {
      stringComplete = true;
      DataScanner = inputString;
    }
  }
}
