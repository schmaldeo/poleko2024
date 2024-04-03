#include <Arduino.h>
#include <WiFi.h>

WiFiServer server(80);
String header;

IPAddress localIP(192, 168, 11, 20);
IPAddress gateway(192, 168, 11, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// put function declarations here:
int myFunction(int, int);
void initialWifiSetup();

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("smallboard", "12345678");
  WiFi.config(localIP, gateway, subnet);
  WiFi.begin("TP-Link_DF70", "74884728");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";
    while(client.connected()) {
      currentTime = millis();
      if (client.available()) {
          char c = client.read();
          header += c;
          if (c == '\n') {
            if (currentLine.length() == 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              if (header.indexOf("GET /lol") >= 0) {
                client.println("<html><body>xdddd</body></html>");
              }
              client.println();
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

// put function definitions here:
void initialWifiSetup() {

}