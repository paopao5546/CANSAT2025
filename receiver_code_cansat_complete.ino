#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// ============ WiFi Config ================
const char* ssid = "Zhengzoza";
const char* password = "12345678";
WiFiServer server(80);

// ============ LoRa Config ================
#define ss 5
#define rst 14
#define dio0 2

// ============ Sensor Variables ===========
// MPU6050
String lastPacketNum = "0";
String accelX = "0", accelY = "0", accelZ = "0";
String gyroX = "0", gyroY = "0", gyroZ = "0";
String zDirection = "UNKNOWN";
String totalMagnitude = "0";
unsigned long lastReceived = 0;

// GPS
String LoRaGPS = "Waiting for data...";
float latitude = 0.0;
float longitude = 0.0;
int satellites = 0;
float altitude = 0.0;
float speed_kmph = 0.0;
int counter = 0;
bool validGPS = false;

// BMP280
String LoRaBMP = "";
String temperature = "--";
String pressure = "--";
String bmpAltitude = "--";

// เพิ่มตัวแปร global สำหรับเก็บข้อมูล LoRa ล่าสุด
String lastReceivedData = "Waiting for data...";

// Web
String header = "";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== CanSat ESP32 Starting ===");

  // Initialize LoRa
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(922.525E6)) {
    Serial.println("LoRa init failed. Trying again...");
    delay(500);
  }
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(9);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  Serial.println("✅ LoRa Initialized successfully!");

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n=== WiFi Connected Successfully ===");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    Serial.println("=== API Endpoints ===");
    Serial.println("GET /data - Returns JSON sensor data");
    Serial.println("GET /status - Returns system status");
    Serial.println("=== CORS Headers Enabled for WebApp ===");
    Serial.println();
    
    server.begin();
    Serial.println("🌐 HTTP Server Started!");
    Serial.println("Ready to serve data to WebApp");
    Serial.println("==================================");
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    Serial.println("Starting in offline mode...");
  }
}

void loop() {
  checkWiFiStatus();

  // Check for LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    lastReceived = millis();
    lastReceivedData = received;
    Serial.println("📡 LoRa Data Received:");
    Serial.println("Raw: " + received);

    // Parse all sensor data
    parseAllData(received);

    Serial.println("✅ Data parsed and ready for WebApp");
    Serial.println("====================");
  }

  // Handle HTTP requests
  WiFiClient client = server.available();
  if (client) {
    handleHTTPClient(client);
  }
}

void handleHTTPClient(WiFiClient client) {
  currentTime = millis();
  previousTime = currentTime;
  String currentLine = "";
  header = "";

  Serial.println("📱 WebApp client connected");

  while (client.connected() && currentTime - previousTime <= timeoutTime) {
    currentTime = millis();

    if (client.available()) {
      char c = client.read();
      header += c;

      if (c == '\n') {
        if (currentLine.length() == 0) {
          // Handle different endpoints
          if (header.indexOf("GET /data") >= 0) {
            sendJSONData(client);
          } else if (header.indexOf("GET /status") >= 0) {
            sendStatusData(client);
          } else {
            sendNotFound(client);
          }
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  header = "";
  client.stop();
  Serial.println("📱 WebApp client disconnected");
}

void sendJSONData(WiFiClient client) {
  Serial.println("📤 Sending JSON data to WebApp");

  // Send HTTP headers with CORS
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Connection: close");
  client.println();

  // Create JSON response
  StaticJsonDocument<1024> jsonDoc;
  
  // GPS data
  JsonObject gps = jsonDoc.createNestedObject("gps");
  gps["valid"] = validGPS;
  gps["latitude"] = String(latitude, 6);
  gps["longitude"] = String(longitude, 6);
  gps["altitude"] = String(altitude, 1);
  gps["satellites"] = String(satellites);
  gps["speed"] = String(speed_kmph, 1);

  // Accelerometer data
  JsonObject accelerometer = jsonDoc.createNestedObject("accelerometer");
  accelerometer["x"] = accelX;
  accelerometer["y"] = accelY;
  accelerometer["z"] = accelZ;

  // Gyroscope data
  JsonObject gyroscope = jsonDoc.createNestedObject("gyroscope");
  gyroscope["x"] = gyroX;
  gyroscope["y"] = gyroY;
  gyroscope["z"] = gyroZ;

  // BMP280 data
  JsonObject bmp280 = jsonDoc.createNestedObject("bmp280");
  bmp280["temperature"] = temperature;
  bmp280["pressure"] = pressure;
  bmp280["altitude"] = bmpAltitude;

  // Motion data
  JsonObject motion = jsonDoc.createNestedObject("motion");
  motion["zDirection"] = zDirection;
  motion["totalMagnitude"] = totalMagnitude;

  // System data
  jsonDoc["packet"] = lastPacketNum;
  jsonDoc["counter"] = String(counter);
  jsonDoc["rawData"] = lastReceivedData;
  
  // Calculate seconds since last received
  unsigned long secondsAgo = (millis() - lastReceived) / 1000;
  jsonDoc["lastReceived"] = String(secondsAgo) + " sec ago";

  // Send JSON
  serializeJson(jsonDoc, client);
  
  Serial.println("✅ JSON data sent successfully");
}

void sendStatusData(WiFiClient client) {
  Serial.println("📤 Sending status data to WebApp");

  // Send HTTP headers with CORS
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Connection: close");
  client.println();

  // Create status JSON
  StaticJsonDocument<512> statusDoc;
  statusDoc["system"] = "CanSat ESP32";
  statusDoc["version"] = "2.0";
  statusDoc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  statusDoc["wifi_ip"] = WiFi.localIP().toString();
  statusDoc["wifi_rssi"] = WiFi.RSSI();
  statusDoc["uptime_ms"] = millis();
  statusDoc["free_heap"] = ESP.getFreeHeap();
  statusDoc["lora_frequency"] = "922.525 MHz";
  
  unsigned long secondsAgo = (millis() - lastReceived) / 1000;
  statusDoc["last_lora_data"] = String(secondsAgo) + " sec ago";
  statusDoc["total_packets"] = counter;

  serializeJson(statusDoc, client);
  
  Serial.println("✅ Status data sent successfully");
}

void sendNotFound(WiFiClient client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  
  client.println("{\"error\": \"Endpoint not found\", \"available_endpoints\": [\"/data\", \"/status\"]}");
}

// ===== Helper: WiFi =====
void checkWiFiStatus() {
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000) { // Check every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("🔄 WiFi disconnected, reconnecting...");
      WiFi.begin(ssid, password);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi reconnected!");
        Serial.println("IP Address: " + WiFi.localIP().toString());
      } else {
        Serial.println("\n❌ WiFi reconnection failed");
      }
    }
    lastWiFiCheck = millis();
  }
}

// ===== แก้ไขฟังก์ชัน Parse ทั้งหมด =====
void parseAllData(String data) {
  Serial.println("🔍 Parsing sensor data...");
  
  // Parse BMP280 data (T:xx,P:xx,BA:xx)
  parseBMPData(data);
  
  // Parse GPS data (GPS:lat,lon,sat,alt,speed หรือ GPS:NO_FIX)
  parseGPSData(data);
  
  // Parse MPU6050 data (PKT:xx, ACC:xx,yy,zz, GYRO:xx,yy,zz)
  parseMPUData(data);
  
  // Parse Motion data (ZDIR:xx, TMAG:xx)
  parseMotionData(data);
}

void parseBMPData(String data) {
  // หา T:xx.xx,P:xx.xx,BA:xx.xx
  int tIndex = data.indexOf("T:");
  int pIndex = data.indexOf("P:");
  int baIndex = data.indexOf("BA:");
  
  if (tIndex >= 0) {
    int commaAfterT = data.indexOf(',', tIndex);
    if (commaAfterT > tIndex) {
      temperature = data.substring(tIndex + 2, commaAfterT);
      Serial.println("🌡️ Temperature: " + temperature + "°C");
    }
  }
  
  if (pIndex >= 0) {
    int commaAfterP = data.indexOf(',', pIndex);
    if (commaAfterP > pIndex) {
      pressure = data.substring(pIndex + 2, commaAfterP);
      Serial.println("🔘 Pressure: " + pressure + " hPa");
    }
  }
  
  if (baIndex >= 0) {
    int pipeAfterBA = data.indexOf('|', baIndex);
    if (pipeAfterBA > baIndex) {
      bmpAltitude = data.substring(baIndex + 3, pipeAfterBA);
    } else {
      // ถ้าไม่มี | หลัง BA แสดงว่าเป็นตัวสุดท้าย
      bmpAltitude = data.substring(baIndex + 3);
    }
    Serial.println("📏 BMP Altitude: " + bmpAltitude + " m");
  }
}

void parseGPSData(String data) {
  int gpsIndex = data.indexOf("GPS:");
  if (gpsIndex >= 0) {
    int pipeIndex = data.indexOf('|', gpsIndex);
    String gpsData;
    
    if (pipeIndex > gpsIndex) {
      gpsData = data.substring(gpsIndex + 4, pipeIndex);
    } else {
      gpsData = data.substring(gpsIndex + 4);
    }
    
    Serial.println("🛰️ GPS data: " + gpsData);
    
    if (gpsData == "NO_FIX") {
      validGPS = false;
      latitude = 0.0;
      longitude = 0.0;
      satellites = 0;
      altitude = 0.0;
      speed_kmph = 0.0;
      Serial.println("❌ GPS: No fix available");
    } else {
      // Parse lat,lon,sat,alt,speed
      int comma1 = gpsData.indexOf(',');
      int comma2 = gpsData.indexOf(',', comma1 + 1);
      int comma3 = gpsData.indexOf(',', comma2 + 1);
      int comma4 = gpsData.indexOf(',', comma3 + 1);
      
      if (comma1 > 0 && comma2 > comma1 && comma3 > comma2 && comma4 > comma3) {
        latitude = gpsData.substring(0, comma1).toFloat();
        longitude = gpsData.substring(comma1 + 1, comma2).toFloat();
        satellites = gpsData.substring(comma2 + 1, comma3).toInt();
        altitude = gpsData.substring(comma3 + 1, comma4).toFloat();
        speed_kmph = gpsData.substring(comma4 + 1).toFloat();
        validGPS = true;
        
        Serial.println("✅ GPS parsed successfully:");
        Serial.println("   📍 Lat: " + String(latitude, 6));
        Serial.println("   📍 Lon: " + String(longitude, 6));
        Serial.println("   🛰️ Satellites: " + String(satellites));
        Serial.println("   📏 Altitude: " + String(altitude) + " m");
        Serial.println("   🚀 Speed: " + String(speed_kmph) + " km/h");
      }
    }
  }
}

void parseMPUData(String data) {
  // Parse PKT:
  int pktIndex = data.indexOf("PKT:");
  if (pktIndex >= 0) {
    int pipeIndex = data.indexOf('|', pktIndex);
    if (pipeIndex > pktIndex) {
      lastPacketNum = data.substring(pktIndex + 4, pipeIndex);
      counter = lastPacketNum.toInt();
      Serial.println("📦 Packet #" + lastPacketNum);
    }
  }
  
  // Parse ACC:
  int accIndex = data.indexOf("ACC:");
  if (accIndex >= 0) {
    int pipeIndex = data.indexOf('|', accIndex);
    String accData;
    if (pipeIndex > accIndex) {
      accData = data.substring(accIndex + 4, pipeIndex);
    } else {
      accData = data.substring(accIndex + 4);
    }
    parseXYZ(accData, accelX, accelY, accelZ);
    Serial.println("📐 Accelerometer - X:" + accelX + " Y:" + accelY + " Z:" + accelZ + " m/s²");
  }
  
  // Parse GYRO:
  int gyroIndex = data.indexOf("GYRO:");
  if (gyroIndex >= 0) {
    int pipeIndex = data.indexOf('|', gyroIndex);
    String gyroData;
    if (pipeIndex > gyroIndex) {
      gyroData = data.substring(gyroIndex + 5, pipeIndex);
    } else {
      gyroData = data.substring(gyroIndex + 5);
    }
    parseXYZ(gyroData, gyroX, gyroY, gyroZ);
    Serial.println("🔄 Gyroscope - X:" + gyroX + " Y:" + gyroY + " Z:" + gyroZ + " °/s");
  }
}

void parseMotionData(String data) {
  // Parse ZDIR:
  int zdirIndex = data.indexOf("ZDIR:");
  if (zdirIndex >= 0) {
    int pipeIndex = data.indexOf('|', zdirIndex);
    if (pipeIndex > zdirIndex) {
      zDirection = data.substring(zdirIndex + 5, pipeIndex);
    } else {
      zDirection = data.substring(zdirIndex + 5);
    }
    Serial.println("🧭 Z Direction: " + zDirection);
  }
  
  // Parse TMAG:
  int tmagIndex = data.indexOf("TMAG:");
  if (tmagIndex >= 0) {
    int pipeIndex = data.indexOf('|', tmagIndex);
    if (pipeIndex > tmagIndex) {
      totalMagnitude = data.substring(tmagIndex + 5, pipeIndex);
    } else {
      totalMagnitude = data.substring(tmagIndex + 5);
    }
    Serial.println("📊 Total Magnitude: " + totalMagnitude + " m/s²");
  }
}

void parseXYZ(String data, String &x, String &y, String &z) {
  // ลบช่องว่างออก
  data.trim();
  
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (firstComma > 0 && secondComma > firstComma) {
    x = data.substring(0, firstComma);
    y = data.substring(firstComma + 1, secondComma);
    z = data.substring(secondComma + 1);
    
    // ลบช่องว่างและอักขระที่ไม่ต้องการ
    x.trim();
    y.trim();
    z.trim();
  } else {
    Serial.println("❌ Error parsing XYZ data: " + data);
    x = "0";
    y = "0";
    z = "0";
  }
}
