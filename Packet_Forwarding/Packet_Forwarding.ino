#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <Ethernet.h>

// WIFI Connection
char ssid[] = "eee-iot";
char password[] = "I0t@eee2024!";
int status  = WL_IDLE_STATUS;

byte mac[] = { 0xA8, 0x61, 0x0A, 0x50, 0x64, 0x63 };  
byte ip[] = { 192, 168, 23, 200 };
byte server[] = { 192, 168, 23, 150 };

// LoRaWAN Endpoint URL and Port
char endpoint[] = "192.168.23.150"; 
String packetAPI = "api/urpackets";

String loginAPI = "api/internal/login";

int HTTP_PORT = 80;

// Authorization Token (Bearer Token)
String authName = "admin";
String authPass = "NicJjG18XOV3U1efQyo8AQ==";

String token = "";

// Variables ---------------------------------------------------------------
EthernetClient client;

// Functions ---------------------------------------------------------------
String getToken();
bool stringInArray(String target, String* arr, int arraySize);
int getStringindex(String response,String p, int startIndex);
String getString(String response,int index, char lookOut);

void setup() {
  Serial.begin(9600);
  if(Ethernet.begin(mac, ip)) {
    delay(1000);
    token = getToken();
  }else {
    Serial.println("Ethernet Connection Failed");
  }
} 

void loop() {
  // Leave empty if you just want to execute once
  int arraySize = 0;
  String* arr = new String[arraySize];
  if (client.connect(server, HTTP_PORT)) {
    // Prepare and send the HTTP GET request
    client.println("GET /" + packetAPI + " HTTP/1.1");  // GET Request
    client.println("Host: " + String(endpoint));
    client.println("Authorization: Bearer " + token);  // Add the Authorization header
    client.println("Connection: close");
    client.println();  // End of headers

    // Wait for server response
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Token Checking ( Since tokens change every 24H )
    String response = client.readString();
    if (response.indexOf("error") != -1){
      token = getToken();
    }

    // If devEUI is similar, ignore the temp and humi readings as they are the earlier results
    int index = getStringIndex(response, "devEUI", 0);
    while ( index != -1 ){
      String eui = getString(response, index + 9, '"');
      if (!(stringInArray(eui, arr, arraySize)) ) {
        Serial.println("devEUI: " + eui);
        index = getStringIndex(response, "humidity", index);
        if (index == -1) break;
        
        Serial.println("Humidity: " + getString(response, index + 11, ','));
        index = getStringIndex(response, "temperature", index);
        if (index == -1) break;
        
        Serial.println("Temperature: " + getString(response, index + 14, '}'));
        
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
      }
      
      
      index = getStringIndex(response, "devEUI", index + 1);

    } 
    
  } else {
    Serial.println("- Connection to LoRaWAN device failed");
  }
  delete[] arr;
  delay(5000);  // Run every 5 seconds
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
  if (client.connect(server, HTTP_PORT)) {
    Serial.println("Getting Token...");
    JsonDocument doc;

    doc["username"] = authName;
    doc["password"] = authPass;
    String jsonData;
        
    serializeJson(doc, jsonData);
    
    client.println("POST /" + loginAPI + " HTTP/1.1");
    client.println("Host: " + String(endpoint));
    client.println("Content-Type: application/json");  // Content type
    client.println("Content-Length: " + String(jsonData.length())); 
    client.println("Connection: close");
    client.println();  // End of headers

    // Send the JSON payload
    client.println(jsonData);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Read and print the server response
    String response = client.readString();

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
