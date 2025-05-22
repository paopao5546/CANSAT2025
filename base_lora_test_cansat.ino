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

String LoRaData = "";    // เก็บค่าล่าสุดที่รับจาก LoRa
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
            // ==== ส่งข้อมูลแบบเรียลไทม์ด้วย AJAX ====
            if (header.indexOf("GET /data") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/plain");
              client.println("Connection: close");
              client.println();
              client.println(LoRaData);
            } else {
              // ==== HTML หลัก ====
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>");
              client.println("<title>LoRa Receiver Web</title>");
              client.println("<style>body { font-family: Arial; text-align: center; margin-top: 50px; }");
              client.println("h1 { color: #333; } .data-box { font-size: 24px; background: #eee; display: inline-block; padding: 20px; border-radius: 10px; }</style>");
              // === JavaScript AJAX ===
              client.println("<script>");
              client.println("setInterval(() => {");
              client.println("  fetch('/data').then(r => r.text()).then(t => { document.getElementById('data').innerHTML = t; });");
              client.println("}, 1000);");
              client.println("</script>");
              client.println("</head>");
              client.println("<body><h1>LoRa Data Receiver</h1>");
              client.println("<div class='data-box'>Last Received:<br><strong id='data'>Loading...</strong></div>");
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
    Serial.println("Client disconnected.\n");
  }
}
