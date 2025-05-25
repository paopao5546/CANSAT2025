/*
 * LoRa Sender with MPU6050 Sensor Data (Corrected Units)
 * Reads MPU6050 data and transmits via LoRa
 * Accelerometer in m/s² with gravity correction
 * Compatible with ESP32
 */

#include <SPI.h>
#include <LoRa.h>
#include "Wire.h"
#include <MPU6050_light.h>

// ============ LoRa Config ================
#define ss 5
#define rst 14
#define dio0 2

// ============ MPU6050 Config =============
MPU6050 mpu(Wire);

// ============ Constants ==================
const float GRAVITY = 9.80665; // Standard gravity in m/s²

// ============ Timing Variables ===========
long timer = 0;
int counter = 0;

void setup() {
  Serial.begin(115200);
  
  // --- MPU6050 Init ---
  Wire.begin();
  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  
  if(status != 0) {
    Serial.println("MPU6050 connection failed!");
    while(1); // Stop if MPU6050 failed
  }
  
  Serial.println(F("Calculating MPU6050 offsets, do not move sensor"));
  delay(1000);
  mpu.calcOffsets(true, true); // gyro and accelerometer
  Serial.println("MPU6050 Ready!");

  // --- LoRa Init ---
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(922.525E6)) {
    Serial.println("LoRa init failed. Trying again...");
    delay(500);
  }
  
  // Set LoRa parameters for better range and reliability
  LoRa.setSignalBandwidth(125E3);    // 125 KHz bandwidth
  LoRa.setSpreadingFactor(9);        // SF9
  LoRa.setCodingRate4(5);            // 4/5 coding rate
  LoRa.setSyncWord(0xF3);            // Sync word
  LoRa.enableCrc();                  // Enable CRC
  LoRa.setTxPower(20);               // Maximum power (20dBm)
  
  Serial.println("LoRa Configuration:");
  Serial.println("- Frequency: 922.525 MHz");
  Serial.println("- Bandwidth: 125 KHz");
  Serial.println("- Spreading Factor: 9");
  Serial.println("- Coding Rate: 4/5");
  Serial.println("- TX Power: 20 dBm");
  Serial.println("LoRa Initializing OK!");
  Serial.println("=== System Ready ===\n");
}

void loop() {
  mpu.update();

  // Send data every 1 second (more frequent updates)
  if(millis() - timer > 1000) {
    // Convert accelerometer data from g to m/s²
    // MPU6050 gives data in g units, multiply by 9.80665 to get m/s²
    // Keep Z-axis normal - positive when sensor faces up (normal position)
    float accX_ms2 = mpu.getAccX() * GRAVITY;        // X-axis in m/s²
    float accY_ms2 = mpu.getAccY() * GRAVITY;        // Y-axis in m/s²
    float accZ_ms2 = mpu.getAccZ() * GRAVITY;        // Z-axis normal direction
    
    // Gyroscope data remains in degrees per second
    float gyroX_dps = mpu.getGyroX();
    float gyroY_dps = mpu.getGyroY();
    float gyroZ_dps = mpu.getGyroZ();

    // Create data string to send
    String dataPacket = "";
    dataPacket += "PKT:" + String(counter) + "|";
    dataPacket += "ACC:" + String(accX_ms2, 3) + "," + String(accY_ms2, 3) + "," + String(accZ_ms2, 3) + "|";
    dataPacket += "GYRO:" + String(gyroX_dps, 2) + "," + String(gyroY_dps, 2) + "," + String(gyroZ_dps, 2);

    // Send via LoRa
    Serial.print("Sending packet #");
    Serial.println(counter);
    Serial.println("Data: " + dataPacket);
    Serial.print("Data length: ");
    Serial.println(dataPacket.length());
    
    LoRa.beginPacket();
    LoRa.print(dataPacket);
    int result = LoRa.endPacket();
    
    if (result == 1) {
      Serial.println("✓ Packet sent successfully");
    } else {
      Serial.println("✗ Failed to send packet");
    }
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    // Display data on Serial Monitor
    Serial.println("--- Sensor Data ---");
    Serial.println("Raw Accelerometer (g): X=" + String(mpu.getAccX(), 3) + " Y=" + String(mpu.getAccY(), 3) + " Z=" + String(mpu.getAccZ(), 3));
    Serial.println("Converted Accelerometer (m/s²): X=" + String(accX_ms2, 3) + " Y=" + String(accY_ms2, 3) + " Z=" + String(accZ_ms2, 3));
    Serial.println("Gyroscope (°/s): X=" + String(gyroX_dps, 2) + " Y=" + String(gyroY_dps, 2) + " Z=" + String(gyroZ_dps, 2));
    Serial.println("Note: Z-axis shows +9.81 m/s² when sensor is placed flat (chip facing up)");
    Serial.println("==================\n");
    
    counter++;
    timer = millis();
  }
}
