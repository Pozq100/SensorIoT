#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <String.h>
#include <Ethernet.h>

// WiFi Credentials
char ssid[] = "eee-iot";
char password[] = "I0t@mar2025!";

byte mac[] = { 0xA8, 0x61, 0x0A, 0x50, 0x64, 0x63 };  
byte ip[] = { 192, 168, 23, 200 };
byte server[] = { 192, 168, 23, 150 };

// LoRaWAN Endpoint URL and Port
char lorawan[] = "192.168.23.150"; 
String packetAPI = "api/urpackets";

String loginAPI = "api/internal/login";

int HTTP_PORT = 80;

// Authorization Token (Bearer Token)
String authName = "admin";
String authPass = "NicJjG18XOV3U1efQyo8AQ==";
String token = "";

// AWS Endpoint URL
char endpoint[] = "54.252.31.39";  // REST API
String endpointEntity = "/web/a";
int PORT = 8080;
char apiKey[] = "95130";  // Your API key

// WiFi and Client Objects
WiFiClient client;
EthernetClient espClient;

// Function Prototypes
void connectToWiFi();
void connectToEthernet();
void sendDataToServer(const String& jsonData);
void sendDataToSerialMonitor(const String& jsonData);//testing purpose
void postData();
String getToken();
bool stringInArray(String target, String* arr, int arraySize);
int getStringindex(String response,String p, int startIndex);
String getString(String response,int index, char lookOut);

void setup() {
  // Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();
  connectToEthernet();

  
}


void loop() {
  // Read sensor data and post it to the server
  postData();
  delay(180000);  // Wait before the next reading
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("- Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay(500);
  }

  // Display Wi-Fi network information
  Serial.println();
  Serial.println("- NETWORK INFORMATION");
  Serial.print("- You're now connected to the network ");
}

void connectToEthernet(){
  if(Ethernet.begin(mac, ip)) {
    delay(1000);
    token = getToken();
    Serial.print("Got Token");
  }else {
    Serial.println("Ethernet Connection Failed");
  }
}

void postData() {
  // Create JSON document to store sensor data
  DynamicJsonDocument allJson(1024);

  int arraySize = 0;
  String* arr = new String[arraySize];
  espClient.stop();
  
  if (espClient.connect(server, HTTP_PORT)) {
    // Prepare and send the HTTP GET request
    espClient.println("GET /" + packetAPI + " HTTP/1.1");  // GET Request
    espClient.println("Host: " + String(lorawan));
    espClient.println("Authorization: Bearer " + token);  // Add the Authorization header
    espClient.println("Connection: close");
    espClient.println();  // End of headers



    // Wait for server response
    while (espClient.connected()) {
      String line = espClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Token Checking ( Since tokens change every 24H )
    String response = espClient.readString();
    if (response.indexOf("error") != -1){
      token = getToken();
    }

    // If devEUI is similar, ignore the temp and humi readings as they are the earlier results
    int index = getStringIndex(response, "devEUI", 0);
    while ( index != -1 ){
      String eui = getString(response, index + 9, '"');
      
      index = getStringIndex(response, "payloadJson", index);
      
      String json = getString(response,index + 14, '}') + '}';
      Serial.println(json);

      DynamicJsonDocument jsonDoc(200);
      deserializeJson(jsonDoc, json);

      float temperature = jsonDoc["temperature"]; // Get temperature value

      Serial.println(temperature);


      if (jsonDoc.containsKey("battery")){
        allJson[eui + "_battery"] = jsonDoc["battery"];
        
        // Print the updated allJson content
        Serial.println("Updated allJson:");
        serializeJson(allJson, Serial);  // Serialize and print directly to Serial Monitor
        Serial.println(); 
      }

      if (jsonDoc.containsKey("humidity") && jsonDoc.containsKey("temperature")){
        allJson[eui + "_humidity"] = jsonDoc["humidity"];
        allJson[eui + "_temperature"] = jsonDoc["temperature"];

        // Optionally print again if you want to monitor after these additions too
        Serial.println("Updated allJson after humidity/temperature:");
        serializeJson(allJson, Serial);
        Serial.println();  // Print newline for readability
      }
      

      
      String* tempArray = new String[arraySize + 1];
      
      // Copy the current elements to the new array
      for (int i = 0; i < arraySize; i++) {
        tempArray[i] = arr[i];
      }
    
      // Add the new string to the end of the array
      tempArray[arraySize] = eui;
    
      // Point to the new array and update size
      delete[] arr;
      arr = tempArray;
      arraySize ++;

      jsonDoc.clear();
      
      index = getStringIndex(response, "devEUI", index);
    } 
    
    espClient.stop();  // Close the connection to free up resources
    
    // Now proceed with the DELETE request
    if (espClient.connect(server, HTTP_PORT)) {
      espClient.println("DELETE /" + packetAPI + " HTTP/1.1");
      espClient.println("Host: " + String(lorawan));
      espClient.println("Authorization: Bearer " + token);
      espClient.println("Content-Type: application/json");
      espClient.println("Connection: close");
      espClient.println();

      delay(5000);
    } else {
      Serial.println("- Failed to connect for DELETE request");
    }
    
  } else {
    Serial.println("- Connection to LoRaWAN device failed");
  }
  delete[] arr;


  // Convert JSON to string
  String jsonData;
  serializeJson(allJson, jsonData);

  if (allJson.isNull()) {
  Serial.println("The allJson document is null or uninitialized.");
} else {
  Serial.println("The allJson document has data.");
}

    // Send data to the serial monitor
          
  //sendDataToServer(jsonData);
  sendDataToSerialMonitor(jsonData);

  //Send data to server
  
}

void sendDataToServer(const String& jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
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

      // Read server response
      /**
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") break;
      }
      **/

      // Print server response
      // String response = client.readString();
      // Serial.println("Server response:");
      // Serial.println(response);
    }
    client.stop();  // Close the connection
  } else {
    connectToWiFi();
  }
}

//Function to test json file
void sendDataToSerialMonitor(const String& jsonData) {
  Serial.print("Output: ");
  Serial.println(jsonData);
}

bool stringInArray(String target, String* arr, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    if (arr[i].equals(target)) {
      return true;  // String found in the array
    }
  }
  return false;  // String not found
}

int getStringIndex(String response,String p, int startIndex){
  return response.indexOf(p, startIndex);
}

String getString(String response,int index, char lookOut){
  String result = "";
  while (response[index] != lookOut){
    result += String(response[index]);
    index ++;
  }
  
  return result;
}

String getToken(){
  espClient.stop();
  
  if (espClient.connect(lorawan, HTTP_PORT)) {
    Serial.println("Getting Token...");
    JsonDocument doc;

    doc["username"] = authName;
    doc["password"] = authPass;
    String jsonData;
        
    serializeJson(doc, jsonData);
    
    espClient.println("POST /" + loginAPI + " HTTP/1.1");
    espClient.println("Host: " + String(lorawan));
    espClient.println("Content-Type: application/json");  // Content type
    espClient.println("Content-Length: " + String(jsonData.length())); 
    espClient.println("Connection: close");
    espClient.println();  // End of headers

    // Send the JSON payload
    espClient.println(jsonData);

    while (espClient.connected()) {
      String line = espClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Read and print the server response
    String response = espClient.readString();

    // Parse JSON response
    JsonDocument doc2;
    DeserializationError error = deserializeJson(doc2, response);
    if (error) {
      Serial.print("DeserializeJson() failed: ");
      Serial.println(error.c_str());
      return "";
    }
    // Extract JWT from the JSON response
    String jwt = doc2["jwt"].as<String>();
    return jwt;
  }
}
