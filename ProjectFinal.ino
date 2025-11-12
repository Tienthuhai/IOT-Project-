#define BLYNK_TEMPLATE_ID   "TMPL6oOC0yUk4"
#define BLYNK_TEMPLATE_NAME "ProjectFinal"
#define BLYNK_AUTH_TOKEN    "dhSbEkuKlVUnsmLLU8GLPZRqZXgKz2KR"

#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <HTTPClient.h>

// ================== WIFI ==================
char ssid[] = "Thuy Duong";
char pass[] = "0977061097";

// ================== DHT SENSOR ==================
#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================== LCD ==================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== RELAY ==================
#define HEATER_PIN      15
#define FAN_COOL_PIN    5
#define FAN_VENT_PIN    18
#define HUMIDIFIER_PIN  19

// ================== BUTTONS ==================
#define BTN_MODE        13
#define BTN_HEATER      12
#define BTN_HUMIDIFIER  14
#define BTN_FAN_VENT    27
#define BTN_FAN_COOL    26

// ================== THRESHOLDS ==================
float TempLow = 37.0;
float TempHigh = 39.0;
float HumLow  = 55.0;
float HumHigh = 60.0;
bool autoMode = true;

// ================== GOOGLE SHEET ==================
String googleSheetUrl = "https://script.google.com/macros/s/AKfycbzkYDlg6MHV2XrGfCg6t4uOTiwU6dBNRSXAfmnZkTvdC1_-YvAEQU5MVOykcpr9H1UY/exec";

// ================== VARIABLES ==================
BlynkTimer timer;
unsigned long lastDisplaySwitch = 0;
bool showThreshold = false; // true = ngưỡng, false = giá trị thực

// ================== BLYNK VIRTUAL PINS ==================
#define VP_TEMP_LOW  V7
#define VP_TEMP_HIGH V8
#define VP_HUM_LOW   V9
#define VP_HUM_HIGH  V10
#define VP_MODE      V2
#define VP_HEATER    V3
#define VP_FAN_COOL  V4
#define VP_FAN_VENT  V5
#define VP_HUMIDIFIER V6

// =========================================================
//                  HÀM GỬI DỮ LIỆU GOOGLE SHEET
// =========================================================
void sendToGoogleSheet() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("❌ Sensor Error, skip sendToGoogleSheet");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = googleSheetUrl + "?sts=write&temp=" + String(temp) + "&humi=" + String(hum);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
      Serial.println("✅ Data sent to Google Sheet successfully!");
    else
      Serial.println("⚠️ Error sending data: " + String(httpCode));

    http.end();
  }

  // Cập nhật lên Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
}

// =========================================================
//                     XỬ LÝ AUTO MODE
// =========================================================
void updateAutoMode(float temp, float hum) {
  if (!autoMode) return;

  // Nhiệt độ
  if (temp < TempLow) {
    digitalWrite(HEATER_PIN, HIGH);
    digitalWrite(FAN_COOL_PIN, LOW);
  } else if (temp > TempHigh) {
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(FAN_COOL_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(FAN_COOL_PIN, LOW);
  }

  // Độ ẩm
  if (hum < HumLow) {
    digitalWrite(HUMIDIFIER_PIN, HIGH);
    digitalWrite(FAN_VENT_PIN, LOW);
  } else if (hum > HumHigh) {
    digitalWrite(HUMIDIFIER_PIN, LOW);
    digitalWrite(FAN_VENT_PIN, HIGH);
  } else {
    digitalWrite(HUMIDIFIER_PIN, LOW);
    digitalWrite(FAN_VENT_PIN, LOW);
  }
  if (temp < TempLow)  
  Blynk.logEvent("TEMP_LOW", String("Nhiet do thap hon muc cho phep " + String(temp) + "°C, " + String(hum) + "%"));
  if (temp > TempHigh) 
  Blynk.logEvent("TEMP_HIGH", String("Nhiet do cao hon muc cho phep " + String(temp) + "°C, " + String(hum) + "%"));
  if (hum  < HumLow)   
  Blynk.logEvent("HUM_LOW", String("Do am thap hon muc cho phep " + String(temp) + "°C, " + String(hum) + "%"));
  if (hum  > HumHigh)  
  Blynk.logEvent("HUM_HIGH", String("Do am cao hon muc cho phep " + String(temp) + "°C, " + String(hum) + "%"));


}

// =========================================================
//                      HIỂN THỊ LCD
// =========================================================
void displayLCD(float temp, float hum) {
  unsigned long now = millis();
  if (now - lastDisplaySwitch >= 10000) { // 10s đổi hiển thị
    lastDisplaySwitch = now;
    showThreshold = !showThreshold;
    lcd.clear();
  }

  if (autoMode && showThreshold) {
    lcd.setCursor(0, 0);
    lcd.print("Tg:");
    lcd.print(TempLow, 1);
    lcd.print("-");
    lcd.print(TempHigh, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hg:");
    lcd.print(HumLow, 0);
    lcd.print("-");
    lcd.print(HumHigh, 0);
    lcd.print("%");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(hum, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Mode: ");
    lcd.print(autoMode ? "AUTO  " : "MANUAL");
  }
}

// =========================================================
//                XỬ LÝ NÚT NHẤN VẬT LÝ
// =========================================================
void readPhysicalButtons() {
  static bool lastBtnMode = HIGH;
  bool btnState = digitalRead(BTN_MODE);
  if (lastBtnMode == HIGH && btnState == LOW) {
    autoMode = !autoMode;
    Blynk.virtualWrite(V2, autoMode ? 0 : 1);
  }
  lastBtnMode = btnState;

  if (!autoMode) {
    if (digitalRead(BTN_HEATER) == LOW)      digitalWrite(HEATER_PIN, !digitalRead(HEATER_PIN));
    if (digitalRead(BTN_HUMIDIFIER) == LOW)  digitalWrite(HUMIDIFIER_PIN, !digitalRead(HUMIDIFIER_PIN));
    if (digitalRead(BTN_FAN_VENT) == LOW)    digitalWrite(FAN_VENT_PIN, !digitalRead(FAN_VENT_PIN));
    if (digitalRead(BTN_FAN_COOL) == LOW)    digitalWrite(FAN_COOL_PIN, !digitalRead(FAN_COOL_PIN));
    delay(200);
  }
}

// =========================================================
//                     BLYNK HANDLERS
// =========================================================
BLYNK_WRITE(V7){ float val = param.asFloat(); if(val < TempHigh) TempLow = val; else Blynk.virtualWrite(V7, TempLow); }
BLYNK_WRITE(V8){ float val = param.asFloat(); if(val > TempLow) TempHigh = val; else Blynk.virtualWrite(V8, TempHigh); }
BLYNK_WRITE(V9){ float val = param.asFloat(); if(val < HumHigh) HumLow = val; else Blynk.virtualWrite(V9, HumLow); }
BLYNK_WRITE(V10){ float val = param.asFloat(); if(val > HumLow) HumHigh = val; else Blynk.virtualWrite(V10, HumHigh); }
BLYNK_WRITE(V2){ autoMode = (param.asInt() == 0); }
BLYNK_WRITE(V3){ if(!autoMode) digitalWrite(HEATER_PIN, param.asInt()); }
BLYNK_WRITE(V4){ if(!autoMode) digitalWrite(FAN_COOL_PIN, param.asInt()); }
BLYNK_WRITE(V5){ if(!autoMode) digitalWrite(FAN_VENT_PIN, param.asInt()); }
BLYNK_WRITE(V6){ if(!autoMode) digitalWrite(HUMIDIFIER_PIN, param.asInt()); }

BLYNK_CONNECTED() {
  Blynk.virtualWrite(V7, TempLow);
  Blynk.virtualWrite(V8, TempHigh);
  Blynk.virtualWrite(V9, HumLow);
  Blynk.virtualWrite(V10, HumHigh);
}

// =========================================================
//                        SETUP
// =========================================================
void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
  Wire.begin(22,21);
  lcd.begin(16,2);
  lcd.backlight();

  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_COOL_PIN, OUTPUT);
  pinMode(FAN_VENT_PIN, OUTPUT);
  pinMode(HUMIDIFIER_PIN, OUTPUT);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_HEATER, INPUT_PULLUP);
  pinMode(BTN_HUMIDIFIER, INPUT_PULLUP);
  pinMode(BTN_FAN_VENT, INPUT_PULLUP);
  pinMode(BTN_FAN_COOL, INPUT_PULLUP);

  lcd.setCursor(0,0);
  lcd.print("Incubator Ready");
  delay(1500);
  lcd.clear();

  timer.setInterval(10000L, sendToGoogleSheet); // Gửi Google Sheet mỗi 10s
}

// =========================================================
//                         LOOP
// =========================================================
void loop() {
  Blynk.run();
  timer.run();
  readPhysicalButtons();

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) return;

  updateAutoMode(temp, hum);
  displayLCD(temp, hum);

  // Gửi relay status lên Blynk
  Blynk.virtualWrite(V3, digitalRead(HEATER_PIN));
  Blynk.virtualWrite(V4, digitalRead(FAN_COOL_PIN));
  Blynk.virtualWrite(V5, digitalRead(FAN_VENT_PIN));
  Blynk.virtualWrite(V6, digitalRead(HUMIDIFIER_PIN));
}
