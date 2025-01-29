#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// Use HardwareSerial for ESP32
HardwareSerial myHardwareSerial(1);  // Use Serial1
DFRobotDFPlayerMini myDFPlayer;

// Define RX and TX pins
#define LED_PIN 12
#define RX_PIN 16         // Connect to DFPlayer's TX
#define TX_PIN 17         // Connect to DFPlayer's RX
#define IR_SENSOR_PIN 13  // Pin for IR sensor
#define SCANNER_PIN 15    // Pin for scanner control
#define MAX_DISTANCE 100

LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD I2C address: 0x27, size: 16x2

bool isScanning = false;                      // Scanning state
bool coolDown = false;                        // Cool-down state
unsigned long startScanTime = 0;              // Start time for scanning
unsigned long coolDownStartTime = 0;          // Start time for cool-down
const unsigned long scanDuration = 5000;      // Scanning duration (5 seconds)
const unsigned long coolDownDuration = 3000;  // Cool-down duration (3 seconds)
const unsigned long clearDelay = 20000;       // ระยะเวลาสำหรับลบข้อความ (20 วินาที)

// เพิ่มตัวแปร global
unsigned long displayStartTime = 0;  // เวลาเริ่มแสดงผล
bool isDisplaying = false;           // สถานะแสดงผล LCD

String inputString = "";      // Store data from scanner
String DataScanner = "";      // Processed scanner data
bool stringComplete = false;  // Flag to indicate complete data
byte volume = 10; //กำหนดระดับความดัง 0 - 30

struct Medicine {
  String code;
  String name;
  int track;
};

String currentState = "";  // เก็บสถานะปัจจุบัน

// Array to store medicine data
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
  pinMode(IR_SENSOR_PIN, INPUT);                             // IR sensor input
  pinMode(SCANNER_PIN, OUTPUT);                              // Scanner control output
  pinMode(LED_PIN, OUTPUT);
  lcd.begin();      // Initialize LCD
  lcd.backlight();  // Turn on LCD backlight

  inputString.reserve(200);  // Reserve memory for string

  digitalWrite(SCANNER_PIN, HIGH);  // Turn scanner off initially

  // Initialize DFPlayer
  unsigned long startMillis = millis();
  while (!myDFPlayer.begin(myHardwareSerial)) {
    if (millis() - startMillis > 5000) {  // Timeout after 5 seconds
      Serial.println(F("Unable to begin DFPlayer"));
      break;
    }
    delay(100);
  }
  // myDFPlayer.volume(volume);  // Set volume level
  Serial.println(F("DFPlayer Mini online."));
}

void handleMedicine(String scannerData) {
  scannerData.trim();  // Remove any whitespace or newline characters

  Serial.println("Real Data: " + scannerData);

  // Filter printable characters
  String cleanData = "";
  for (size_t i = 0; i < scannerData.length(); i++) {
    char c = scannerData[i];
    if (isPrintable(c)) {
      cleanData += c;
    }
  }
  scannerData = cleanData;

  for (auto &med : medicines) {
    if (scannerData == "medicine:" + med.code) {
      Serial.println("Medicine: " + med.name);

      lcd.clear();

      // Initialize strings for the two lines
      String firstLine, secondLine;
      int maxLineLength = 16;  // Maximum characters per line for 1602 LCD

      // Check if the name is longer than the maximum line length
      if (med.name.length() > maxLineLength) {
        firstLine = med.name.substring(0, maxLineLength);  // First 16 characters
        secondLine = med.name.substring(maxLineLength);    // Remaining characters
      } else {
        firstLine = med.name;  // Fits in one line
        secondLine = "";       // Second line remains empty
      }

      // Display on the LCD
      lcd.setCursor(0, 0);
      lcd.print(firstLine);
      lcd.setCursor(0, 1);
      lcd.print(secondLine);

      // if(!myDFPlayer.begin(myHardwareSerial)) {
      //   Serial.println("No module mp3");
      //   lcd.clear();
      //   lcd.print("No module mp3");
      // }

      // Play the corresponding track
      myDFPlayer.volume(volume); 
      myDFPlayer.play(med.track);
      Serial.print("play med.track:");
      Serial.println(med.track);

      // Start timer for displaying on LCD
      displayStartTime = millis();
      isDisplaying = true;

      return;
    }
  }

  // If no match found
  Serial.println("Medicine not found in system.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Not in system");
  myDFPlayer.volume(volume); 
  myDFPlayer.play(1);

  // Start timer for displaying "Not in system"
  displayStartTime = millis();
  isDisplaying = true;

  Serial.println("Cleaned Data: " + scannerData);
}

void loop() {
  if (digitalRead(IR_SENSOR_PIN) == LOW && !isScanning) {
    startScanning();
  }

  if (isScanning && millis() - startScanTime > scanDuration) {
    stopScanning();
  }

  // Handle data if string is complete
  if (stringComplete) {
    handleMedicine(DataScanner);
    inputString = "";  // Clear input buffer
    DataScanner = "";  // Clear processed data
    stringComplete = false;
  }
}

// ฟังก์ชันเริ่มการสแกน
void startScanning() {
  isScanning = true;
  startScanTime = millis();
  digitalWrite(SCANNER_PIN, LOW); // เปิดเครื่องสแกน
  digitalWrite(LED_PIN, HIGH);   // เปิดไฟ LED
  // lcd.clear();
  // lcd.print("Scanning...");
}

// ฟังก์ชันหยุดการสแกน
void stopScanning() {
  isScanning = false;
  digitalWrite(SCANNER_PIN, HIGH); // ปิดเครื่องสแกน
  digitalWrite(LED_PIN, LOW);      // ปิดไฟ LED
  // lcd.clear();
  // lcd.print("Scan complete");
  // delay(1000); // แสดงข้อความก่อนกลับสู่สถานะพร้อม
  // lcd.clear();
}

// Function to read data from serial input
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
