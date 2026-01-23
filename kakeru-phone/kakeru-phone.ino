#define TONE_USE_INT
#define TONE_PITCH 440
#include <WiFiS3.h>          // UNO R4 WiFi用
#include <TonePitch.h>
#include "config/config.h"   // SSID, PASS, NOTE定義など
#include <ArduinoOSCWiFi.h>

//=============================

// ★ 送り先PCのIPアドレス（プライベートIPv4）
const char* OSC_HOST = "192.168.0.119";
// ★ Max側の受信ポート
const uint16_t OSC_PORT = 3000;

// --- ピン設定 ---
const int pulsePin = 2; // ダイヤルの下接点（パルス）
const int shuntPin = 3; // ダイヤルの上接点（シャント）
const int hookPin  = 4; // フックスイッチ（受話器ON/OFF）

// --- 状態変数 ---
int pulseCount = 0;
bool isDialing = false;
int lastPulseState = LOW;
bool lastHookState = HIGH; // HIGH＝受話器が置かれている（INPUT_PULLUP基準）

int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);

  pinMode(pulsePin, INPUT_PULLUP);
  pinMode(shuntPin, INPUT_PULLUP);
  pinMode(hookPin, INPUT_PULLUP);

  Serial.println("Ready: Hook + Dial (0 unified)");

  // ===== Wi-Fi接続 =====
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFiモジュールが見つかりません");
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
    }
  }

  while (status != WL_CONNECTED) {
    Serial.print("接続中: ");
    Serial.println(WIFI_SSID);
    status = WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(5000);
  }

  Serial.println("✅ WiFi接続成功");
  Serial.print("ArduinoのIPアドレス: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}

void loop() {
  // ============================
  // ☎️ フックスイッチ監視部分
  // ============================
  bool hookState = digitalRead(hookPin);

  if (hookState != lastHookState) {
    lastHookState = hookState;
    delay(20); // チャタリング防止

    if (hookState == LOW) {
      // 受話器が上がった → /dial 0 を送る
      Serial.println("Hook OFF → Sending /dial 0");
      OscWiFi.send(OSC_HOST, OSC_PORT, "/dial", 0);
      OscWiFi.update();
    } else {
      Serial.println("🧷 Hook ON (Receiver placed)");
    }
  }


  // ☎️ ダイヤルパルス検出部分
  // =================================
  if (digitalRead(shuntPin) == LOW) { // ダイヤルを回している間
    if (!isDialing) {
      isDialing = true;
      pulseCount = 0;
      Serial.println("Dialing started...");
    }

    int currentPulseState = digitalRead(pulsePin);
    if (lastPulseState == LOW && currentPulseState == HIGH) {
      pulseCount++;
      delay(30); // チャタリング防止
    }
    lastPulseState = currentPulseState;
  } 
  else { // シャントがHIGH → ダイヤル戻りきり
    if (isDialing) {
      if (pulseCount > 0) {
        int digit = pulseCount % 10;
        Serial.print("Dial detected: ");
        Serial.println(digit);

        
        OscWiFi.send(OSC_HOST, OSC_PORT, "/dial", digit);
        OscWiFi.update();
        Serial.print("Sent to Max: /dial ");
        Serial.println(digit);
      }
      isDialing = false;
      pulseCount = 0;
    }
  }

  // ============================
  // OSC通信の更新
  // ============================
  OscWiFi.update();
}
