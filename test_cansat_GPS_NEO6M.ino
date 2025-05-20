/*********
  Modified from Random Nerd Tutorials
  GPS Module NEO-6M integration with LoRa communication
  Uses TinyGPS++ for parsing GPS data
*********/

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// กำหนดขา LoRa
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2

// กำหนดขา GPS (UART)
#define GPS_RX 16  // ต่อกับขา TX ของ GPS
#define GPS_TX 17  // ต่อกับขา RX ของ GPS (อาจไม่จำเป็นต้องใช้ ถ้าเราเพียงรับข้อมูลอย่างเดียว)

// ประกาศตัวแปร GPS
TinyGPSPlus gps;
HardwareSerial GPSSerial(1); // ใช้ UART1 สำหรับการเชื่อมต่อ GPS

// ตัวแปรสำหรับเก็บค่า GPS
float latitude = 0.0;
float longitude = 0.0;
int satellites = 0;
float altitude = 0.0;
float speed_kmph = 0.0;

unsigned long lastSendTime = 0;
const int sendInterval = 2000;  // ส่งข้อมูลทุกๆ 2 วินาที
int counter = 0;

void setup() {
  // เริ่มต้น Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa GPS Sender");

  // เริ่มต้นการเชื่อมต่อกับ GPS
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS Initialized");

  // ตั้งค่า LoRa transceiver
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  
  // เริ่มต้น LoRa ที่ความถี่ 915MHz สำหรับอเมริกาเหนือ (ปรับตามภูมิภาคของคุณ)
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  
  // ตั้งค่า sync word (0xF3) ให้ตรงกับตัวรับ
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  // อ่านข้อมูลจาก GPS
  while (GPSSerial.available() > 0) {
    if (gps.encode(GPSSerial.read())) {
      updateGPSData();
    }
  }

  // ตรวจสอบว่าถึงเวลาส่งข้อมูลหรือยัง
  if (millis() - lastSendTime > sendInterval) {
    sendLoRaData();
    lastSendTime = millis();
  }
}

void updateGPSData() {
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }
  
  if (gps.satellites.isValid()) {
    satellites = gps.satellites.value();
  }
  
  if (gps.altitude.isValid()) {
    altitude = gps.altitude.meters();
  }
  
  if (gps.speed.isValid()) {
    speed_kmph = gps.speed.kmph();
  }
}

void sendLoRaData() {
  // สร้างข้อความที่จะส่ง
  String message = "";
  
  if (gps.location.isValid()) {
    message = String(latitude, 6) + "," + String(longitude, 6) + "," + 
              String(satellites) + "," + String(altitude, 1) + "," + 
              String(speed_kmph, 1) + "," + String(counter);
  } else {
    message = "NO_GPS," + String(counter);
  }
  
  Serial.print("Sending packet: ");
  Serial.println(message);

  // ส่งแพ็คเก็ต LoRa ไปยังตัวรับ
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  counter++;
}
