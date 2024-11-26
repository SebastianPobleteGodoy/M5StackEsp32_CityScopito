#include <M5Unified.h>
#include <ClosedCube_TCA9548A.h>
#include <MFRC522_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define PaHub_I2C_ADDRESS 0x70

MFRC522 mfrc522(0x28);
ClosedCube::Wired::TCA9548A tca9548a;

int tarjetas[6]; // Para los canales 6 al 11
String uids[6];

const char* ssid = "Administrativos";
const char* password = "CLBBMIT!!";
const char* postURL = "http://192.168.31.48:8000/process-json";

// Variables para manejar la lectura de UID
bool readUIDs = true;

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

    // Inicializar los lectores RFID para los canales 6 al 11
    for (int i = 6; i < 12; i++) {
        tca9548a.selectChannel(i - 6);  // Ajustar índice para los lectores 6-11
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
  tca9548a.selectChannel(channel - 6);  // Ajustar para los canales 6-11
  delay(10);
  
  bool cardDetected = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      cardDetected = true;

      String uid = getUIDString();
      int value = mapUIDToValue(uid);

      tarjetas[channel - 6] = value;  // Almacenar valor para canales 6-11
      uids[channel - 6] = uid;
      
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
    tarjetas[channel - 6] = 0;
    uids[channel - 6] = "";
  }

  delay(10);
}

String createJSON() {
  DynamicJsonDocument doc(1024);  // Tamaño del buffer para el JSON

  for (int i = 6; i < 12; i++) {
    int value = tarjetas[i - 6];  // Ajustar para canales 6-11
    String uid = uids[i - 6];

    if (value != 0 || !uid.isEmpty()) {  // Incluir en el JSON si el valor es diferente de 0 o si hay un UID
      doc[String("canal_") + i] = value;  // Almacenar valores de los canales 6-11
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
    for (int i = 6; i < 12; i++) {
        if (tarjetas[i - 6] != 0 || !uids[i - 6].isEmpty()) {
            anyUIDDetected = true;
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.print("Canal ");
            M5.Lcd.print(i);
            M5.Lcd.print(": UID = ");
            M5.Lcd.println(uids[i - 6]);
        }
    }

    if (!anyUIDDetected) {
        M5.Lcd.clear(BLACK);  // Dejar la pantalla en blanco si no se detectan UIDs
    }
}

void loop() {
    M5.update();

    if (readUIDs) {
        bool allChannelsDetected = true;
        for (int i = 6; i < 12; i++) {
            checkRFIDOnChannel(i);  // Leer canales 6 al 11
        }

        updateScreen(); // Actualizar la pantalla con el estado de los canales

        for (int i = 6; i < 12; i++) {
            if (tarjetas[i - 6] == 0 && uids[i - 6].isEmpty()) {
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

        delay(10); // Evitar sobrecarga del CPU
    }
}