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

String receivedMessage;

HardwareSerial SensorSerial(2);

void initialWifiSetup();

void setup() {
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("smallboard", "12345678");
  WiFi.config(localIP, gateway, subnet);
  WiFi.begin("TP-Link_DF70", "74884728");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.begin();
  Serial.begin(9600);
  Serial.println("looool");
  SensorSerial.begin(19200, SERIAL_8N1, 16, 17);
  SensorSerial.println('\n');
  Serial.println(SensorSerial.baudRate());
    // Serial1.begin(19200, SERIAL_8N1);
    // Serial1.println('\n');
}

void loop() {
  // put your main code here, to run repeatedly:
//   WiFiClient client = server.available();

//   if (client) {
//     currentTime = millis();
//     previousTime = currentTime;
//     String currentLine = "";
//     while(client.connected()) {
//       currentTime = millis();
//       if (client.available()) {
//           char c = client.read();
//           header += c;
//           if (c == '\n') {
//             if (currentLine.length() == 0) {
//               client.println("HTTP/1.1 200 OK");
//               client.println("Content-type:text/html");
//               client.println("Connection: close");
//               client.println();

//               if (header.indexOf("GET /lol") >= 0) {
//                 client.println("<html><body>xdddd</body></html>");
//               }
//               client.println();
//               break;
//             } else {
//               currentLine = "";
//             }
//           } else if (c != '\r') {
//             currentLine += c;
//           }
//       }
//     }
//     header = "";
//     client.stop();
//   }

    SensorSerial.println("{F99RDD}\r\n");
    delay(2000); // Adjust delay based on probe response time

    unsigned long startTime = millis();
    char receivedChar;
    String receivedMessage = "";

    // while (SensorSerial.available() > 0 && (millis() - startTime) < 1000) { // Limit wait time
    //     // receivedChar = (char)SensorSerial.read(); // Cast to char
    //     // if (receivedChar == '\n') {
    //     // Serial.println(receivedMessage);
    //     // receivedMessage = ""; // Clear for next message
    //     // } else {
    //     // receivedMessage += receivedChar;
    //     // }
    //     int incomingByte = SensorSerial.read();
    //     Serial.print("0x"); // Print in hexadecimal format
    //     Serial.println(incomingByte, HEX);
    // }

    while (SensorSerial.available() > 0) {
        char receivedChar = SensorSerial.read();

        if (receivedChar == 0xD) {
            Serial.println(receivedMessage);  // Print the received message in the Serial monitor
            receivedMessage = "";  // Reset the received message
        } else {
            receivedMessage += receivedChar;  // Append characters to the received message
        }
    }
}
