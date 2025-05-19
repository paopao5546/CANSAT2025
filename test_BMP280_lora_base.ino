#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// ============ WiFi Config ================
const char* ssid = "Chanathip's S24 Ultra";
const char* password = "wcyg2779";
WiFiServer server(80);

// ============ LoRa Config ================
#define ss 5
#define rst 14
#define dio0 2

String LoRaData = "";      // เก็บค่าล่าสุดจาก LoRa
String temperature = "--"; // ค่าอุณหภูมิ
String pressure = "--";    // ค่าความดัน
String altitude = "--";    // ความสูง

String header = "";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);

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
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
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

    // --- แยกค่าจากข้อความ ---
    int tIndex = LoRaData.indexOf("T:");
    int hIndex = LoRaData.indexOf("H:");
    int aIndex = LoRaData.indexOf("A:");

    if (tIndex != -1 && hIndex != -1 && aIndex != -1) {
      temperature = LoRaData.substring(tIndex + 2, hIndex - 1);
      pressure = LoRaData.substring(hIndex + 2, aIndex - 1);
      altitude = LoRaData.substring(aIndex + 2);
    }
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
            // ==== AJAX เรียกข้อมูลดิบ ====
            if (header.indexOf("GET /data") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/plain");
              client.println("Connection: close");
              client.println();
              client.println("T:" + temperature + ",H:" + pressure + ",A:" + altitude);
            } else {
              // ==== HTML Page ====
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>");
              client.println("<title>LoRa BMP280 Receiver</title>");
              client.println("<style>body { font-family: Arial; text-align: center; margin-top: 50px; }");
              client.println("h1 { color: #333; } .data-box { font-size: 24px; background: #eee; display: inline-block; padding: 20px; border-radius: 10px; }</style>");
              
              // === JavaScript for Real-time Data ===
              client.println("<script>");
              client.println("setInterval(() => {");
              client.println("  fetch('/data').then(r => r.text()).then(t => {");
              client.println("    let parts = t.split(',');");
              client.println("    if (parts.length === 3) {");
              client.println("      document.getElementById('temp').innerText = parts[0].replace('T:', '') + ' °C';");
              client.println("      document.getElementById('pres').innerText = parts[1].replace('H:', '') + ' hPa';");
              client.println("      document.getElementById('alt').innerText = parts[2].replace('A:', '') + ' m';");
              client.println("    }");
              client.println("  });");
              client.println("}, 1000);");
              client.println("</script>");

              client.println("</head>");
              client.println("<body><h1>BMP280 Sensor Data via LoRa</h1>");
              client.println("<div class='data-box'>");
              client.println("Temperature: <strong id='temp'>--</strong><br>");
              client.println("Pressure: <strong id='pres'>--</strong><br>");
              client.println("Altitude: <strong id='alt'>--</strong>");
              client.println("</div></body></html>");
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
    Serial.println("Client disconnected.\n");
  }
}
