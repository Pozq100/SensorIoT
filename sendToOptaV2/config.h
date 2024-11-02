#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID "eee-iot"
#define WIFI_PASS "I0t@mar2025!"

// AWS Configuration
#define AWS_USER_ID "xxxx"
#define AWS_IP "54.252.31.39"
#define AWS_PATH "/web/lorawan"
#define AWS_FC_STATUS "web/aircon_status"
#define AWS_PORT 8080
#define AWS_API "95130"

// MQTT Configuration
#define MQTT_SERVER "3.25.195.46"
#define MQTT_PORT 3000
#define MQTT_TOPIC "arduino/read"
#define MQTT_USER "user_01"
#define MQTT_PASS "superman101"

// Energy Meter Configeration
#define NUM_METER_TYPES 2
#define NUM_ENERGY_METERS { 2, 4 }

#define POWER_ADDRESS { 0x2012, 0x2004}
#define POWER_SIG_FIG { 0.0001, 1 }
#define POWER_DATA_LEN { 2, 2 }

#define CURRENT_ADDRESS { 0, 0x2002 }
#define CURRENT_SIG_FIG { 0.001, 1 }
#define CURRENT_DATA_LEN { 2, 2 }

#define ENERGY_ADDRESS { 0x101E, 0x4000 }
#define ENERGY_SIG_FIG { 1, 1 }
#define ENERGY_DATA_LEN { 2, 2 }

#define JSON_BYTE 1024

#define MAC { 0xA8, 0x61, 0x0A, 0x50, 0x64, 0x63 }
#define IP { 192, 168, 23, 200 }
#define SERVER { 192, 168, 23, 150 }
#define SERVER_STRING "192.168.23.150"

#define LORAWAN_NAME "admin"
#define LORAWAN_PASS "NicJjG18XOV3U1efQyo8AQ=="

#define INTESIS_GATEWAY 7
#define NUM_FC_UNITS 8
#define FC_ADDRESS {1, 2, 3, 4, 5, 6, 7, 8}
#define FC_ACTION_ADDRESS {0, 1, 3, 5, 6, 42}


#endif // CONFIG_H
