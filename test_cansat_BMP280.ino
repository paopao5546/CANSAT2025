#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_BMP280.h> // ใช้สำหรับ BMP280

// ==== LoRa Pin Definitions ====
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// ==== BMP280 Config ====
Adafruit_BMP280 bmp; // ใช้ I2C โดย default (SDA/SCL)

// ==== Variables ====
float temperature = 0.0;
float pressure = 0.0;
float altitude = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa + BMP280 Sender Initializing...");

  // ==== BMP280 Init ====
  if (!bmp.begin(0x76)) {  // หากไม่ทำงานให้ลอง 0x77 แทน
    Serial.println("BMP280 not found. Check wiring!");
    while (1);
  }

  // ==== LoRa Init ====
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed...");
    delay(1000);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa initialized.");
}

void loop() {
  // อ่านข้อมูลจาก BMP280
  temperature = bmp.readTemperature();       // °C
  pressure = bmp.readPressure() / 100.0F;    // hPa
  altitude = bmp.readAltitude(1013.25);      // คำนวณจากความดันระดับน้ำทะเลมาตรฐาน

  // แสดงข้อมูลใน Serial Monitor
  Serial.print("T: ");
  Serial.print(temperature);
  Serial.print(" °C | H: ");
  Serial.print(pressure);
  Serial.print(" hPa | A: ");
  Serial.print(altitude);
  Serial.println(" m");

  // สร้างข้อความส่งไปยัง Receiver
  String data = "T:" + String(temperature, 2) +
                ",H:" + String(pressure, 2) +
                ",A:" + String(altitude, 2);

  // ส่งข้อมูลผ่าน LoRa
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  delay(2000); // ส่งทุก 2 วินาที
}
