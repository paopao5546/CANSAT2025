/*
 * LoRa Receiver with WiFi Web Interface (Corrected Units)
 * Receives MPU6050 data via LoRa and displays on web page
 * Accelerometer in m/s² with proper gravity handling
 * Compatible with ESP32
 */

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// ============ WiFi Config ================
const char* ssid = "Chanathip's S24 Ultra";  // เปลี่ยนเป็น WiFi ของคุณ
const char* password = "wcyg2779";            // เปลี่ยนเป็นรหัสผ่าน WiFi ของคุณ
WiFiServer server(80);

// ============ LoRa Config ================
#define ss 5
#define rst 14
#define dio0 2

// ============ Data Variables =============
String LoRaData = "Waiting for data...";
String lastPacketNum = "0";
String accelX = "0", accelY = "0", accelZ = "0";
String gyroX = "0", gyroY = "0", gyroZ = "0";
unsigned long lastReceived = 0;

// ============ Web Variables ==============
String header = "";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);

  // --- LoRa Init ---
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(922.525E6)) {
    Serial.println("LoRa init failed. Trying again...");
    delay(500);
  }
  
  // Set LoRa parameters (must match sender)
  LoRa.setSignalBandwidth(125E3);    // 125 KHz bandwidth
  LoRa.setSpreadingFactor(9);        // SF9
  LoRa.setCodingRate4(5);            // 4/5 coding rate
  LoRa.setSyncWord(0xF3);            // Sync word
  LoRa.enableCrc();                  // Enable CRC
  
  Serial.println("LoRa Configuration:");
  Serial.println("- Frequency: 922.525 MHz");
  Serial.println("- Bandwidth: 125 KHz");
  Serial.println("- Spreading Factor: 9");
  Serial.println("- Coding Rate: 4/5");
  Serial.println("- CRC: Enabled");
  Serial.println("LoRa Initializing OK!");

  // --- WiFi Init ---
  Serial.print("Connecting to WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Web Server IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("=== System Ready ===\n");
}

void loop() {
  // ==== LoRa Receive ====
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("--- LoRa Packet Received ---");
    Serial.print("Packet size: ");
    Serial.println(packetSize);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    Serial.print("SNR: ");
    Serial.println(LoRa.packetSnr());
    
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    
    Serial.print("Raw data: ");
    Serial.println(received);
    
    // Parse received data
    if (received.length() > 0) {
      parseLoRaData(received);
      lastReceived = millis();
      
      Serial.println("--- Parsed Data ---");
      Serial.println("Packet: " + lastPacketNum);
      Serial.println("Accel (m/s²): X=" + accelX + " Y=" + accelY + " Z=" + accelZ);
      Serial.println("Gyro (°/s): X=" + gyroX + " Y=" + gyroY + " Z=" + gyroZ);
    } else {
      Serial.println("Empty packet received!");
    }
    Serial.println("========================\n");
  }
  
  // Debug: Print status every 10 seconds
  static unsigned long debugTimer = 0;
  if (millis() - debugTimer > 10000) {
    Serial.println("--- Status Check ---");
    Serial.print("Last data received: ");
    Serial.print((millis() - lastReceived) / 1000);
    Serial.println(" seconds ago");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("Listening for LoRa packets...");
    Serial.println("==================\n");
    debugTimer = millis();
  }

  // ==== Handle Web Client ====
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // ==== API endpoint for real-time data ====
            if (header.indexOf("GET /data") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();
              
              // Send JSON data
              client.println("{");
              client.println("  \"packet\": \"" + lastPacketNum + "\",");
              client.println("  \"accelerometer\": {");
              client.println("    \"x\": \"" + accelX + "\",");
              client.println("    \"y\": \"" + accelY + "\",");
              client.println("    \"z\": \"" + accelZ + "\"");
              client.println("  },");
              client.println("  \"gyroscope\": {");
              client.println("    \"x\": \"" + gyroX + "\",");
              client.println("    \"y\": \"" + gyroY + "\",");
              client.println("    \"z\": \"" + gyroZ + "\"");
              client.println("  },");
              client.println("  \"lastReceived\": \"" + String((millis() - lastReceived) / 1000) + "s ago\"");
              client.println("}");
            } else {
              // ==== Main HTML Page ====
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              
              // HTML with enhanced styling
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>");
              client.println("<title>CANSAT LoRa Data Monitor</title>");
              client.println("<style>");
              client.println("body { font-family: 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; color: white; }");
              client.println(".container { max-width: 1200px; margin: 0 auto; }");
              client.println("h1 { text-align: center; font-size: 2.5em; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }");
              client.println(".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }");
              client.println(".card { background: rgba(255,255,255,0.15); backdrop-filter: blur(10px); border-radius: 15px; padding: 20px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); }");
              client.println(".card h3 { margin: 0 0 15px 0; font-size: 1.3em; border-bottom: 2px solid rgba(255,255,255,0.3); padding-bottom: 10px; }");
              client.println(".value { font-size: 1.6em; font-weight: bold; margin: 10px 0; }");
              client.println(".label { font-size: 0.9em; opacity: 0.8; }");
              client.println(".status { text-align: center; padding: 10px; border-radius: 10px; margin-bottom: 20px; }");
              client.println(".online { background: rgba(76, 175, 80, 0.3); }");
              client.println(".offline { background: rgba(244, 67, 54, 0.3); }");
              client.println(".note { font-size: 0.85em; opacity: 0.7; margin-top: 15px; padding: 10px; background: rgba(255,255,255,0.1); border-radius: 8px; }");
              client.println("</style>");
              
              // JavaScript for real-time updates
              client.println("<script>");
              client.println("let isOnline = false;");
              client.println("function updateData() {");
              client.println("  fetch('/data')");
              client.println("    .then(response => response.json())");
              client.println("    .then(data => {");
              client.println("      document.getElementById('packet').textContent = data.packet;");
              client.println("      document.getElementById('accX').textContent = data.accelerometer.x + ' m/s²';");
              client.println("      document.getElementById('accY').textContent = data.accelerometer.y + ' m/s²';");
              client.println("      document.getElementById('accZ').textContent = data.accelerometer.z + ' m/s²';");
              client.println("      document.getElementById('gyroX').textContent = data.gyroscope.x + '°/s';");
              client.println("      document.getElementById('gyroY').textContent = data.gyroscope.y + '°/s';");
              client.println("      document.getElementById('gyroZ').textContent = data.gyroscope.z + '°/s';");
              client.println("      document.getElementById('lastReceived').textContent = data.lastReceived;");
              client.println("      isOnline = parseInt(data.lastReceived) < 10;");
              client.println("      document.getElementById('status').className = 'status ' + (isOnline ? 'online' : 'offline');");
              client.println("      document.getElementById('statusText').textContent = isOnline ? 'ONLINE' : 'OFFLINE';");
              client.println("    })");
              client.println("    .catch(err => console.log('Error:', err));");
              client.println("}");
              client.println("setInterval(updateData, 1000);");
              client.println("updateData();");
              client.println("</script>");
              client.println("</head>");
              
              client.println("<body>");
              client.println("<div class='container'>");
              client.println("<h1>🛰️ CANSAT Data Monitor</h1>");
              client.println("<div id='status' class='status offline'>");
              client.println("<strong id='statusText'>CONNECTING...</strong> | Last Update: <span id='lastReceived'>-</span> | Packet #<span id='packet'>-</span>");
              client.println("</div>");
              
              client.println("<div class='grid'>");
              
              // Accelerometer Card
              client.println("<div class='card'>");
              client.println("<h3>📐 Accelerometer (m/s²)</h3>");
              client.println("<div class='label'>X-axis:</div><div class='value' id='accX'>-</div>");
              client.println("<div class='label'>Y-axis:</div><div class='value' id='accY'>-</div>");
              client.println("<div class='label'>Z-axis:</div><div class='value' id='accZ'>-</div>");
              client.println("<div class='note'>📝 Note: Z-axis shows +9.81 m/s² when sensor is placed flat (chip facing up)</div>");
              client.println("</div>");
              
              // Gyroscope Card
              client.println("<div class='card'>");
              client.println("<h3>🔄 Gyroscope (°/s)</h3>");
              client.println("<div class='label'>X-axis:</div><div class='value' id='gyroX'>-</div>");
              client.println("<div class='label'>Y-axis:</div><div class='value' id='gyroY'>-</div>");
              client.println("<div class='label'>Z-axis:</div><div class='value' id='gyroZ'>-</div>");
              client.println("<div class='note'>📝 Angular velocity in degrees per second</div>");
              client.println("</div>");
              
              client.println("</div>");
              client.println("</div>");
              client.println("</body></html>");
              client.println();
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
  }
}

// Function to parse LoRa data
void parseLoRaData(String data) {
  // Expected format: PKT:1|ACC:0.123,0.456,0.789|GYRO:1.23,4.56,7.89
  
  Serial.println("Parsing data: " + data);
  
  int pktIndex = data.indexOf("PKT:");
  int accIndex = data.indexOf("ACC:");
  int gyroIndex = data.indexOf("GYRO:");
  
  if (pktIndex >= 0) {
    int endIndex = data.indexOf("|", pktIndex);
    if (endIndex > 0) {
      lastPacketNum = data.substring(pktIndex + 4, endIndex);
      Serial.println("Found packet: " + lastPacketNum);
    }
  }
  
  if (accIndex >= 0) {
    int endIndex = data.indexOf("|", accIndex);
    if (endIndex > 0) {
      String accData = data.substring(accIndex + 4, endIndex);
      Serial.println("Found acc data: " + accData);
      parseXYZ(accData, accelX, accelY, accelZ);
    }
  }
  
  if (gyroIndex >= 0) {
    String gyroData = data.substring(gyroIndex + 5);
    Serial.println("Found gyro data: " + gyroData);
    parseXYZ(gyroData, gyroX, gyroY, gyroZ);
  }
  
  // Update LoRaData for legacy compatibility
  LoRaData = "PKT:" + lastPacketNum + " | ACC:" + accelX + "," + accelY + "," + accelZ + " | GYRO:" + gyroX + "," + gyroY + "," + gyroZ;
}

// Helper function to parse X,Y,Z values
void parseXYZ(String data, String &x, String &y, String &z) {
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);
  
  if (firstComma > 0 && secondComma > 0) {
    x = data.substring(0, firstComma);
    y = data.substring(firstComma + 1, secondComma);
    z = data.substring(secondComma + 1);
  }
}
