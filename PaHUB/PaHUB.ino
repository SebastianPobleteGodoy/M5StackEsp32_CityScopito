#include <M5Stack.h>
#include "ClosedCube_TCA9548A.h"
#include <MFRC522_I2C.h>
#define PaHub_I2C_ADDRESS 0x70

MFRC522 mfrc522(0x28); // Crea las instancias del sensor RFID
ClosedCube::Wired::TCA9548A tca9548a; // Crea el objeto para poder usar el multiplexor

void setup() {
  M5.begin();
  M5.Power.begin();
  Wire.begin();
  Serial.begin(115200);
  tca9548a.address(PaHub_I2C_ADDRESS); // Establece la dirección I2C.
  Serial.println("PaHUB Example");
  for (int i = 0; i < 6; i++) {
    tca9548a.selectChannel(i);
    mfrc522.PCD_Init();
    Serial.println("Lector " + String(i) + " iniciado");
  }
}

void checkRFIDOnChannel(int channel) {
  tca9548a.selectChannel(channel); // Selecciona el canal especificado
  delay(10); // Asegura un pequeño retraso para que el canal se estabilice
  
  bool cardDetected = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) { // Verifica si hay una nueva tarjeta y lee el serial
      cardDetected = true;
      Serial.print("Canal ");
      Serial.print(channel);
      Serial.print(": UID de tarjeta detectada: ");
      for (byte j = 0; j < mfrc522.uid.size; j++) {
        Serial.print(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[j], HEX);
      }
      Serial.println();
      break;
    }
    
  }

  if (!cardDetected) {
    Serial.print("Canal ");
    Serial.print(channel);
    Serial.println(": No se detectó tarjeta.");
  }

  delay(10); // Espera antes de cambiar al siguiente canal
}

void loop() {
  for (int i = 0; i < 6; i++) {
    checkRFIDOnChannel(i);
  }
  delay(30); // Espera antes de volver a iniciar el ciclo
}



