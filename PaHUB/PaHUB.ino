#include <M5Unified.h>
#include <M5StackMenuSystem.h>
#include "ClosedCube_TCA9548A.h"
#include "MFRC522_I2C.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>   
#define PaHub_I2C_ADDRESS 0x70

MFRC522 mfrc522(0x28);
ClosedCube::Wired::TCA9548A tca9548a;
 
int tarjetas[6];
String uids[6];

const char* ssid = "Administrativos";
const char* password = "CLBBMIT!!";
//const char* postURL = "http://192.168.31.44:8000/process-json";
//const char* getURL = "http://192.168.31.44:8000/categoria/";
const char* serverHost = "citylab.local";   // hostname mDNS de tu Raspberry
const int   serverPort = 8000;

String buildURL(const String& path) {
  return String("http://") + serverHost + ":" + serverPort + path;
}

Menu mainMenu("Seleccione Mapa ");

// Variables para manejar el estado del menú y la lectura de UID
bool inMenu = false;
bool readUIDs = true;

int mapUIDToValue(String uid) {
  if (uid == "04CC5D3CBB2A81") return 1;// ok plan 6 futuro
  if (uid == "042EF304BB2A81") return 1;//ok plan 4 futuro
  if (uid == "04A1F304BB2A81") return 1;//ok plan 4 actual 
  if (uid == "04CAF304BB2A81") return 1;//ok plan 2 actual 
  if (uid == "0435F404BB2A81") return 1;//ok plan 6 actual  
  if (uid == "0416F404BB2A81") return 1;//ok plan 1 futuro
  if (uid == "049EF304BB2A81") return 0;//ok plan 5 futuro
  if (uid == "042FF304BB2A81") return 0;//ok plan 3 actual
  if (uid == "049FF304BB2A81") return 0;//ok plan  5 actual
  if (uid == "04D35D3CBB2A81") return 0;//ok plan 1 actual 
  if (uid == "04CB5D3CBB2A81") return 0;//ok plan 2 futuro
  if (uid == "043AF304BB2A81") return 0;//ok plan 3 futuro 
  return 0;
}

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);
    tca9548a.address(PaHub_I2C_ADDRESS);
    Serial.println("PaHUB Example");

    connectToWiFi();

    tca9548a.address(PaHub_I2C_ADDRESS);
    for (int i = 0; i < 6; i++) {
        tca9548a.selectChannel(i);
        mfrc522.PCD_Init();
        Serial.println("Lector " + String(i) + " iniciado");
    }

    mainMenu.addMenuItem("tramites", handleMenuItem);
    mainMenu.addMenuItem("servicios", handleMenuItem);
    mainMenu.addMenuItem("comercio", handleMenuItem);
    mainMenu.addMenuItem("comida_abastecimiento", handleMenuItem);
    mainMenu.addMenuItem("comida_servir", handleMenuItem);
    mainMenu.addMenuItem("deportes_privado", handleMenuItem);
    mainMenu.addMenuItem("deportes_publico", handleMenuItem);
    mainMenu.addMenuItem("salud_privada", handleMenuItem);
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

    if (!MDNS.begin("m5rfid")) {
      Serial.println("⚠️  mDNS no pudo iniciar");
    } else {
      Serial.println("mDNS iniciado: http://m5rfid.local");
    }

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

void handleMenuItem(CallbackMenuItem& menuItem) {
    M5.Lcd.clear(BLACK);
    mainMenu.disable();
    inMenu = false; // Regresar a la lectura de UID

    String item = menuItem.getText();
    String url = buildURL("/categoria/" + item);
    
    // Realizar la solicitud GET 
    sendGetRequest(url);
}

void sendGetRequest(String url) {
    HTTPClient http;
    http.begin(url);
    http.GET(); // Enviar la solicitud GET sin esperar la respuesta
    http.end(); // Finalizar la conexión sin esperar la respuesta
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

      tarjetas[channel] = value;
      uids[channel] = uid;
      
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
    tarjetas[channel] = 0;
    uids[channel] = "";
  }

  delay(10);
}

String createJSON() {
  DynamicJsonDocument doc(1024);  // Tamaño del buffer para el JSON

  for (int i = 0; i < 6; i++) {
    int value = tarjetas[i];  
    String uid = uids[i];

    if (value != 0 || !uid.isEmpty()) {  // Incluir en el JSON si el valor es diferente de 0 o si hay un UID
      doc[String("canal_") + i] = value;  // Asignar el valor como entero
    }
  }

  String json;
  serializeJson(doc, json);
  return json;
}

void sendPostRequest(String json) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(buildURL("/process-json")); 
    http.addHeader("Content-Type", "application/json");

    Serial.println("Enviando solicitud POST...");
    Serial.println("JSON a enviar:");
    Serial.println(json);

    http.POST(json);
    http.end(); // Finaliza la conexión, sin esperar respuesta
  } else {
    Serial.println("Error: No conectado a Wi-Fi.");
  }
}

void updateScreen() {
    M5.Lcd.clear(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextSize(2);

    bool anyUIDDetected = false;
    for (int i = 0; i < 6; i++) {
        if (tarjetas[i] != 0 || !uids[i].isEmpty()) {
            anyUIDDetected = true;
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.print("Canal ");
            M5.Lcd.print(i);
            M5.Lcd.print(": UID = ");
            M5.Lcd.println(uids[i]);
        }
    }

    if (!anyUIDDetected) {
        M5.Lcd.clear(BLACK);  // Dejar la pantalla en blanco si no se detectan UIDs
    }
}

void loop() {
    M5.update();

    if (mainMenu.isEnabled()) {
        mainMenu.loop();
    } else {
        if (readUIDs) {
            bool allChannelsDetected = true;
            for (int i = 0; i < 6; i++) {
                checkRFIDOnChannel(i);
            }

            updateScreen(); // Actualizar la pantalla con el estado de los canales

            for (int i = 0; i < 6; i++) {
                if (tarjetas[i] == 0 && uids[i].isEmpty()) {
                    allChannelsDetected = false;
                    break;
                }
            }

            if (allChannelsDetected) {
                String json = createJSON();
                Serial.println("JSON de UIDs:");
                Serial.println(json);

                sendPostRequest(json);
            } else {
                Serial.println("No se detectaron UIDs en todos los canales.");
            }
        }

        delay(10); // Evitar sobrecarga del CPU
    }
}