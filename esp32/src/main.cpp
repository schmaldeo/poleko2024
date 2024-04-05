#include <Arduino.h>
#include <WiFi.h>
#include <Sensor.h>
#include <vector>
#include <utility>

// WiFiServer server(80);
String header;

IPAddress localIP(192, 168, 11, 20);
IPAddress gateway(192, 168, 11, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

String receivedMessage;

HardwareSerial SensorSerial(2);
Sensor sensor(SensorSerial);

void setupWifi(String ssid, String password, unsigned int timeout);
void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin);

void setup() {
    setupSerial(SensorSerial, 16, 17);
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("smallboard", "12345678");
//   WiFi.config(localIP, gateway, subnet);
//   server.begin();
//   SensorSerial.println('\n');
}

void loop() {
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


    delay(2000);


    // String message = readSensorData(SensorSerial);
    // std::pair<float, float> data = processSensorData(message);
    std::pair<float, float> data = sensor.getSensorData();
    Serial.print("Humidity: ");
    Serial.print(data.first);
    Serial.print("\n");
    Serial.print("Temperature: ");
    Serial.print(data.second);
    Serial.print("\n\n");
}

/// @brief Sets up WiFi connection
/// @param ssid 
/// @param password 
/// @param timeout 
void setupWifi(String ssid, String password, unsigned int timeout) {
    WiFi.begin("TP-Link_DF70", "74884728");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

/// @brief Sets up UART communication
/// @param sensorSerial A reference to the HardwareSerial object related to the sensor
/// @param rxPin RxD pin the sensor is plugged into
/// @param txPin TxD pin the sensor is plugged into
void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin) {
    Serial.begin(9600);
    sensorSerial.begin(19200, SERIAL_8N1, rxPin, txPin);
}


