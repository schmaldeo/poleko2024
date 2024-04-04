#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <utility>

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
String readSensorData(HardwareSerial& serial);
std::pair<float, float> processSensorData(String& sensorData);

void setup() {
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("smallboard", "12345678");
  WiFi.config(localIP, gateway, subnet);
  WiFi.begin("TP-Link_DF70", "74884728");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // TODO fix weird characters upon launch
  server.begin();
  Serial.begin(9600);
  SensorSerial.begin(19200, SERIAL_8N1, 16, 17);
  SensorSerial.println('\n');
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


    String message = readSensorData(SensorSerial);
    std::pair<float, float> data = processSensorData(message);
    Serial.print("Humidity: ");
    Serial.print(data.first);
    Serial.print("\n");
    Serial.print("Temperature: ");
    Serial.print(data.second);
    Serial.print("\n\n");
}

String readSensorData(HardwareSerial& serial) {
    serial.println("{F99RDD}\r\n");

    char receivedChar;
    String receivedMessage = "";

    while (serial.available() > 0) {
        char receivedChar = serial.read();

        if (receivedChar == 0xD) {
            return receivedMessage;
        } else {
            receivedMessage += receivedChar;
        }
    }

    return receivedMessage;
}

std::pair<float, float> processSensorData(String& sensorData) {
    // length of the string is constant, doing a check to avoid processing empty strings that sometimes get passed
    if (sensorData.length() != 94) {
        return std::make_pair(0.0f, 0.0f);
    }
    char* data = new char[sensorData.length() + 1];
    strcpy(data, sensorData.c_str());

    char* split = strtok(data, ";");

    split = strtok(NULL, ";");

    float humidity = atof(split);

    for (int i = 0; i < 4; i++) {
        split = strtok(NULL, ";");
    }

    float temperature = atof(split);

    delete[] data;

    return std::make_pair(humidity, temperature);
}