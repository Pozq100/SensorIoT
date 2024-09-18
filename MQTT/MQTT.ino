#include <WiFi.h>              // For Arduino Opta's Wi-Fi connectivity
#include <PubSubClient.h>

// Replace with your network credentials
const char* ssid = "eee-iot";
const char* password = "I0t@eee2024!";

// Replace with your MQTT broker address
const char* mqtt_server = "54.252.31.39";

// Create a Wi-Fi client
WiFiClient espClient;
// Create an MQTT client
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to Wi-Fi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("OptaClient")) {
      Serial.println("connected");
      client.subscribe("test/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publish a message every 10 seconds
  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    char msg[50];
    snprintf(msg, 50, "Hello world! %lu", now);
    Serial.print("Publishing message: ");
    Serial.println(msg);
    client.publish("test/topic", msg);
  }
}
