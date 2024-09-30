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
char password[] = "I0t@eee2024!";
int status = WL_IDLE_STATUS;

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
  // Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize Modbus RTU Client
  // Serial.println("Modbus RTU Client");
  RS485.setDelays(preDelayBR, postDelayBR);
  if (!ModbusRTUClient.begin(baudrate)) {
    // Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }

  for (int i = 0; i < 4; i++) {
    pinMode(USER_LEDS[i], OUTPUT);
  }
  
}


void loop() {
  // Read sensor data and post it to the server
  postData();
  delay(274000);  // Wait before the next reading

  
}

void connectToWiFi() {
  while (status != WL_CONNECTED) {
    //Serial.print("- Attempting to connect to WPA SSID: ");
    //Serial.println(ssid);
    status = WiFi.begin(ssid, password);
    digitalWrite(USER_LEDS[0], LOW);
    delay(500);
  }

  // Display Wi-Fi network information
  // Serial.println();
  // Serial.println("- NETWORK INFORMATION");
  // Serial.print("- You're now connected to the network ");
  digitalWrite(USER_LEDS[0], HIGH); 
}

void postData() {
  // Create JSON document to store sensor data
  DynamicJsonDocument jsonDoc(1024);

  if (jsonDoc.capcity() == 0){
    NVIC_SystemReset();
  }

  
  jsonDoc["Sensor_1_Energy"] = readDDSUAddress(1, 0x101E);
  jsonDoc["Sensor_1_Current"] = round(sqrt((pow(readDDSUAddress(1, 0x200C)*0.001, 2 ) + pow(readDDSUAddress(1, 0x200E)*0.001, 2 ) + pow(readDDSUAddress(1, 0x2010)*0.001, 2)) / 3) * 100.0) / 100.0;
  jsonDoc["Sensor_1_Voltage"] = round(sqrt((pow(readDDSUAddress(1, 0x2006)*0.1, 2 ) + pow(readDDSUAddress(1, 0x2008)*0.1, 2 ) + pow(readDDSUAddress(1, 0x200A)*0.1, 2)) / 3) * 100.0) / 100.0;

  jsonDoc["Sensor_2_Energy"] = readDDSUAddress(2, 0x101E);
  jsonDoc["Sensor_2_Current"] = round(sqrt((pow(readDDSUAddress(2, 0x200C)*0.001, 2 ) + pow(readDDSUAddress(2, 0x200E)*0.001, 2 ) + pow(readDDSUAddress(2, 0x2010)*0.001, 2)) / 3) * 100.0) / 100.0;
  jsonDoc["Sensor_2_Voltage"] = round(sqrt((pow(readDDSUAddress(2, 0x2006)*0.1, 2 ) + pow(readDDSUAddress(2, 0x2008)*0.1, 2 ) + pow(readDDSUAddress(2, 0x200A)*0.1, 2)) / 3) * 100.0) / 100.0;

 
  
  jsonDoc["Sensor_3_Energy"] = readDDSUAddress(3, 0x4000);
  jsonDoc["Sensor_3_Current"] = readDDSUAddress(3, 0x2002);
  jsonDoc["Sensor_3_Voltage"] = readDDSUAddress(3, 0x2000);

  
  jsonDoc["Sensor_4_Energy"] = readDDSUAddress(4, 0x4000);
  jsonDoc["Sensor_4_Current"] = readDDSUAddress(4, 0x2002);
  jsonDoc["Sensor_4_Voltage"] = readDDSUAddress(4, 0x2000);
    
  jsonDoc["Sensor_5_Energy"] = readDDSUAddress(5, 0x4000);
  jsonDoc["Sensor_5_Current"] = readDDSUAddress(5, 0x2002);
  jsonDoc["Sensor_5_Voltage"] = readDDSUAddress(5, 0x2000);
    
  jsonDoc["Sensor_6_Energy"] = readDDSUAddress(6, 0x4000);
  jsonDoc["Sensor_6_Current"] = readDDSUAddress(6, 0x2002);
  jsonDoc["Sensor_6_Voltage"] = readDDSUAddress(6, 0x2000);
  
  
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
        
      // Read server response
      String responseLine = client.readStringUntil('\n');  // Read the first line (status line)
      // Example of a status line: HTTP/1.1 200 OK
      
      // Check if the server response contains "200 OK"
      if (responseLine.indexOf("200 OK") > 0 || responseLine.indexOf("201 Created") > 0) {
        // Successfully received OK response from the server
        digitalWrite(USER_LEDS[3], HIGH);  // Indicate success
      } else {
        // Handle non-OK responses
        digitalWrite(USER_LEDS[3], LOW);  // Indicate failure
        // Optionally, print the response for debugging
        // Serial.println("Server responded with an error:");
        // Serial.println(responseLine);
      }
      

      // Print server response
      // String response = client.readString();
      // Serial.println("Server response:");
      // Serial.println(response);
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
  /**
  Serial.print("Reading values from device ");
  Serial.println("Slave address of sensor: " + String(ID));
  **/
  delay(1000);
  if (!ModbusRTUClient.requestFrom(ID, INPUT_REGISTERS, addr, 2)) {

    NVIC_SystemReset();
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
  return result;
}
