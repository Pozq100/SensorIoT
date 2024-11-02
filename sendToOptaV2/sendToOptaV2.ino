#include "config.h"

#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <String.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Scheduler.h>

// Constants
constexpr auto baudrate {9600};
constexpr auto bitduration {1.f / baudrate};
constexpr auto preDelayBR {bitduration * 9.6f * 3.5f * 1e6};
constexpr auto postDelayBR {bitduration * 9.6f * 3.5f * 1e6};

int gotError = 0;

// Initializing all the arrays
const int USER_LEDS[] = {LED_D0, LED_D1, LED_D2, LED_D3};
const int TOTAL_TYPE[] = NUM_ENERGY_METERS;

const int POWER_ADD[] = POWER_ADDRESS;
const float POWER_SF[] = POWER_SIG_FIG;
const int POWER_DL[] = POWER_DATA_LEN;

const int CURRENT_ADD[] = CURRENT_ADDRESS;
const float CURRENT_SF[] = CURRENT_SIG_FIG;
const int CURRENT_DL[] = CURRENT_DATA_LEN;

const int ENERGY_ADD[] = ENERGY_ADDRESS;
const float ENERGY_SF[] = ENERGY_SIG_FIG;
const int ENERGY_DL[] = ENERGY_DATA_LEN;

const int FC_ADD[] = FC_ADDRESS;
const int FC_ACTION_ADD[] = FC_ACTION_ADDRESS;

// WiFi
char ssid[] = WIFI_SSID;
char password[] = WIFI_PASS;

// AWS EC2 URL
char endpoint[] = AWS_IP;
String statusPath = AWS_FC_STATUS;
String endpointPath = AWS_PATH;
int PORT = AWS_PORT;
char apiKey[] = AWS_API;

// Ethernet
byte mac[] = MAC;  
byte ip[] = IP;
byte server[] = SERVER;

// LoRaWAN Endpoint URL and Port
char lorawan[] = SERVER_STRING; 
String packetAPI = "api/urpackets";
String loginAPI = "api/internal/login";

// MQTT
const char* mqtt_server = MQTT_SERVER;
int mqttPort = MQTT_PORT;
char mqttTopic[] = MQTT_TOPIC;
char mqttUser[] = MQTT_USER;
char mqttPass[] = MQTT_PASS;

int HTTP_PORT = 80;

// Authorization Token (Bearer Token)
String authName = LORAWAN_NAME;
String authPass = LORAWAN_PASS;
String token = "";



// Client Objects
EthernetClient ethClient;
WiFiClient client;
WiFiClient httpClient;
PubSubClient mqttClient(client);

// Connection Functions
void connectToWiFi();
void connectToEthernet();
void connectToModbus();
void connectToMQTT();

// Debugging Functions
void status_debug(int error_code);
void clearError();
void waitTime(int milli);

// HTTP to Database Functions
void sendDataToServer(const String& jsonData);
void updateAirConStatus();
void postData();

// Energy Meter Functions
float readEMAddress(int ID, int addr, int data_len);
double precision2double(unsigned long f);
String getEnergyData();


// LORAWAN Functions
String getToken();
int getStringIndex(String response,String p, int startIndex);
String getString(String response,int index, char lookOut);
String getLorawanData();
void deletePackets();

// Intesis Gateway Functions
String getRoomTemperatureData();

String getFCStatus(int FU);
String getFCFanStatus(int FU);
String getFCOMStatus(int FU);
float getFCSetPoint(int FU);
String getFCErrorCode(int FU);

String getAllFCErrorCodes();
String getAllFCFullStatus();
String FC_Action(int FU, int action, int action_state, int mode);

// MQTT Functions
void callback(char* topic, byte* message, unsigned int length);
void runMQTT();

void setup() {
  Serial.begin(9600);
  connectToEthernet();
  connectToWiFi();
  connectToModbus();

  while (token == ""){
    token = getToken();
    delay(1000);
  }

  Serial.println("Got Token");
  
  mqttClient.setServer(mqtt_server, MQTT_PORT);
  mqttClient.setCallback(callback);

  // Initialize LEDs Status
  for (int i = 0; i < 4; i++) {
    pinMode(USER_LEDS[i], OUTPUT);
  }
  
  
  postData();
  Scheduler.startLoop(runMQTT);
  
}


void loop() {
  static unsigned long lastData = 0;
  unsigned long now = millis();
  
  if (now - lastData > 300000) {
    lastData = now;
    
    if (WiFi.status() != WL_CONNECTED){
      connectToWiFi();
    }
    
    Serial.println("Start Loop");
    postData();
    Serial.println("End Loop");
  }

  yield();
}

void connectToWiFi() {

  while (WiFi.status() != WL_CONNECTED) {
    // WiFi.begin(ssid); // If no password is set up
    WiFi.begin(ssid, password);
    delay(500);

    status_debug(1);
  }
  
  if (gotError == 1){
    clearError();
  }
  
  Serial.println("WiFi Connection - Successful");
}

void connectToEthernet(){
  if(Ethernet.begin(mac, ip)) {
    Serial.println("Ethernet Connection - Successful");
  }else {
    status_debug(2);
    Serial.println("Ethernet Connection Failed");
  }
}

void connectToModbus(){
  RS485.setDelays(preDelayBR, postDelayBR);
  while (!ModbusRTUClient.begin(baudrate, SERIAL_8N1)) {
    Serial.println("Failed to start Modbus RTU Client!");
    status_debug(3);
  }
  if (gotError == 3){
    clearError();
  }
  Serial.println("Modbus RTU Connection - Successful");
}

void connectToMQTT(){
  while (!mqttClient.connected()) {
    if (mqttClient.connect("OptaClient",mqttUser,mqttPass)) {
      if (gotError == 11){
        clearError();
      }
      Serial.println("Connected to MQTT Server");
      mqttClient.subscribe(mqttTopic);
      
    } else {
      status_debug(11);
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println("Error Connecting to MQTT Server");
      delay(1000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  int startIndex = 0;
  int endIndex = 0;
  
  endIndex = messageTemp.indexOf(',');
  String action = messageTemp.substring(startIndex, endIndex);

  startIndex = endIndex;
  endIndex = messageTemp.indexOf(',', startIndex + 1);
  String action_value = messageTemp.substring(startIndex + 1, endIndex);
  
  startIndex = endIndex;
  endIndex = messageTemp.indexOf(',', startIndex + 1);
  int FC_Unit_Index = (messageTemp.substring(startIndex + 1, endIndex)).toInt();
  int FC_Unit = FC_ADD[FC_Unit_Index];

  Serial.println("Action: " + action + " Value: " + action_value + " Unit: " + FC_Unit);
  
  if (action == "temp"){
    int action_state = (int)(action_value.toFloat() * 10);
    if (FC_Action(FC_Unit + 1, 3, action_state, 0) == "success"){
      Serial.println("successful action");
    } else {
      Serial.println("error");
    }
    
  } else if (action == "power"){
    int action_state = action_value.toInt();
    if (FC_Action(FC_Unit + 1, 0, action_state, 0) == "success"){
      Serial.println("successful action");
    } else {
      Serial.println("error");
    }
    
  } else if (action == "fanmode"){
    int action_state = action_value.toInt();
    if (FC_Action(FC_Unit + 1, 2, action_state, 0) == "success"){
      Serial.println("successful action");
    } else {
      Serial.println("error");
    }

  } else if (action == "operationmode"){
    int action_state = action_value.toInt();
    if (FC_Action(FC_Unit + 1, 1, action_state, 0) == "success"){
      Serial.println("successful action");
    } else {
      Serial.println("error");
    }
    
    
  }


  //updateAirConStatus();
}

void runMQTT(){
  if (WiFi.status() != WL_CONNECTED){
    connectToWiFi();
  }
    
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  
  mqttClient.loop();

  yield();
}


void postData() {
  // Create JSON document to store sensor data
  DynamicJsonDocument allReadings_Json(JSON_BYTE * 3);
  String lorawan_readings = getLorawanData();
  String energy_readings = getEnergyData();
  String FCRT_readings = getRoomTemperatureData();
  String FC_status_readings = getAllFCFullStatus();
  String FC_Error = getAllFCErrorCodes();
  
  DynamicJsonDocument energy_doc(JSON_BYTE);
  DeserializationError error = deserializeJson(energy_doc, energy_readings);
  if (error) {
    status_debug(8);
    return;
  }
  
  DynamicJsonDocument lorawan_doc(JSON_BYTE);
  error = deserializeJson(lorawan_doc, lorawan_readings);
  if (error) {
    status_debug(8);
    return;
  }

  DynamicJsonDocument fancoil_doc(JSON_BYTE);
  error = deserializeJson(fancoil_doc, FCRT_readings);
  if (error) {
    status_debug(8);
    return;
  }

  DynamicJsonDocument allStatus_doc(JSON_BYTE);
  error = deserializeJson(allStatus_doc, FC_status_readings);
  if (error) {
    status_debug(8);
    return;
  }

  DynamicJsonDocument FCerror_doc(JSON_BYTE);
  error = deserializeJson(FCerror_doc, FC_Error);
  if (error) {
    status_debug(8);
    return;
  }
  
  if ( gotError == 8 ){
      clearError();
  }

  // Create nested objects in the allReadings_Json document
  JsonObject energyData = allReadings_Json.createNestedObject("Energy_Readings");
  energyData.set(energy_doc.as<JsonObject>());

  JsonObject lorawanData = allReadings_Json.createNestedObject("Lorawan_Readings");
  lorawanData.set(lorawan_doc.as<JsonObject>());
  
  JsonObject FC_TempData = allReadings_Json.createNestedObject("Fan_coil_RT_Readings");
  FC_TempData.set(fancoil_doc.as<JsonObject>());

  JsonObject FC_StatusData = allReadings_Json.createNestedObject("FC_FullStatus_Readings");
  FC_StatusData.set(allStatus_doc.as<JsonObject>());

  JsonObject FC_ErrorData = allReadings_Json.createNestedObject("FC_ErrorCode_Readings");
  FC_ErrorData.set(FCerror_doc.as<JsonObject>());

  // Serialize and print the combined JSON
  //serializeJson(allReadings_Json, Serial);

  String jsonData;
  serializeJson(allReadings_Json, jsonData);

  sendDataToServer(jsonData);
}

String getEnergyData(){
  DynamicJsonDocument allEnergyJson(JSON_BYTE);
  int slave_address = 1;
  for (int type = 0; type < NUM_METER_TYPES; type++){
    for (int j = 0; j < TOTAL_TYPE[type]; j++){
      JsonObject newSensorJson  = allEnergyJson.createNestedObject("Sensor_" + String(slave_address));
      if (type == 0){
        // first type for current (DTSU666)
        float sensor_current1 = readEMAddress(slave_address, 0x200C, CURRENT_DL[type]) * CURRENT_SF[type];
        float sensor_current2 = readEMAddress(slave_address, 0x200E, CURRENT_DL[type]) * CURRENT_SF[type];
        float sensor_current3 = readEMAddress(slave_address, 0x2010, CURRENT_DL[type]) * CURRENT_SF[type];
        if (sensor_current1 == -1 || sensor_current2 == -1 || sensor_current3 == -1){
          newSensorJson["Current"] = -1;
        } else {
          float currents[] = {sensor_current1, sensor_current2, sensor_current3};
          for (int m = 0; m < 2; m++) {
            for (int n = m + 1; n < 3; n++) {
              if (currents[m] > currents[n]) {
                float temp = currents[m];
                currents[m] = currents[n];
                currents[n] = temp;
              }
            }
          }
          newSensorJson["Current"] = currents[1];
        }
      } else {
        newSensorJson["Current"] = readEMAddress(slave_address, CURRENT_ADD[type], CURRENT_DL[type]) * CURRENT_SF[type];
      }

      newSensorJson["Energy"] = readEMAddress(slave_address, ENERGY_ADD[type], ENERGY_DL[type]) * ENERGY_SF[type];
      newSensorJson["Power"] = readEMAddress(slave_address, POWER_ADD[type], POWER_DL[type]) * POWER_SF[type];
      slave_address += 1;
    }
  }
  
  // Convert JSON to string
  String jsonData;
  serializeJson(allEnergyJson, jsonData);

  return jsonData;
}

String getLorawanData(){
  DynamicJsonDocument allJson(JSON_BYTE);
  
  if (ethClient.connect(server, HTTP_PORT)) {
    if ( gotError == 5 ){
      clearError();
    }
    // Prepare and send the HTTP GET request
    ethClient.println("GET /" + packetAPI + " HTTP/1.1");  // GET Request
    ethClient.println("Host: " + String(lorawan));
    ethClient.println("Authorization: Bearer " + token);  // Add the Authorization header
    ethClient.println("Connection: close");
    ethClient.println();  // End of headers

    // Wait for server response
    while (ethClient.connected()) {
      String line = ethClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Token Checking ( Since tokens change every 24H )
    String response = ethClient.readString();
    if (response.indexOf("error") != -1){
      token = getToken();
      return getLorawanData();
    } else {
      int index = getStringIndex(response, "devEUI", 0);

      while ( index != -1 ){
        String eui = getString(response, index + 9, '"');
        
        int tempIndex = getStringIndex(response, "payloadJson", index);
        
        String json = getString(response, tempIndex + 14, '}') + '}';
        json.replace("\\", "");
        
        DynamicJsonDocument jsonDoc(JSON_BYTE);
        DeserializationError err = deserializeJson(jsonDoc, json);
        if (err) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(err.c_str());
        } else if (!allJson.containsKey(eui)) {
            JsonObject newSensorJson  = allJson.createNestedObject(eui);
            index = tempIndex;
            if (jsonDoc.containsKey("battery")){
              newSensorJson["battery"] = jsonDoc["battery"];
            }
          
            if (jsonDoc.containsKey("humidity") && jsonDoc.containsKey("temperature")){
              newSensorJson["humidity"] = jsonDoc["humidity"];
              newSensorJson["temperature"] = jsonDoc["temperature"];
            }

            if (jsonDoc.containsKey("co2")){
              newSensorJson["co2"] = jsonDoc["co2"];
            }

            if (jsonDoc.containsKey("magnet_status")){
              newSensorJson["magnet_status"] = jsonDoc["magnet_status"];
            }
        } else {
          if (jsonDoc.containsKey("battery")){
            allJson[eui]["battery"] = jsonDoc["battery"];
          }
        }
        
        jsonDoc.clear();
        
        index = getStringIndex(response, "devEUI", index + 5);
      } 
      ethClient.stop();
      deletePackets();

      // Convert JSON to string
      String jsonData;
      serializeJson(allJson, jsonData);
    
      Serial.println(jsonData);
      return jsonData;
      }
    } else {
      Serial.println("- Connection to LoRaWAN device failed");
      status_debug(5);
      return "{}";
    }

        
}

void deletePackets(){
  if (ethClient.connect(server, HTTP_PORT)) {
    ethClient.println("DELETE /" + packetAPI + " HTTP/1.1");
    ethClient.println("Host: " + String(lorawan));
    ethClient.println("Authorization: Bearer " + token);
    ethClient.println("Content-Type: application/json");
    ethClient.println("Connection: close");
    ethClient.println();
  } else {
    status_debug(5);
  }

  ethClient.stop();
}

String getRoomTemperatureData(){
  DynamicJsonDocument fancoil_doc(JSON_BYTE);
  
  for (int i = 0; i < NUM_FC_UNITS; i++ ){
    int register_address = ((FC_ADD[i] + 1) * 200) + 6;
    String json_string = "FC_Unit_" + String(FC_ADD[i]);
    fancoil_doc[json_string] = (FC_Action(FC_ADD[i], 4, 0, 1)).toFloat()/10;
  }

  String jsonData;
  serializeJson(fancoil_doc, jsonData);

  return jsonData;
}

String getFCStatus(int FU){
  String result = FC_Action(FU, 0, 0, 1);
  if (result == "error"){
    return "ERROR";
  }
  int FC_status = result.toInt();
  switch (FC_status){
    case 0:
      return "OFF";
    case 1:
      return "ON";
  }
}

String getFCOMStatus(int FU){
  String result = FC_Action(FU, 1, 0, 1);
  if (result == "error"){
    return "ERROR";
  }
  int FC_OM = result.toInt();
  switch (FC_OM){
    case 0:
      return "AUTO";
    case 1:
      return "HEAT";
    case 2:
      return "COOL";
    case 3:
      return "FAN";
    case 4:
      return "DRY";
  }
}

String getFCFanStatus(int FU){
  String result = FC_Action(FU, 2, 0, 1);
  if (result == "error"){
    return "ERROR";
  }
  int FC_FAN = result.toInt();
  switch (FC_FAN){
    case 0:
      return "AUTO";
    case 1:
      return "VERY LOW";
    case 2:
      return "LOW";
    case 3:
      return "MID";
    case 4:
      return "HIGH";
    case 5:
      return "VERY HIGH";
  }
}

float getFCSetPoint(int FU){
  String result = FC_Action(FU, 3, 0, 1);
  if (result == "error"){
    return 404;
  }
  return result.toFloat() / 10.0;
}

String getFCErrorCode(int FU){
  return FC_Action(FU, 5, 0, 1);
}

String getAllFCErrorCodes(){
  DynamicJsonDocument allError_doc(JSON_BYTE);
  
  for (int i = 0; i < NUM_FC_UNITS; i++ ){
    String json_string = "FC_Unit_" + String(FC_ADD[i]);
    allError_doc[json_string] = getFCErrorCode(FC_ADD[i] + 1);
  }

  String jsonData;
  serializeJson(allError_doc, jsonData);
  return jsonData;
}

String getAllFCFullStatus(){
  DynamicJsonDocument allDoc(JSON_BYTE);
  for (int i = 0; i < NUM_FC_UNITS; i++ ){
    JsonObject newFCJson  = allDoc.createNestedObject("FC_Unit_" + String(FC_ADD[i]));
    for (int j = 0; j < 4; j++ ){
      int FU = FC_ADD[i] + 1;
      newFCJson["Status"] = getFCStatus(FU);
      newFCJson["Fan_Status"] = getFCFanStatus(FU);
      newFCJson["Set_Point"] = getFCSetPoint(FU);
      newFCJson["Operation_Mode"] = getFCOMStatus(FU);
    } 
  }

  String jsonData;
  serializeJson(allDoc, jsonData);
  return jsonData;
}

String FC_Action(int FU, int action, int action_state ,int mode){
  /*
   * ACTION == 0, ON and OFF
   * ACTION == 1, OPERATION MODE
   * ACTION == 2, FAN SPEED
   * ACTION == 3, TEMPERATURE SET POINT
   * ACTION == 4, ROOM TEMPERATURE
   * ACTION == 5, ERROR CODE
   */
  /*
   * MODE == 0, WRITE
   * MODE == 1, READ
   */
  int register_address = (FU * 200) + FC_ACTION_ADD[action];
  if (mode == 0){
    // Write
    if (ModbusRTUClient.holdingRegisterWrite(INTESIS_GATEWAY, register_address, action_state)) {
      return "success";  
    } else {
      Serial.println("Failed to write to holding register.");
      status_debug(9);
      return "error";
    }
    

  } else if (mode == 1) {
    // Read
    if (ModbusRTUClient.holdingRegisterRead(INTESIS_GATEWAY, register_address) == -1) {
      status_debug(10);
      return "error";
    } else {
      if (gotError == 10){
        clearError();
      }
      return String(ModbusRTUClient.holdingRegisterRead(INTESIS_GATEWAY, register_address));
    }
  }
}

void updateAirConStatus(){
  String FC_status_readings = getAllFCFullStatus();
  if (WiFi.status() == WL_CONNECTED) {
    connectToWiFi();
  }
  
  if (httpClient.connect(endpoint, PORT)) {
    if (gotError == 7){
      clearError();
    }
    
    // Prepare HTTP POST request
    httpClient.println("PUT " + statusPath + " HTTP/1.1");
    httpClient.println("Host: " + String(endpoint));
    httpClient.println("Content-Type: application/json");
    httpClient.print("Content-Length: ");
    httpClient.println(FC_status_readings.length());
    httpClient.print("x-api-key: ");
    httpClient.println(apiKey);
    httpClient.println();  // End of headers
    httpClient.println(FC_status_readings);  // Send JSON data
    
  } else {
    status_debug(7);
  }

  httpClient.stop();
}

void sendDataToServer(const String& jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    connectToWiFi();
  }
  
  if (httpClient.connect(endpoint, PORT)) {
    if (gotError == 7){
      clearError();
    }
    
    // Prepare HTTP POST request
    httpClient.println("POST " + endpointPath + " HTTP/1.1");
    httpClient.println("Host: " + String(endpoint));
    httpClient.println("Content-Type: application/json");
    httpClient.print("Content-Length: ");
    httpClient.println(jsonData.length());
    httpClient.print("x-api-key: ");
    httpClient.println(apiKey);
    httpClient.println();  // End of headers
    httpClient.println(jsonData);  // Send JSON data
    
    // Print server response
    String response = httpClient.readString();
    Serial.println("Server response:");
    Serial.println(response);
  } else {
    status_debug(7);
  }

  httpClient.stop();
}

// Function for debugging with LED status
void status_debug(int error_code){
  // LED 1,2,3,4 are used, with LED 1 being the MSD and LED 4 being the LSD
  // In perfect working condition, LED 1-4 are LOW
  /*
  * ERROR 1 - WiFi Connection Failed
  * ERROR 2 - Ethernet Connection Failed
  * ERROR 3 - Modbus Connection Failed
  * ERROR 4 - Modbus Request Failed at readEMAddress Function
  * ERROR 5 - Lorawan connection Failed
  * ERROR 6 - Lorawan getToken Failed
  * ERROR 7 - AWS connection Failed
  * ERROR 8 - Parsing of Readings Failed
  * ERROR 9 - Failed to write to Intesis Gateway
  * ERROR 10 - Failed to read from Intesis Gateway
  * ERROR 11 - Failed to Connect to MQTT Server
  */
  int sig_digit = 3;
  
  clearError();
  
  while (error_code != 0){
    if (error_code == 1){
      digitalWrite(USER_LEDS[sig_digit], HIGH);
      break;
    }else if (error_code % 2 == 1){
      digitalWrite(USER_LEDS[sig_digit], HIGH);
    }
    error_code /= 2;
    sig_digit --;
  }

  gotError = error_code;

 
}

void clearError(){
  digitalWrite(USER_LEDS[3], LOW); 
  digitalWrite(USER_LEDS[2], LOW); 
  digitalWrite(USER_LEDS[1], LOW);
  digitalWrite(USER_LEDS[0], LOW);

  gotError = 0;
}

float readEMAddress(int ID, int addr, int data_len) {
  waitTime(1000);
  
  if (!ModbusRTUClient.requestFrom(ID, INPUT_REGISTERS, addr, data_len)) {
    // Failed Modbus Request
    status_debug(4);
    return -1;
  } else {
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
  if (ethClient.connect(lorawan, HTTP_PORT)) {
    if (gotError == 5){
      clearError();
    }
    Serial.print("Getting Token... ");
    JsonDocument doc;

    doc["username"] = authName;
    doc["password"] = authPass;
    String jsonData;
        
    serializeJson(doc, jsonData);
    
    ethClient.println("POST /" + loginAPI + " HTTP/1.1");
    ethClient.println("Host: " + String(lorawan));
    ethClient.println("Content-Type: application/json");  // Content type
    ethClient.println("Content-Length: " + String(jsonData.length())); 
    ethClient.println("Connection: close");
    ethClient.println();  // End of headers

    // Send the JSON payload
    ethClient.println(jsonData);

    while (ethClient.connected()) {
      String line = ethClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Read and print the server response
    String response = ethClient.readString();

    // Parse JSON response
    JsonDocument doc2;
    DeserializationError error = deserializeJson(doc2, response);

    if (error) {
      Serial.print("DeserializeJson() failed: ");
      Serial.println(error.c_str());
      status_debug(6);
      return "";
    }

    // Extract JWT from the JSON response
    String jwt = doc2["jwt"].as<String>();
    
    ethClient.stop();
    return jwt;
  } else {
    status_debug(5);
    
    ethClient.stop();
    return "";
  }
}

void waitTime(int milli){
  unsigned long startTime = millis();
  while (millis() - startTime < milli){
    yield();
  }
}
