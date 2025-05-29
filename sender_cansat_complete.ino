#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "Wire.h"
#include <MPU6050_light.h>
#include <Adafruit_BMP280.h> 

//---------- LoRa -----------
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2

//----------- GPS -----------
#define GPS_RX 16  
#define GPS_TX 17  
TinyGPSPlus gps;
HardwareSerial GPSSerial(1); 
float latitude = 0.0;
float longitude = 0.0;
int satellites = 0;
float gps_altitude = 0.0;
float speed_kmph = 0.0;

//----------- MPU -------------
MPU6050 mpu(Wire);
const float GRAVITY = 9.80665;

//-------------BMP ------------
Adafruit_BMP280 bmp;
float temperature = 0.0;
float pressure = 0.0;
float bmp_altitude = 0.0;

//----------- System -----------
unsigned long lastSendTime = 0;
const int sendInterval = 2000;
int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  //------------- I2C Scanner --------------
  Wire.begin();
  Serial.println("=== I2C Device Scanner ===");
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at address 0x");
      Serial.println(i, HEX);
    }
  }
  
  //------------- MPU6050 --------------
  Serial.println("=== MPU6050 Initialization ===");
  byte status = mpu.begin();
  Serial.print("MPU6050 status: ");
  Serial.println(status);
  
  if (status == 0) {
    Serial.println("✓ MPU6050 initialized successfully!");
    Serial.println("Calibrating MPU6050... Keep sensor still!");
    mpu.calcOffsets(true, true);
    Serial.println("✓ MPU6050 calibration complete!");
  } else {
    Serial.println("✗ MPU6050 initialization failed!");
    Serial.println("Check wiring: SDA->21, SCL->22, VCC->3.3V, GND->GND");
  }

  // -------------- GPS -------------
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("✓ GPS Initialized");

  //-------------- BMP280 ---------------
  Serial.println("=== BMP280 Initialization ===");
  if (!bmp.begin(0x76)) {
    Serial.println("Trying BMP280 at address 0x77...");
    if (!bmp.begin(0x77)) {
      Serial.println("✗ BMP280 not found. Check wiring!");
      while (1);
    }
  }
  Serial.println("✓ BMP280 initialized successfully!");

  //----------------LoRa ------------
  Serial.println("=== LoRa Initialization ===");
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  while (!LoRa.begin(922.525E6)) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("✓ LoRa Initializing OK!");
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(9);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  
  Serial.println("=== System Ready ===\n");
}

void loop() {
  // -------------- BMP280 ---------------------
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F;
  bmp_altitude = bmp.readAltitude(1013.25);

  //--------------- GPS --------------------
  while (GPSSerial.available() > 0) {
    if (gps.encode(GPSSerial.read())) {
      updateGPSData();
    }
  }

  //-------------- MPU6050 --------------------
  mpu.update();
  
  // Raw acceleration (in g-force)
  float accX_g = mpu.getAccX();
  float accY_g = mpu.getAccY();
  float accZ_g = mpu.getAccZ();
  
  // Convert to m/s²
  float accX_ms2 = accX_g * GRAVITY;
  float accY_ms2 = accY_g * GRAVITY;
  float accZ_ms2 = accZ_g * GRAVITY;
  
  // Gyroscope data (degrees per second)
  float gyroX_dps = mpu.getGyroX();
  float gyroY_dps = mpu.getGyroY();
  float gyroZ_dps = mpu.getGyroZ();
  
  // Z-axis direction analysis
  String zDirection = analyzeZDirection(accZ_ms2);
  
  // Calculate total acceleration magnitude
  float totalAccel = sqrt(accX_ms2*accX_ms2 + accY_ms2*accY_ms2 + accZ_ms2*accZ_ms2);

  // -------------- Serial Display ----------------
  Serial.println("=== Sensor Readings ===");
  Serial.printf("BMP280 -> T: %.2f°C | P: %.2f hPa | Alt: %.2fm\n", 
                temperature, pressure, bmp_altitude);
  
  Serial.printf("MPU6050 Acceleration (m/s²):\n");
  Serial.printf("  X: %7.3f | Y: %7.3f | Z: %7.3f (%.3fg)\n", 
                accX_ms2, accY_ms2, accZ_ms2, accZ_g);
  Serial.printf("  Z-Direction: %s\n", zDirection.c_str());
  Serial.printf("  Total Magnitude: %.3f m/s²\n", totalAccel);
  
  Serial.printf("MPU6050 Gyroscope (°/s):\n");
  Serial.printf("  X: %7.2f | Y: %7.2f | Z: %7.2f\n", 
                gyroX_dps, gyroY_dps, gyroZ_dps);

  // --------------------- LoRa Transmission ----------------
  String data = "T:" + String(temperature, 2) +
                ",P:" + String(pressure, 2) +
                ",BA:" + String(bmp_altitude, 2) + "|";
  
  // GPS Data
  if (gps.location.isValid()) {
    data += "GPS:" + String(latitude, 6) + "," + String(longitude, 6) + "," + 
            String(satellites) + "," + String(gps_altitude, 1) + "," + 
            String(speed_kmph, 1) + "|";
  } else {
    data += "GPS:NO_FIX|";
  }
  
  // Packet counter
  data += "PKT:" + String(counter) + "|";
  
  // MPU6050 Data
  data += "ACC:" + String(accX_ms2, 3) + "," + String(accY_ms2, 3) + "," + String(accZ_ms2, 3) + "|";
  data += "GYRO:" + String(gyroX_dps, 2) + "," + String(gyroY_dps, 2) + "," + String(gyroZ_dps, 2) + "|";
  data += "ZDIR:" + zDirection + "|";
  data += "TMAG:" + String(totalAccel, 3);

  Serial.println("\n=== LoRa Transmission ===");
  Serial.println("Data: " + data);
  Serial.printf("Length: %d characters\n", data.length());

  LoRa.beginPacket();
  LoRa.print(data);
  int result = LoRa.endPacket();
  
  if (result == 1) {
    Serial.println("✓ Packet sent successfully");
  } else {
    Serial.println("✗ Failed to send packet");
  }
  
  Serial.printf("RSSI: %d dBm\n", LoRa.packetRssi());
  Serial.println("========================\n");

  counter++;
  delay(2000);
}

//---------------- GPS Functions ---------------
void updateGPSData() {
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }
  
  if (gps.satellites.isValid()) {
    satellites = gps.satellites.value();
  }
  
  if (gps.altitude.isValid()) {
    gps_altitude = gps.altitude.meters();
  }
  
  if (gps.speed.isValid()) {
    speed_kmph = gps.speed.kmph();
  }
}

//---------------- Z-Axis Direction Analysis ---------------
String analyzeZDirection(float accZ_ms2) {
  const float THRESHOLD = 2.0; // ค่าขีดจำกัดสำหรับการตัดสินใจ
  
  if (accZ_ms2 > (GRAVITY + THRESHOLD)) {
    return "UP_ACCEL";        // กำลังเร่งขึ้น (เกิน 1g มาก)
  }
  else if (accZ_ms2 > (GRAVITY - THRESHOLD) && accZ_ms2 <= (GRAVITY + THRESHOLD)) {
    return "STABLE";          // อยู่นิ่งหรือเคลื่อนที่ด้วยความเร็วคงที่
  }
  else if (accZ_ms2 > 0 && accZ_ms2 <= (GRAVITY - THRESHOLD)) {
    return "FALLING_SLOW";    // กำลังตกแต่ยังมีแรงต้านอากาศ
  }
  else if (accZ_ms2 >= -2.0 && accZ_ms2 <= 0) {
    return "FREE_FALL";       // เกือบร่วงอิสระ
  }
  else if (accZ_ms2 < -2.0) {
    return "DOWN_ACCEL";      // กำลังเร่งลง (แรงกว่าแรงโน้มถ่วง)
  }
  
  return "UNKNOWN";
}
