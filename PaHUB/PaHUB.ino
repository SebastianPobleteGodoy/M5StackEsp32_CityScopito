#include <M5Stack.h>
#include "ClosedCube_TCA9548A.h"
#include "MFRC522_I2C.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define PaHub_I2C_ADDRESS 0x70

MFRC522 mfrc522(0x28);
ClosedCube::Wired::TCA9548A tca9548a;

int tarjetas[2][3];
String uids[2][3];

const char* ssid = "Administrativos";
const char* password = "CLBBMIT!!";
const char* postURL = "http://192.168.31.234:8000/process-json";

int mapUIDToValue(String uid) {
  if (uid == "04CA5D3CBB2A81") return 1;
  if (uid == "042EF304BB2A81") return 1;
  if (uid == "04A1F304BB2A81") return 1;
  if (uid == "04CBF304BB2A81") return 1;
  if (uid == "0436F404BB2A81") return 1;
  if (uid == "0416F404BB2A81") return 1;
  if (uid == "049EF304BB2A81") return 0;
  if (uid == "042FF304BB2A81") return 0;
  if (uid == "049FF304BB2A81") return 0;
  if (uid == "04D35D3CBB2A81") return 0;
  if (uid == "04CB5D3CBB2A81") return 0;
  if (uid == "043AF304BB2A81") return 0;
  return 0;
}

void setup() {
  M5.begin();
  M5.Power.begin();
  Wire.begin();
  Serial.begin(115200);
  tca9548a.address(PaHub_I2C_ADDRESS);
  Serial.println("PaHUB Example");

  connectToWiFi();

  for (int i = 0; i < 6; i++) {
    tca9548a.selectChannel(i);
    mfrc522.PCD_Init();
    Serial.println("Lector " + String(i) + " iniciado");
  }
}

void connectToWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("Conectado a Wi-Fi.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.print("Conectado a Wi-Fi");
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("IP: ");
  M5.Lcd.print(WiFi.localIP());
}

String getUIDString() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void checkRFIDOnChannel(int channel) {
  tca9548a.selectChannel(channel);
  delay(10);
  
  bool cardDetected = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      cardDetected = true;

      String uid = getUIDString();
      int value = mapUIDToValue(uid);

      if (channel < 3) {
        tarjetas[0][channel] = value;
        uids[0][channel] = uid;
      } else {
        tarjetas[1][channel - 3] = value;
        uids[1][channel - 3] = uid;
      }
      
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(": UID detectado = ");
      Serial.println(uid);
      Serial.print("Valor asignado = ");
      Serial.println(value);
      
      break;
    }
  }

  if (!cardDetected) {
    if (channel < 3) {
      tarjetas[0][channel] = 0;
      uids[0][channel] = "";
    } else {
      tarjetas[1][channel - 3] = 0;
      uids[1][channel - 3] = "";
    }
  }

  delay(10);
}

String createJSON() {
  DynamicJsonDocument doc(1024);  // Tamaño del buffer para el JSON
  int start_channel = 6;
  for (int i = 0; i < 6; i++) {
    int channel_index = start_channel + i;
    int row = i / 3;
    int col = i % 3;
    int value = tarjetas[row][col];
    String uid = uids[row][col];

    if (value != 0 || !uid.isEmpty()) {  // Incluir en el JSON si el valor es diferente de 0 o si hay un UID
      doc[String("canal_") + channel_index] = value;
    }
  }

  String json;
  serializeJson(doc, json);
  return json;
}

void sendPostRequest(String json) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(postURL);
    http.addHeader("Content-Type", "application/json");

    Serial.println("Enviando solicitud POST...");
    Serial.println("JSON a enviar:");
    Serial.println(json);

    // Enviar la solicitud POST sin esperar la respuesta
    http.POST(json);
    http.end(); // Finaliza la conexión, sin esperar respuesta
  } else {
    Serial.println("Error: No conectado a Wi-Fi.");
  }
}

void loop() {
  bool allChannelsDetected = true; // Asumir que todos los canales han detectado un UID
  
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      tarjetas[i][j] = 0;
      uids[i][j] = "";
    }
  }

  for (int i = 0; i < 6; i++) {
    checkRFIDOnChannel(i);
  }
  
  // Verificar si todos los canales han detectado un UID
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      if (tarjetas[i][j] == 0 && uids[i][j].isEmpty()) {
        allChannelsDetected = false;
        break;
      }
    }
    if (!allChannelsDetected) break;
  }
  
  if (allChannelsDetected) {
    String json = createJSON();
    Serial.println("JSON de UIDs:");
    Serial.println(json);

    sendPostRequest(json);
  } else {
    Serial.println("No se han detectado todos los UIDs.");
  }

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      int state = tarjetas[i][j];

      int x = 50 + j * 100;
      int y = 90 + i * 30;

      M5.Lcd.setCursor(x, y);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.print(state);
    }
    Serial.println();
  }

  delay(10); // Espera más tiempo para evitar demasiadas solicitudes GET
}

