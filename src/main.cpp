#include <M5StickC.h>
#include <WiFiManager.h>
#include <ArduinoOSC.h>
#include <endian.h>

#define SW 26
#define MODE_FAN 0
#define MODE_MIST 1

WiFiManager wifiManager;

const char* localhost = "127.0.0.1";
const int bind_port = 54345;

// ステータス
struct DEVICE_STATUS {
  int fan;
  int fog;
};
DEVICE_STATUS status;

enum CMD {
  FAN_0,
  FAN_1,
  FAN_2,
  FAN_3,
  FOG_0,
  FOG_1
};

void sw(uint8_t MODE = 0){
  // Serial.println("sw");
  M5.Lcd.fillScreen(GREEN);
  digitalWrite(SW, HIGH);
  if (MODE == 0){
    delay(300);
  }
  else {
    delay(3000);
  }
  M5.Lcd.fillScreen(BLACK);
  digitalWrite(SW, LOW);
  delay(500);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setupWiFi() {
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.autoConnect("M5StickC-Fan");
}

void setup() {
  Serial.begin(115200);
  status.fan = FAN_0;
  status.fog = FOG_0;

  M5.begin();
  M5.Axp.ScreenBreath(8);   // 7-12
  M5.Lcd.setRotation(0);    // 0-3
  M5.Lcd.fillScreen(GREEN);

  pinMode(SW, OUTPUT);
  digitalWrite(SW, LOW);

  // WiFI マネージャー初期化
  setupWiFi();
  
  Serial.println();
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(bind_port);

  // OSC受信設定
  // FAN "/cmd/fan 0" - "/cmd/fan 3"
  // FOG "/cmd/fog 4" - "/cmd/fog 5"
  // https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/wpa_supplicant/endian.h
  OscWiFi.subscribe(bind_port, "/cmd/fan",
      [](const OscMessage& m)
      {
        int c = (int) (m.arg<float>(0));
        Serial.print("/cmd/fan "); Serial.println(c);
        if ((FAN_0 <= c) && (FAN_3 >= c)){
          // FAN
          if (status.fan != c){
            //現在の status.fan をみて 指定 i になるまで繰り返す
            while (status.fan != c)
            {
              sw();
              status.fan += 1;
              if (status.fan > (int)FAN_3) status.fan = (int) FAN_0;
            }
          }
        }
      }
  );
  OscWiFi.subscribe(bind_port, "/cmd/fog",
      [](const OscMessage& m)
      {
        // Endian変換
        int c = (int) (m.arg<float>(0));
        Serial.print("/cmd/fog "); Serial.println(c);
        if ((FOG_0 <= c) && (FOG_1 >= c)){
          // FOG 現在の status.fog と異なったら 実行 トグル動作
          if (status.fog != c){
            sw(1);
            status.fog = c;
          }
        }
      }
  );     
}

void loop() {
  // Serial.println("loop");
  M5.update();
  if ( M5.BtnA.wasReleased() ) {
    Serial.println("wasReleased");
    // テストで自分に送る
    OscWiFi.send(localhost, bind_port, "/cmd/fog", (int)CMD::FOG_1);
  }
  if ( M5.BtnB.wasReleased() ) {
    OscWiFi.send(localhost, bind_port, "/cmd/fan", (int)CMD::FAN_3);
  }

  // ホームボタン長押し
  if ( M5.BtnA.wasReleasefor(3000) ) {
    Serial.println("wasReleasefor");
    OscWiFi.send(localhost, bind_port, "/cmd/fan", (int)CMD::FAN_0);
    OscWiFi.send(localhost, bind_port, "/cmd/fog", (int)CMD::FOG_0);
  }

  OscWiFi.update();
}