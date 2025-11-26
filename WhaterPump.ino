#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "RTClib.h"
#include "html.h"
#define EEPROM_WIFI_ADDR 50
#define EEPROM_SIZE 128
#define RELAY_PIN 26
#define BUTON_PIN 32

#define LED_AZUL 5
#define LED_VERMELHO 16
#define LED_VERDE 17

#define TRIG_PIN 2
#define ECHO_PIN 15

WebServer server(80);
RTC_DS3231 rtc;
String tempW;

const char* ssid = "WaterPump";
String wifiPassword, F_wifiPassword = "12345678"; // default wifi password

String F_start1, start1 = "05:00", F_stop1, stop1 = "10:00";
String F_start2, start2 = "05:00", F_stop2, stop2 = "10:00";

int counter = 0;
bool radioState = false;
bool buttonHeld = false;
unsigned long buttonPressMillis = 0;
unsigned long buttonPressMillis_2 = 0;

bool waterBuoy = false;
// Timer
unsigned long previousMillis = 0;
unsigned long wifi_previousMillis = 0;
unsigned long wifi_activeTime = 0;
const unsigned long intervalCheck = 1000;
int leds[]= {LED_AZUL, LED_VERDE, LED_VERMELHO};

void writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
}
String readStringFromEEPROM(int addrOffset) {
  int len = EEPROM.read(addrOffset);
  char data[len + 1];
  for (int i = 0; i < len; i++) data[i] = EEPROM.read(addrOffset + 1 + i);
  data[len] = '\0';
  return String(data);
}
void writeWiFiPassToEEPROM(const String &pass) {
  byte len = pass.length() > 10 ? 10 : pass.length();
  EEPROM.write(EEPROM_WIFI_ADDR, len);
  for (int i=0; i<10; i++) {
    char c = i < len ? pass[i] : 0;
    EEPROM.write(EEPROM_WIFI_ADDR+1+i, c);
  }
}
String readWiFiPassFromEEPROM() {
  int len = EEPROM.read(EEPROM_WIFI_ADDR);
  char data[11];
  for (int i=0; i<10; i++) data[i] = EEPROM.read(EEPROM_WIFI_ADDR+1+i);
  data[10] = '\0';
  return String(data).substring(0,len);
}
unsigned long timeToMillis(String timeStr) {
  int h = timeStr.substring(0, 2).toInt();
  int m = timeStr.substring(3, 5).toInt();
  return (h * 3600000UL) + (m * 60000UL);
}
void handleSet() {
  int size=0;
  if (server.hasArg("start1")) start1 = server.arg("start1");
  if (server.hasArg("stop1"))  stop1  = server.arg("stop1");
  if (server.hasArg("start2")) start2 = server.arg("start2");
  if (server.hasArg("stop2"))  stop2  = server.arg("stop2");
  if(server.hasArg("wifiPass")) {
    wifiPassword = server.arg("wifiPass");
    size = wifiPassword.length();
    if((size >= 8) && (size <= 10)){
      writeWiFiPassToEEPROM(wifiPassword);
      EEPROM.commit();
    }
  }
  // Save in EEPROM
  writeStringToEEPROM(0, start1);
  writeStringToEEPROM(10, stop1);
  writeStringToEEPROM(20, start2);
  writeStringToEEPROM(30, stop2);
  EEPROM.commit();
  String w_r = "";
  bool newPass = false;
  if ((wifiPassword != tempW) && (size <= 10) && (size >= 8)){
    w_r = "<br>📶 Nova palavra-passe: " + wifiPassword +
    "<br><br>O controlador de eletrobomba será reiniciado <br>"+
    "para por em vigor a nova palavra-passe do Wi-Fi.";
    tempW = wifiPassword;
    newPass = true;
  }else if((size > 10) || (size <8)){
    w_r = "<br>⚠️A palavra-passe deve ter no mínimo 8 e no maximo 10 caracteres. <br>"
              "Não foi possivel trocar a palavra-passe para " + wifiPassword + "<br>";
    newPass = false;
  }
  String msg = "<b>Novos Horários atribuidos!</b><br>"
              "1️º Horário 🕢: " + start1 + " - " + stop1 + "<br>" +
              "2️º Horário 🕖: " + start2 + " - " + stop2 + "<br>" +
              "" + w_r + "";
  server.send(200, "text/html", msg);
  Serial.println(msg);
  if(newPass) {
    for(int i=0; i<3; i++){
      digitalWrite(leds[i], 0);
    }
    for(int i=0; i<10; i++){
      digitalWrite(leds[2], 0);
      digitalWrite(leds[1], 0);
      digitalWrite(leds[0], 1);
      delay(100);
      digitalWrite(leds[2], 0);
      digitalWrite(leds[1], 1);
      digitalWrite(leds[0], 0);
      delay(100);
      digitalWrite(leds[2], 1);
      digitalWrite(leds[1], 0);
      digitalWrite(leds[0], 0);
      delay(200);
    }
    for(int i=0; i<3; i++){
      digitalWrite(leds[i], 0);
    }
    ESP.restart();
  }
}
void handleRoot() {
  DateTime now = rtc.now();
  char currentDate[20];
  sprintf(currentDate, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  char currentTime[10];
  sprintf(currentTime, "%02d:%02d", now.hour(), now.minute());

  String html = String(config_html);
  html.replace("value_start1", start1);
  html.replace("value_stop1",  stop1);
  html.replace("value_start2", start2);
  html.replace("value_stop2",  stop2);
  html.replace("value_wifiPass", wifiPassword);
  server.send(200, "text/html", html);
}
void handleSetTime() {
  if (!server.hasArg("datetime")) {
    server.send(400, "text/plain", "Parâmetro datetime ausente");
    return;
  }
  String datetime = server.arg("datetime");
  Serial.print("Recebido datetime: ");
  Serial.println(datetime);
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  if (strptime(datetime.c_str(), "%Y-%m-%dT%H:%M", &tm) == NULL) {
    server.send(400, "text/plain", "Formato inválido (use yyyy-mm-ddThh:mm)");
    return;
  }
  // Ajist RTC DS3231 with received values
  rtc.adjust(DateTime(
    tm.tm_year + 1900,
    tm.tm_mon + 1,
    tm.tm_mday,
    tm.tm_hour,
    tm.tm_min,
    tm.tm_sec
  ));
  server.send(200, "text/plain", "Hora atualizada com sucesso!");
  server.send(400, "text/plain", "Formato inválido...");
  server.send(400, "text/plain", "Parâmetro datetime ausente");
}
void curetTime() {
  DateTime now = rtc.now();
  Serial.printf("[!] Curret datetime: %02d/%02d/%02d %02d:%02d:%02d\n",now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
}
void ptmilis(int interval) {
  static unsigned long before = 0;
  static bool state = false;
  unsigned long after = millis();
  bool led_state = true;
  if (state && after - before >= 500) {
    state = false;
    before = after;
    bool bstate = digitalRead(BUTON_PIN);
    if (!radioState) digitalWrite(LED_AZUL, LOW);
    if(radioState && !bstate) {
      digitalWrite(LED_VERMELHO, LOW);
      
    }
    delay(500);
    led_state = false;
  }else if (!state && after - before >= interval) {
    state = true;
    before = after;
    curetTime();
    bool bstate = digitalRead(BUTON_PIN);
    //Serial.println(!bstate);
    if(!bstate) digitalWrite(LED_AZUL, HIGH);
    if(radioState && !bstate) digitalWrite(LED_VERMELHO, HIGH);
    if(led_state) digitalWrite(LED_VERMELHO, HIGH);
    
  }
}
void blinkmilis(int higthtime = 500, int lowtime = 500) {
  static unsigned long before = 0;
  static bool state = false;
  unsigned long after = millis();
  if (state && after - before >= 500) {
    state = false;
    before = after;
    digitalWrite(LED_VERDE, LOW);
  }else if (!state && after - before >= higthtime) {
    state = true;
    before = after;
    digitalWrite(LED_VERDE, HIGH);
  }
}
void checkButton() {
  bool pressed = !digitalRead(BUTON_PIN); // ativo em LOW

  if (pressed && !buttonHeld) {
    buttonHeld = true;
    buttonPressMillis = millis();
  }

  if (!pressed && buttonHeld) {
    buttonHeld = false;
    counter = 0; 
  }
  if (buttonHeld) {
    unsigned long heldTime = millis() - buttonPressMillis;
    if (heldTime >= 5000 && !radioState ) {
      turOnWifi(300000);
      while(!digitalRead(BUTON_PIN)) {
        delay(2500);
      }
      buttonHeld = false;
      counter = 0;
    }
    if(heldTime >= 10000 && radioState){
      for(int i=0; i<3; i++){
        digitalWrite(leds[i], 0);
      }
      delay(1000);
      for(int i=0; i<3; i++){
        digitalWrite(leds[i], 1);
        delay(800);
      }
      for(int i=0; i<8; i++){
        digitalWrite(leds[2], 0);
        delay(300);
        digitalWrite(leds[2], 1);
        delay(64);
      }
      for(int i=0; i<3; i++){
        digitalWrite(leds[i], 0);
      }
      delay(1500);
      writeWiFiPassToEEPROM(F_wifiPassword);
      EEPROM.commit();
      delay(500);
      writeStringToEEPROM(0, F_start1);
      writeStringToEEPROM(10, F_stop1);
      writeStringToEEPROM(20, F_start2);
      writeStringToEEPROM(30, F_stop2);
      EEPROM.commit();
      delay(500);
      ESP.restart();
    }
  }
}
void turOnWifi(unsigned long activeTime){
  String tempPass = readWiFiPassFromEEPROM();
  if(tempPass != "") wifiPassword = tempPass;
  tempW = wifiPassword;
  WiFi.mode(WIFI_AP);
  bool status = WiFi.softAP(ssid, wifiPassword.c_str());
  if(status){
    radioState = true;
    wifi_previousMillis = millis();   // guarda tempo de início
    wifi_activeTime = activeTime;     // guarda tempo de duração
    Serial.println("\nAccess Point iniciado!");
    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("Password: "); Serial.println(wifiPassword);
    Serial.print("Water Pump IP: "); Serial.println(WiFi.softAPIP());
    digitalWrite(LED_AZUL, HIGH);
    ////////////////////////////////////////////////
    server.on("/", handleRoot);
    server.on("/set", HTTP_POST, handleSet);
    server.on("/set_time", HTTP_POST, handleSetTime);
    server.begin();
    Serial.println("Servidor Web ativo.");
  }else Serial.println("WIFI ERROR");
}
bool turnOffWifi() {
  if(radioState){
    unsigned long currentMillis = millis();
    if(currentMillis - wifi_previousMillis >= wifi_activeTime){ // usa o tempo guardado
      Serial.println("⏱ Tempo limite atingido. Desligando WiFi...");
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      radioState = false;
      digitalWrite(LED_AZUL, LOW);
      return true;
    }
  }
  return false;
}
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, LOW);
  pinMode(LED_VERDE, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERMELHO, LOW);
  pinMode(BUTON_PIN, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);

  if (!rtc.begin()) {
    Serial.println("❌ Erro ao iniciar RTC!");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC sem hora definida! \nAjustando com hora e data da gravação do FIRMWARE...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // Read Date time
  String temp;
  temp = readStringFromEEPROM(0);  if(temp!="") start1 = temp;
  temp = readStringFromEEPROM(10); if(temp!="") stop1 = temp;
  temp = readStringFromEEPROM(20); if(temp!="") start2 = temp;
  temp = readStringFromEEPROM(30); if(temp!="") stop2 = temp;
  digitalWrite(LED_VERMELHO, HIGH);
  for(int i=0;i<12; i++){
    digitalWrite(LED_VERMELHO, LOW);
    digitalWrite(LED_AZUL, HIGH);
    delay(200);
    digitalWrite(LED_VERMELHO, HIGH);
    digitalWrite(LED_AZUL, LOW);
    delay(200);
  }
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_AZUL, LOW);
}
void loop() {
  if(radioState) server.handleClient();
  checkButton();
  turnOffWifi();
  ptmilis(10);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalCheck) {
    previousMillis = currentMillis;
    DateTime now = rtc.now();
    unsigned long nowMs = (now.hour() * 3600UL + now.minute() * 60UL + now.second()) * 1000UL;
    unsigned long startMs1 = timeToMillis(start1);
    unsigned long stopMs1  = timeToMillis(stop1);
    unsigned long startMs2 = timeToMillis(start2);
    unsigned long stopMs2  = timeToMillis(stop2);
    bool dentroDoIntervalo1, dentroDoIntervalo2;
    if (stopMs1 >= startMs1) {
      dentroDoIntervalo1 = (nowMs >= startMs1 && nowMs <= stopMs1);
    } else {
      // Passa da meia-noite
      dentroDoIntervalo1 = (nowMs >= startMs1 || nowMs <= stopMs1);
    }
    if (stopMs2 >= startMs2) {
      dentroDoIntervalo2 = (nowMs >= startMs2 && nowMs <= stopMs2);
    } else {
      dentroDoIntervalo2 = (nowMs >= startMs2 || nowMs <= stopMs2);
    }
    bool dentroDoIntervalo = dentroDoIntervalo1 || dentroDoIntervalo2;
    digitalWrite(LED_VERDE, dentroDoIntervalo ? HIGH : LOW);
    digitalWrite(RELAY_PIN, dentroDoIntervalo);
    // bool dentroDoIntervalo = ((nowMs >= startMs1 && nowMs <= stopMs1) || (nowMs >= startMs2 && nowMs <= stopMs2) && waterBuoy);
    printDistance();
    Serial.println("\n");
  }
}
long readDistance() {
  // Trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // Echo
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30 ms
  // Converter para cm
  long distance = duration / 58;
  return distance;
}

void printDistance(){
  long d = readDistance();

  if (d == 0) {
    Serial.println("[!] Sem leitura do nivel da água ou fora do alcance. \n[!] Verifique o sensor se está conectado!");
  } else {
    Serial.print("\n[*] Distância: ");
    Serial.print(d);
    Serial.println(" cm");
  }
}
