#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "Sensor.h"
// #include "EspWifi.h"
#include "TCP.h"
#include <vector>
#include <utility>

HardwareSerial SensorSerial(2);
Sensor sensor(SensorSerial);
TCP tcpServer(5505);
// EspWifi Wifi;

void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin);

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin("TP-Link_DF70", "74884728");
    WiFi.config(IPAddress(192, 168, 11, 20), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    setupSerial(SensorSerial, 16, 17);
    // TCP tcpServer(5505);
    tcpServer.setup(sensor);
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


    // std::pair<float, float> data = sensor.getSensorData();
    // Serial.print("Humidity: ");
    // Serial.print(data.first);
    // Serial.print("\n");
    // Serial.print("Temperature: ");
    // Serial.print(data.second);
    // Serial.print("\n\n");
    tcpServer.loop();
}


/// @brief Sets up UART communication
/// @param sensorSerial A reference to the HardwareSerial object related to the sensor
/// @param rxPin RxD pin the sensor is plugged into
/// @param txPin TxD pin the sensor is plugged into
void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin) {
    // TODO move to Sensor, Serial0 initialisation should be optional
    Serial.begin(9600);
    sensorSerial.begin(19200, SERIAL_8N1, rxPin, txPin);
    sensorSerial.print("{F99RDD\r\n}");
}
