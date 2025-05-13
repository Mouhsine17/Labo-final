#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <LCD_I2C.h>
#include <HCSR04.h>
#include <U8g2lib.h>
#include "Alarm.h"
#include "ViseurAutomatique.h"

#define TRIGGER_PIN 9
#define ECHO_PIN 10
#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37
#define RED_PIN 2
#define GREEN_PIN 3
#define BLUE_PIN 4
#define DIN_PIN 32
#define CLK_PIN 28
#define CS_PIN 30
#define AT_BAUD_RATE 115200

LCD_I2C lcd(0x27, 16, 2);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
U8G2_MAX7219_8X8_1_4W_SW_SPI max7219(U8G2_R0, CLK_PIN, DIN_PIN, CS_PIN, U8X8_PIN_NONE);

float distance = 0;
float angle = 0;
bool moteurActif = true;
char currentColor[8] = "#ff0000";


Alarm alarme(RED_PIN, GREEN_PIN, BLUE_PIN, &distance);
ViseurAutomatique viseur(IN_1, IN_2, IN_3, IN_4, distance);

WiFiClient wifiClient;
PubSubClient client(wifiClient);

const char ssid[] = "TechniquesInformatique-Etudiant";
const char pass[] = "shawi123";

unsigned long lastMQTT = 0;
unsigned long lastLCD = 0;
String oldLine1 = "", oldLine2 = "";

float readStableDistance() {
  float total = 0;
  int validCount = 0;
  for (int i = 0; i < 5; i++) {
    float d = hc.dist();
    if (d > 2 && d < 300) {
      total += d;
      validCount++;
    }
    delay(10);
  }
  return (validCount > 0) ? (total / validCount) : 999;
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connecté au WiFi.");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect("Mouhsine", "etdshawi", "shawi123")) {
      Serial.println(" connectée !");
      client.subscribe("etd/1/data");
    } else {
      Serial.print(" erreur, code=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("MQTT>> : ");
  Serial.println(msg);

  if (msg.indexOf("color") > 0 && msg.indexOf("#") > 0) {
    String hex = msg.substring(msg.indexOf("#"), msg.indexOf("#") + 7);
    hex.toCharArray(currentColor, 8);
    long color = strtol(hex.substring(1).c_str(), NULL, 16);
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    alarme.setColourA(r, g, b);
    alarme.setColourB(r, g, b);

  }
 bool newMotorState = extractMotorFromJson(msg);
if (newMotorState != moteurActif) {
  moteurActif = newMotorState;
  Serial.print(" Moteur => ");
  Serial.println(moteurActif ? "Activé" : "Désactivé");
}
}
bool extractMotorFromJson(const String& msg) {
  int motorIndex = msg.indexOf("\"motor\":");
  if (motorIndex >= 0) {
    int valueStart = motorIndex + 8;
    String val = msg.substring(valueStart, valueStart + 5);
    val.trim();
    if (val.startsWith("true")) return true;
    if (val.startsWith("false")) return false;
    if (val.startsWith("1")) return true;
    if (val.startsWith("0")) return false;
  }
  return moteurActif; // ne rien changer si on ne trouve rien
}


void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);
  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(&Serial1);
  connectWiFi();

  client.setServer("216.128.180.194", 1883);
  client.setCallback(callback);

  lcd.begin(); lcd.backlight();
  max7219.begin();

  viseur.setAngleMin(10);
  viseur.setAngleMax(170);
  viseur.setPasParTour(2048);
  viseur.setDistanceMinSuivi(30);
  viseur.setDistanceMaxSuivi(60);
  viseur.activer();

  alarme.setDistance(15);
  alarme.setVariationTiming(100);
  alarme.setColourA(255, 0, 0);
  alarme.setColourB(0, 0, 255);
  alarme.turnOn();

  lcd.setCursor(0, 0); lcd.print("2349185");
  lcd.setCursor(0, 1); lcd.print("Labo 7");
  delay(2000); lcd.clear();
}

void loop() {
  int rawLight = analogRead(A0);
  int lightPercent = map(rawLight, 0, 1023, 0, 100);

  if (!client.connected()) reconnectMQTT();
  client.loop();

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck >= 60) {
    distance = readStableDistance();
    lastCheck = millis();
  }

  if (distance >= 15) {
    alarme.setColourA(0, 0, 0);
    alarme.setColourB(0, 0, 0);
  }

if (moteurActif && distance >= 30 && distance <= 60) {
  viseur.activer();
  viseur.update();
} else {
  viseur.desactiver();
}

if (moteurActif && distance >= 30 && distance <= 60) {
  viseur.activer();     // autorise les mouvements
  viseur.update();      // le fait bouger
} else {
  viseur.desactiver();  // stoppe tout
}

  alarme.update();
  angle = viseur.getAngle();

  unsigned long now = millis();
  if (now - lastLCD >= 1100) {
    String line1 = "Dist: " + String((int)distance) + " cm";
    String line2 = (distance < 30) ? "Trop pres" :
                   (distance > 60) ? "Trop loin" :
                   "Degre: " + String((int)angle);

    if (line1 != oldLine1 || line2 != oldLine2) {
      char lcdMsg[150];
      sprintf(lcdMsg, "{\"line1\":\"%s\", \"line2\":\"%s\"}", line1.c_str(), line2.c_str());
      client.publish("etd/1/data", lcdMsg);
      oldLine1 = line1;
      oldLine2 = line2;
    }

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(line1);
    lcd.setCursor(0, 1); lcd.print(line2);
    lastLCD = now;
  }

if (now - lastMQTT >= 2500) {
  char distStr[10], angleStr[10];
  dtostrf(distance, 4, 2, distStr);
  dtostrf(angle, 4, 2, angleStr);

  char dataMsg[350];
  sprintf(dataMsg,
    "{\"name\":\"Mouhsine\",\"number\":1,\"uptime\":%lu,\"dist\":%s,\"angle\":%s,\"motor\":%d,\"color\":\"%s\",\"lum\":%d}",
    now / 1000, distStr, angleStr, moteurActif ? 1 : 0, currentColor, lightPercent);

  client.publish("etd/1/data", dataMsg);
  Serial.println(dataMsg);
  lastMQTT = now;
}

}
