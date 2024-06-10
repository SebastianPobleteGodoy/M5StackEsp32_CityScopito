#include <M5Stack.h>
#include "ClosedCube_TCA9548A.h"
#include <MFRC522_I2C.h>
#define PaHub_I2C_ADDRESS 0x70

MFRC522 mfrc522(0x28); // Crea las instancias del sensor RFID
ClosedCube::Wired::TCA9548A tca9548a; // Crea el objeto para poder usar el multiplexor

// Definir una matriz 3x2 para almacenar los UIDs de las tarjetas RFID detectadas
String tarjetas[3][2];

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
      // Almacena el UID de la tarjeta en la matriz
      String uid = "";
      for (byte j = 0; j < mfrc522.uid.size; j++) {
        uid += (mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " ");
        uid += String(mfrc522.uid.uidByte[j], HEX);
      }

        for(int j = 0; j < 3; j++){
          for(int k = 0; k < 2; k++){
            if(j*2 + k == channel) { // Solo asigna el UID en la posición correspondiente al canal actual
            tarjetas[j][k] = uid;
        }
    }
}

    
    }
  }

  if (!cardDetected) {
    // Si no se detecta ninguna tarjeta, deja el espacio en blanco en la matriz
    tarjetas[channel/2][channel%2] = "";
  }

  delay(10); // Espera antes de cambiar al siguiente canal
}

void loop() {
  // Reinicia la matriz en cada iteración del bucle principal
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      tarjetas[i][j] = "";
    }
  }

  // Lee las tarjetas en cada canal y almacena la información en la matriz
  for (int i = 0; i < 6; i++) {
    checkRFIDOnChannel(i);
  }
  
  // Imprime la matriz en el Serial.print
  Serial.println("Matriz de UIDs:");
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      Serial.print("(");
      Serial.print(tarjetas[i][j]);
      Serial.print(")");
    }
    Serial.println();
  }

  delay(15); // Espera antes de volver a iniciar el ciclo
}



