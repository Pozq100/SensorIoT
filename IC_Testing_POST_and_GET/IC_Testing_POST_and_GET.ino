#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <String.h>

// WIFI Connection ( Singapore Polytechnic IoT network )
char ssid[] = "eee-iot";
char password[] = "I0t@eee2024!";
int status  = WL_IDLE_STATUS;

// AWS Endpoint URL
char endpoint[] = "54.252.31.39"; //rest API
String endpointEntity = "/web";
int PORT = 8080;

char apiKey[] = "95130";  // Your API key

// Variables ---------------------------------------------------------------
WiFiClient client;

// Functions ---------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  while (status != WL_CONNECTED) {
    Serial.print("- Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, password);
    delay(500);
  }
  
  // Display Wi-Fi network information
  Serial.println();
  Serial.println("- NETWORK INFORMATION");
  Serial.print("- You're now connected to the network ");
  
}

void loop() {

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["sensor"] = "temperature";
  jsonDoc["value"] = 24.5;
  jsonDoc["unit"] = "C";

  // Convert the JSON object into a string
  String jsonData;
  serializeJson(jsonDoc, jsonData);
  
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connect(endpoint, PORT)) { // Connect to the server on port 80

      // Prepare the HTTP request
      client.println("POST " + endpointEntity + " HTTP/1.1");
      client.println("Host: 54.252.31.39:8080/web");
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(jsonData.length());
      client.print("x-api-key: ");  // Custom header for API key
      client.println(apiKey);  // Send the API key
      client.println(); // End of headers
      client.println(jsonData);  // Post the JSON data

      // Wait for server response
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }

      // Print the response from the server
      String response = client.readString();
      Serial.println("Server response:");
      Serial.println(response);
    } else {
      Serial.println("Connection to server failed");
    }

    client.stop();  // Close the connection
  } else {
    Serial.println("WiFi disconnected");
  }
  
  
  delay(2000);  // Wait for response

}
