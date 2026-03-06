#include <SPI.h> //Serial Peripheral Interface
#include <Ethernet.h> //Para conectarnos con el Ethernet Shield
#include <ArduinoModbus.h> //Para utilizar modbus
#include <SoftwareSerial.h> //para utilizar un puerto serie en el pin que se me cante

// Configuración de red
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 17);        // Arduino IP
IPAddress server(192, 168, 0, 201);    // Modbus slave simulator IP

SoftwareSerial SerialToESP(31, 30); // le pongo los pines que quiero, RX y TX en ese orden

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient); //comando principal para enviar comandos Modbus

void setup()
{
  Serial.begin(9600);
  SerialToESP.begin(9600); //inicializo el serial virtual
  while (!Serial);
  
  // Ethernet
  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Error con el W5100!");
  }
  
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Error con el cable Ethernet");
  }
  
  Serial.println("Cliente ModbusTCP listo!");
}

void loop()
{ //aca vemos si esta conectado, si no, intentamos conectar
  if (!modbusTCPClient.connected()
  {
    Serial.println("Conectando al server Modbus...");
    
    if (!modbusTCPClient.begin(server, 502))
    {
      Serial.println("Falló la conexión :(");
      delay(5000);
      return;
    }
    Serial.println("Conectado al servidor Modbus!");
  }
  
  delay(1000);

  byte empaquetado = 0; //creo un byte, y voy leyendo y guardando el estado de cada coil

  for (int i = 0; i < 8; i++)
  {
    int estado = modbusTCPClient.coilRead(i);

    if (estado == -1)
    {
      Serial.print("Error leyendo coil ");
      Serial.println(i);
    }
    else
    {
      bitWrite(empaquetado, i, estado);
      Serial.print("Coil ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(estado);
    }
  }

  Serial.println("-----------------------");
  Serial.print("Enviando : ");
  Serial.println(empaquetado, BIN);
  //Serial.println("Coils empaquetados (BIN): ");
  //for (int b = 7; b >= 0; b--) {  // recorre los bits de mayor a menor
  //  Serial.print(bitRead(empaquetado, b));
  //  Serial.println();

    SerialToESP.write(empaquetado); //mando crudo el dato
  
  delay(10000);
}
