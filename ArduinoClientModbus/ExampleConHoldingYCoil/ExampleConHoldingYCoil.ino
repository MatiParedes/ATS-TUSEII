#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

// Network configuration
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 17);        // Arduino IP
IPAddress server(192, 168, 0, 200);   // Modbus slave simulator IP

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);

const int ledPin = 4; // Built-in LED pin

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(ledPin, OUTPUT);
  
  // Initialize Ethernet
  Ethernet.begin(mac, ip);
  
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("W5100 not found!");
    while (true);
  }
  
  Serial.println("Modbus TCP Client ready");
}

void loop() {
  // Connect to Modbus server if not connected
  if (!modbusTCPClient.connected()) {
    Serial.println("Connecting to Modbus server...");
    
    if (!modbusTCPClient.begin(server, 502)) {
      Serial.println("Connection failed!");
      delay(5000);
      return;
    }
    Serial.println("Connected to Modbus server!");
  }
  
  // Read single coil
  readCoilAndControlLED();
  
  // Read multiple coils
  readMultipleCoils();
  
  // Read single holding register
  readSingleHoldingRegister();
  
  // Read multiple holding registers
  readMultipleHoldingRegisters();
  
  delay(2000);
}

void readCoilAndControlLED() {
  Serial.print("Reading coil 0: ");
  
  int coilValue = modbusTCPClient.coilRead(0);
  
  if (coilValue == -1) {
    Serial.print("Failed! Error: ");
    Serial.println(modbusTCPClient.lastError());
  } else {
    Serial.println(coilValue ? "ON" : "OFF");
    
    // Control LED based on coil state
    digitalWrite(ledPin, coilValue ? HIGH : LOW);
  }
}

void readMultipleCoils() {
  Serial.print("Reading coils 1-5: ");
  
  if (!modbusTCPClient.requestFrom(1, COILS, 1, 5)) {
    Serial.print("Failed! Error: ");
    Serial.println(modbusTCPClient.lastError());
  } else {
    Serial.print("Values: ");
    while (modbusTCPClient.available()) {
      Serial.print(modbusTCPClient.read());
      Serial.print(" ");
    }
    Serial.println();
  }
}

void readSingleHoldingRegister() {
  Serial.print("Reading holding register 0: ");
  
  long registerValue = modbusTCPClient.holdingRegisterRead(0);
  
  if (registerValue == -1) {
    Serial.print("Failed! Error: ");
    Serial.println(modbusTCPClient.lastError());
  } else {
    Serial.println(registerValue);
  }
}

void readMultipleHoldingRegisters() {
  Serial.print("Reading holding registers 1-3: ");
  
  if (!modbusTCPClient.requestFrom(1, HOLDING_REGISTERS, 1, 3)) {
    Serial.print("Failed! Error: ");
    Serial.println(modbusTCPClient.lastError());
  } else {
    Serial.print("Values: ");
    while (modbusTCPClient.available()) {
      Serial.print(modbusTCPClient.read());
      Serial.print(" ");
    }
    Serial.println();
  }
}