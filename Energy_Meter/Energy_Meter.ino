s#include <ArduinoModbus.h>
#include <ArduinoRS485.h>

constexpr auto baudrate { 9600 };
constexpr auto bitduration { 1.f / baudrate };
constexpr auto preDelayBR { bitduration * 9.6f * 3.5f * 1e6 };
constexpr auto postDelayBR { bitduration * 9.6f * 3.5f * 1e6 };

int ID = 35;

float readDDSUAddress(int ID, int addr);
double precision2double(unsigned long f);

float readDTSUAddress1(int ID, int addr);


void setup() {
    Serial.begin(9600);
    while (!Serial);

    Serial.println("Modbus RTU Client");

    RS485.setDelays(preDelayBR, postDelayBR);

    // Start the Modbus RTU client
    if (!ModbusRTUClient.begin(baudrate, SERIAL_8E1)) {
        Serial.println("Failed to start Modbus RTU Client!");
        while (1);
    }
}

void loop() {
    //Serial.println(readDDSUAddress(ID, 0x2000)); // Voltage
    //Serial.println(readDDSUAddress(ID, 0x2002)); // Current
    //Serial.println(readDDSUAddress(ID, 0x4000)); // Energy
    Serial.println(readDTSUAddress1(ID, 0x101E)); // Positive Total Energy
    Serial.println(readDTSUAddress1(ID, 0x1028)); // Negative Total Energy
    // Combined Active Energy = Positive - Negative
    // Forward Active Energy = Postive
    
    delay(5000);
    Serial.println();
}

float readDDSUAddress(int ID, int addr) {
   // Uses 101EH as the communication address
   Serial.print("Reading values ... ");
   if (!ModbusRTUClient.requestFrom(ID, INPUT_REGISTERS,  addr, 2)) {
      Serial.print("failed! ");
      Serial.println(ModbusRTUClient.lastError()); 
    } else {
      Serial.println("success");
      unsigned long firstByte = ModbusRTUClient.read();
      unsigned long secondByte = ModbusRTUClient.read();
      unsigned long final = (firstByte << 16) | secondByte;
      return precision2double(final);
    }
    return 0.0;
}

float readDTSUAddress1(int ID, int addr){
   Serial.print("Reading values ... ");
   if (!ModbusRTUClient.requestFrom(ID, INPUT_REGISTERS,  addr, 2)) {
      Serial.print("failed! ");
      Serial.println(ModbusRTUClient.lastError()); 
    } else {
      Serial.println("success");
      unsigned long firstByte = ModbusRTUClient.read();
      unsigned long secondByte = ModbusRTUClient.read();
      unsigned long final = (firstByte << 16) | secondByte;
      return precision2double(final);
    }
    return 0.0;
}

struct ieee754{
  unsigned long m:23; // Mantissa
  unsigned long e:8;  // Exponent
  unsigned long s:1;  // Sign
};

double precision2double(unsigned long f){
  struct ieee754 v;
  memcpy(&v, &f, sizeof(f));  // copy f ( Byte ) into v ( IEEE754 )
  double r = (1+v.m * pow(2, -23)) * pow(2, v.e - 127);
  return r;
}
