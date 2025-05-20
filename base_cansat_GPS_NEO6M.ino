/*********
  Base Station with LoRa and WiFi web server
  Receives GPS coordinates and displays them in real-time
*********/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// ============ WiFi Config ================
const char* ssid = "Chanathip's S24 Ultra";  // ชื่อ WiFi
const char* password = "wcyg2779";           // รหัสผ่าน WiFi
WiFiServer server(80);

// ============ LoRa Config ================
#define ss 5
#define rst 14
#define dio0 2

// ============ Global Variables ============
String LoRaData = "ยังไม่ได้รับข้อมูล";    // เก็บค่าล่าสุดที่รับจาก LoRa
String header = "";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// ตัวแปรสำหรับเก็บค่า GPS
float latitude = 0.0;
float longitude = 0.0;
int satellites = 0;
float altitude = 0.0;
float speed_kmph = 0.0;
int counter = 0;
bool validGPS = false;

// ตัวแปรสำหรับการเก็บข้อมูลย้อนหลัง
String lastFiveData[5] = {"", "", "", "", ""};
int dataIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Base Station LoRa GPS Receiver");
  Serial.println("------------------------------");

  // --- LoRa Init ---
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed. Trying again...");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("------------------------------");
  Serial.println("เปิดเบราว์เซอร์แล้วไปที่ IP นี้เพื่อดูข้อมูล GPS");
  Serial.println("------------------------------");
  server.begin();
}

void loop() {
  // ==== LoRa Receive ====
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    LoRaData = received;
    Serial.println("Received: " + LoRaData);
    
    // เก็บข้อมูลในอาร์เรย์
    lastFiveData[dataIndex] = LoRaData;
    dataIndex = (dataIndex + 1) % 5;  // หมุนเวียนในอาร์เรย์ขนาด 5
    
    // แยกข้อมูล GPS จากข้อความที่ได้รับ
    parseGPSData(LoRaData);
  }

  // ==== Handle Web Client ====
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client Connected.");
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // HTML page
            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<head>");
            client.println("<meta charset='UTF-8'>");
            client.println("<meta http-equiv='refresh' content='5'>");  // รีเฟรชทุก 5 วินาที
            client.println("<title>GPS LoRa Receiver</title>");
            client.println("<style>");
            client.println("body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }");
            client.println(".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }");
            client.println("h1 { color: #2c3e50; text-align: center; }");
            client.println("h2 { color: #3498db; }");
            client.println(".status-box { background-color: #ecf0f1; padding: 15px; border-radius: 5px; margin-bottom: 20px; }");
            client.println(".status { font-weight: bold; }");
            client.println(".status.valid { color: green; }");
            client.println(".status.invalid { color: red; }");
            client.println(".data-item { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }");
            client.println(".history { margin-top: 20px; background-color: #f8f9fa; padding: 10px; border-radius: 5px; }");
            client.println(".history-item { padding: 5px; border-bottom: 1px solid #ddd; }");
            client.println(".location-link { margin-top: 20px; text-align: center; }");
            client.println(".location-link a { background-color: #3498db; color: white; padding: 10px 15px; text-decoration: none; border-radius: 5px; }");
            client.println("</style>");
            client.println("</head>");
            
            client.println("<body>");
            client.println("<div class='container'>");
            client.println("<h1>CanSat GPS Tracker</h1>");
            
            // สถานะ GPS
            client.println("<div class='status-box'>");
            client.println("<h2>GPS Status</h2>");
            client.print("<p>Status: <span class='status ");
            client.print(validGPS ? "valid'>VALID" : "invalid'>NO FIX");
            client.println("</span></p>");
            client.println("</div>");
            
            // ข้อมูล GPS
            client.println("<h2>GPS Data</h2>");
            client.println("<div class='data-item'><strong>Latitude:</strong> <span>" + String(latitude, 6) + "</span></div>");
            client.println("<div class='data-item'><strong>Longitude:</strong> <span>" + String(longitude, 6) + "</span></div>");
            client.println("<div class='data-item'><strong>Satellites:</strong> <span>" + String(satellites) + "</span></div>");
            client.println("<div class='data-item'><strong>Altitude:</strong> <span>" + String(altitude, 1) + " m</span></div>");
            client.println("<div class='data-item'><strong>Speed:</strong> <span>" + String(speed_kmph, 1) + " km/h</span></div>");
            client.println("<div class='data-item'><strong>Counter:</strong> <span>" + String(counter) + "</span></div>");
            client.println("<div class='data-item'><strong>Raw Data:</strong> <span>" + LoRaData + "</span></div>");
            
            // ลิงก์ไปยัง Google Maps
            if (validGPS) {
              client.println("<div class='location-link'>");
              client.println("<a href='https://www.google.com/maps?q=" + String(latitude, 6) + "," + String(longitude, 6) + "' target='_blank'>View on Google Maps</a>");
              client.println("</div>");
            }
            
            // ประวัติข้อมูล
            client.println("<div class='history'>");
            client.println("<h2>Last Data Received</h2>");
            for (int i = 0; i < 5; i++) {
              if (lastFiveData[i] != "") {
                client.println("<div class='history-item'>" + lastFiveData[i] + "</div>");
              }
            }
            client.println("</div>");
            
            client.println("<p style='text-align: center; margin-top: 20px; color: #7f8c8d;'>This page will refresh every 5 seconds</p>");
            client.println("</div>");
            client.println("</body>");
            client.println("</html>");
            
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
    Serial.println("Client disconnected.\n");
  }
}

// ฟังก์ชันแยกข้อมูลจากสตริงที่ได้รับจาก LoRa
void parseGPSData(String data) {
  if (data.startsWith("NO_GPS")) {
    // ถ้าข้อมูลเริ่มต้นด้วย NO_GPS แสดงว่ายังไม่มีการล็อค GPS
    validGPS = false;
    
    // อ่านค่า counter ที่อยู่หลัง comma
    int commaIndex = data.indexOf(',');
    if (commaIndex > 0) {
      counter = data.substring(commaIndex + 1).toInt();
    }
    
    Serial.println("GPS: NO FIX");
  } else {
    // พยายามแยกค่าจากสตริงที่ได้รับ
    int indices[5]; // เก็บตำแหน่งของเครื่องหมาย ,
    int idx = 0;
    int pos = 0;
    
    // หาตำแหน่งของเครื่องหมาย , ทั้งหมด
    while (idx < 5 && pos >= 0) {
      pos = data.indexOf(',', pos + 1);
      if (pos >= 0) {
        indices[idx++] = pos;
      }
    }
    
    // ถ้าพบเครื่องหมาย , ครบทั้ง 5 ตัว จึงจะถือว่าเป็นข้อมูล GPS ที่ถูกต้อง
    if (idx == 5) {
      validGPS = true;
      
      // แยกค่าจากสตริง
      latitude = data.substring(0, indices[0]).toFloat();
      longitude = data.substring(indices[0] + 1, indices[1]).toFloat();
      satellites = data.substring(indices[1] + 1, indices[2]).toInt();
      altitude = data.substring(indices[2] + 1, indices[3]).toFloat();
      speed_kmph = data.substring(indices[3] + 1, indices[4]).toFloat();
      counter = data.substring(indices[4] + 1).toInt();
      
      Serial.println("GPS: VALID FIX");
      Serial.println("Lat: " + String(latitude, 6) + ", Lng: " + String(longitude, 6));
      Serial.println("Sats: " + String(satellites) + ", Alt: " + String(altitude) + "m, Speed: " + String(speed_kmph) + "km/h");
    } else {
      // ไม่สามารถแยกข้อมูลได้ถูกต้อง
      validGPS = false;
      Serial.println("Invalid GPS data format");
    }
  }
}
