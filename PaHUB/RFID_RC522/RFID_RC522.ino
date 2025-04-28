#include <M5StickCPlus2.h>

#include <MFRC522_I2C.h>

MFRC522_I2C mfrc522(0x28); // Create MFRC522 instance.  创建MFRC522实例

void setup() {
    M5.begin();             // Init M5Stack.  初始化M5Stack
    M5.Power.begin() ; // Init power  初始化电源模块
    Serial.begin(115200);   // Start serial communication at 115200 baud
    Wire.begin();           // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线

    mfrc522.PCD_Init();     // Init MFRC522.  初始化 MFRC522
    Serial.println("MFRC522 Test");
    Serial.println("Plc:\Users\sebaxo635\Desktop\Sensores-M5stack\PaHUB\PaHUB.inoease put the card\n\nUID:");
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() ||
        !mfrc522.PICC_ReadCardSerial()) {  //如果没有读取到新的卡片
        delay(200);
        
    }

    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {  // Output the stored UID data.  将存储的UID数据输出
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println("");
}