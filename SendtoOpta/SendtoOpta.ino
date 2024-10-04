#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <String.h>

// Constants
constexpr auto baudrate {9600};
constexpr auto bitduration {1.f / baudrate};
constexpr auto preDelayBR {bitduration * 9.6f * 3.5f * 1e6};
constexpr auto postDelayBR {bitduration * 9.6f * 3.5f * 1e6};

const int USER_LEDS[] = {LED_D0, LED_D1, LED_D2, LED_D3};
/**
 * LED1 = Wifi connection, Connected - HIGH
 * LED2 = MODBUS connection, No error - HIGH
 * LED3 - Client can connect - HIGH
 * LEd4 = Sending data, sending - HIGH
 */


// WiFi Credentials
char ssid[] = "eee-iot";
char password[] = "I0t@mar2025!";

// AWS Endpoint URL
char endpoint[] = "54.252.31.39";  // REST API
String endpointEntity = "/web/a";
int PORT = 8080;
char apiKey[] = "95130";  // Your API key


// WiFi and Client Objects
WiFiClient client;

// Function Prototypes
void connectToWiFi();
void sendDataToServer(const String& jsonData);
void sendDataToSerialMonitor(const String& jsonData);//testing purpose
float readDDSUAddress(int ID, int addr);
double precision2double(unsigned long f);
void postData();

void setup() {
  Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize Modbus RTU Client
  Serial.println("Modbus RTU Client");
  RS485.setDelays(preDelayBR, postDelayBR);
  if (!ModbusRTUClient.begin(baudrate)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }

  for (int i = 0; i < 4; i++) {
    pinMode(USER_LEDS[i], OUTPUT);
  }
  
}


void loop() {
    Serial.println("Start Loop");
    digitalWrite(USER_LEDS[3], HIGH);
    postData();
    digitalWrite(USER_LEDS[3], LOW);
    Serial.println("End Loop");
    delay(271000);


}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("- Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    digitalWrite(USER_LEDS[0], LOW);
    delay(500);
  }

  // Display Wi-Fi network information
  Serial.println();
  Serial.println("- NETWORK INFORMATION");
  Serial.print("- You're now connected to the network ");
  digitalWrite(USER_LEDS[0], HIGH); 
}

void postData() {
  // Create JSON document to store sensor data
  DynamicJsonDocument jsonDoc(1024);


  // Sensor 1 current values
  float sensor1_current1 = readDDSUAddress(1, 0x200C) * 0.001;
  float sensor1_current2 = readDDSUAddress(1, 0x200E) * 0.001;
  float sensor1_current3 = readDDSUAddress(1, 0x2010) * 0.001;

  // Sensor 2 current values
  float sensor2_current1 = readDDSUAddress(2, 0x200C) * 0.001;
  float sensor2_current2 = readDDSUAddress(2, 0x200E) * 0.001;
  float sensor2_current3 = readDDSUAddress(2, 0x2010) * 0.001;

  // Sort Sensor 1 current values
  float currents1[] = {sensor1_current1, sensor1_current2, sensor1_current3};
  for (int i = 0; i < 2; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (currents1[i] > currents1[j]) {
        float temp = currents1[i];
        currents1[i] = currents1[j];
        currents1[j] = temp;
      }
    }
  }
  float sensor1_median_current = currents1[1];  // Median for Sensor 1

  // Sort Sensor 2 current values
  float currents2[] = {sensor2_current1, sensor2_current2, sensor2_current3};
  for (int i = 0; i < 2; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (currents2[i] > currents2[j]) {
        float temp = currents2[i];
        currents2[i] = currents2[j];
        currents2[j] = temp;
      }
    }
  }
  float sensor2_median_current = currents2[1];  // Median for Sensor 2

  //Sensor_1 data
  jsonDoc["Sensor_1_Energy"] = readDDSUAddress(1, 0x101E);
  jsonDoc["Sensor_1_Power"] = readDDSUAddress(1,0x2012) * 0.0001;
  jsonDoc["Sensor_1_Current"] = sensor1_median_current;


  //Sensor_2 data
  jsonDoc["Sensor_2_Energy"] = readDDSUAddress(2, 0x101E);
  jsonDoc["Sensor_2_Power"] = readDDSUAddress(2,0x2012) * 0.0001;
  jsonDoc["Sensor_2_Current"] = sensor2_median_current;


  //Sensor_3 data
  jsonDoc["Sensor_3_Energy"] = readDDSUAddress(3, 0x4000);
  jsonDoc["Sensor_3_Power"] = readDDSUAddress(3,0x2004);
  jsonDoc["Sensor_3_Current"] = readDDSUAddress(3, 0x2002);


  //Sensor_4 data
  jsonDoc["Sensor_4_Energy"] = readDDSUAddress(4, 0x4000);
  jsonDoc["Sensor_4_Power"] = readDDSUAddress(4,0x2004);
  jsonDoc["Sensor_4_Current"] = readDDSUAddress(4, 0x2002);

  //Sensor_5 data 
  jsonDoc["Sensor_5_Energy"] = readDDSUAddress(5, 0x4000);
  jsonDoc["Sensor_5_Power"] = readDDSUAddress(5,0x2004);
  jsonDoc["Sensor_5_Current"] = readDDSUAddress(5, 0x2002);

  //Sensor_6 data
  jsonDoc["Sensor_6_Energy"] = readDDSUAddress(6, 0x4000);
  jsonDoc["Sensor_6_Power"] = readDDSUAddress(6,0x2004);
  jsonDoc["Sensor_6_Current"] = readDDSUAddress(6, 0x2002);

  
  
  // Convert JSON to string
  char jsonData[1024];
  serializeJson(jsonDoc, jsonData);

  // Send data to the serial monitor
  // sendDataToSerialMonitor(jsonData);

  //Send data to server
  sendDataToServer(jsonData);
}

void sendDataToServer(const String& jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    
    client.setTimeout(5000);
    Serial.println(jsonData);
    if (client.connect(endpoint, PORT)) {
      
      // Prepare HTTP POST request
      client.println("POST " + endpointEntity + " HTTP/1.1");
      client.println("Host: 54.252.31.39:8080");
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(jsonData.length());
      client.print("x-api-key: ");
      client.println(apiKey);
      client.println();  // End of headers
      client.println(jsonData);  // Send JSON data
      
      delay(1000);
      

      // Print server response
      String response = client.readString();
      Serial.println("Server response:");
      Serial.println(response);
      digitalWrite(USER_LEDS[2], HIGH); 
    } else {
      digitalWrite(USER_LEDS[2], LOW);
    }
    client.stop();
  } else {
    connectToWiFi();
  }
}

//Function to test json file
void sendDataToSerialMonitor(const String& jsonData) {
  Serial.println(jsonData);
}

float readDDSUAddress(int ID, int addr) {

  Serial.print("Reading values from device ");
  Serial.println("Slave address of sensor: " + String(ID));

  delay(1000);
  if (!ModbusRTUClient.requestFrom(ID, INPUT_REGISTERS, addr, 2)) {
    
    digitalWrite(USER_LEDS[1], LOW); 
    
    return 0.0;
  } else {
    digitalWrite(USER_LEDS[1], HIGH); 
    unsigned long firstByte = ModbusRTUClient.read();
    unsigned long secondByte = ModbusRTUClient.read();
    unsigned long finalValue = (firstByte << 16) | secondByte;
    return precision2double(finalValue);
  }
}

double precision2double(unsigned long f) {
  struct ieee754 {
    unsigned long m : 23;  // Mantissa
    unsigned long e : 8;   // Exponent
    unsigned long s : 1;   // Sign
  } v;

  memcpy(&v, &f, sizeof(f));  // Copy raw value into IEEE754 structure ,copy f ( Byte ) into v ( IEEE754 )
  double result = (1 + v.m * pow(2, -23)) * pow(2, v.e - 127);
  return result;
}
