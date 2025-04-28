#include<Adafruit_SSD1306.h>
#include<MFRC522.h>
#include<SPI.h>
#include<WiFi.h>
#include<Wire.h>
#include<Firebase.h>
#include<UbidotsESPMQTT.h>

const char* SSID = "MyHotspost";
const char* WIFI_PASSWORD = "123456778#";
const char* FIREBASE_URL = "https://smart-home-f91b4-default-rtdb.firebaseio.com/";
const char* FIREBASE_TOKEN = "9g6suAdMjituzGv4JfKFYVlrPRXqZN9IoHbZP1v9";
const char* DEVICE_TOKEN = "BBUS-Pi6frD4pXMfy7WROqYzPRxgT9i0Fvu";

Firebase fb(FIREBASE_URL, FIREBASE_TOKEN);
Ubidots client(const_cast<char*>(DEVICE_TOKEN));

#define SS 2
#define RST 5
MFRC522 rfid(SS, RST);

#define BLUE 12
#define RED 14
#define BUZZER 27

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void showMessage(String msg){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
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
  
  fb.pushString("attendance/"+uid, String(millis()));

  client.add("uid", uid.toDouble());
  client.add(const_cast<char*>("logs"), 1);
  client.ubidotsPublish(const_cast<char*>("case2"));

  delay(1000);
  digitalWrite(BLUE, LOW);
}
void denyAccess(String uid){
  showMessage("Access DENIED...");
  digitalWrite(RED, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(1000);
  digitalWrite(BUZZER, LOW);

  client.add("uid", uid.toDouble());
  client.add(const_cast<char*>("logs"), 1);
  client.ubidotsPublish(const_cast<char*>("case2"));

  delay(1000);
  digitalWrite(RED, LOW);
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
  Serial.println("Connecting to wifi..");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("Wifi connected.!");
  showMessage("IP: " + String(WiFi.localIP()));

  client.setDebug(true);
  client.wifiConnection(const_cast<char*>(SSID),const_cast<char*>(WIFI_PASSWORD));
  client.begin(callback);
}

void loop() {
  if(!client.connected()){
    client.reconnect();
  }
  client.loop();

  if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    String uid = getUID();
    Serial.println("Scanned UID: " + uid);

    showMessage("Checking UID....");

    String path = "student/"+uid+"/name";
    String result = fb.getString(path);
    Serial.println("result is  "+ result);
    if(result != "null"){ 
      grantAccess(uid);
    }else{
      denyAccess(uid);
    }

    rfid.PICC_HaltA();
  }
}