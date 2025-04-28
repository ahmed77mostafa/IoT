#include<Adafruit_SSD1306.h>
#include<MFRC522.h>
#include<SPI.h>
#include<WiFi.h>
#include<Wire.h>
#include<Firebase.h>
#include<UbidotsESPMQTT.h>

const char* SSID = "Hatem's S21 ultra";
const char* WIFI_PASSWORD = "Hatem_24";
const char* FIREBASE_URL = "https://case1-4b2bc-default-rtdb.firebaseio.com/";
const char* FIREBASE_TOKEN = "3QrFzAP88eOuwlZor4J0DQDJo8SzuLSr3RnNCUwP";
const char* DEVICE_TOKEN = "BBUS-C1jn61voGxsiSeBL5mq0VZn72FeQRW";

Firebase fb(FIREBASE_URL, FIREBASE_TOKEN);
Ubidots client(const_cast<char*>(DEVICE_TOKEN));

#define SS 2
#define RST 5
MFRC522 rfid(SS, RST);

#define BLUE 12
#define RED 14
#define BUZZER 27

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void ubidotsCallback(char* topic, byte* payload, unsigned int length){
   Serial.print("Topic arrived: ");
   Serial.println(topic);

   for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
   }
   Serial.println();
}

void showMessage(String msg){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10,20);
  display.println(msg);
  display.display();
}

String getUID(){
  String uid = "";
  for(byte i = 0; i < rfid.uid.size; i++){
    uid += String(rfid.uid.uidByte[i]);
  }
  return uid;
}

void grantAccess(String uid){
  showMessage("Access GRANTED...");
  digitalWrite(BLUE, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW);

  fb.pushString("logs", "Access GRANTED: " + uid);

  client.add(const_cast<char*>("logs"), 1);
  client.add(const_cast<char*>("uid"), uid.toDouble());
  client.ubidotsPublish(const_cast<char*>("case1"));
}

void denyAccess(String uid){
  showMessage("Access DENIED...");
  digitalWrite(RED, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(1000);
  digitalWrite(BUZZER, LOW);
  
  fb.pushString("logs", "Access DENIED: " + uid);

  client.add(const_cast<char*>("logs"), 0);
  client.add(const_cast<char*>("uid"), uid.toDouble());
  client.ubidotsPublish(const_cast<char*>("case1"));
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();
  pinMode(BLUE, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  WiFi.begin(SSID, WIFI_PASSWORD);
  Serial.println("Connecting to wifi....");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to wifi...");
  Serial.print("Wifi IP: ");
  Serial.println(WiFi.localIP());
  showMessage(String(WiFi.localIP()));

  client.setDebug(true);
  client.wifiConnection(const_cast<char*>(SSID), const_cast<char*>(WIFI_PASSWORD));
  client.begin(ubidotsCallback);
}

void loop() {
  if(!client.connected()){
    client.reconnect();
  }
  client.loop();

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    String uid = getUID();
    Serial.println("Scanned UID: " + uid);
    showMessage("Checking UID...");

    String path = "authorized/" + uid;
    int result = fb.getInt(path);
    Serial.println("Result: " + result);

    if(result == 1){
      grantAccess(uid);
    } else {
      denyAccess(uid);
    }

    rfid.PICC_HaltA();
  }
}