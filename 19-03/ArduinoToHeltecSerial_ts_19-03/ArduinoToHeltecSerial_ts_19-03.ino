#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>

//Config de la red
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip (192, 168, 0, 17);
IPAddress server (192, 168, 0, 201);

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);

//Control de envío
byte estadoAnterior = 255;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ALIVE = 3600000; //ms

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);   // TX1 pin 18 en ardu → Heltec pin 47

  while (!Serial) {}

  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Error con el W5100!");
  }

  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Error con el cable Ethernet");
  }

  Serial.println("Cliente Modbus listo!");
}

void loop()
{
  //Reconexión automática
  if (!modbusTCPClient.connected())
  {
    Serial.println("Conectando al servidor Modbus...");

    if (!modbusTCPClient.begin(server, 502))
    {
      Serial.println("Falló la conexión, reintentando...");
      delay(5000);
      return;
    }

    Serial.println("Conectado al servidor Modbus!");
  }

  //Lee y empaqueta los 8 coils
  byte empaquetado = 0;
  bool errorLectura = false;

  for (int i = 0; i < 8; i++)
  {
    int estado = modbusTCPClient.coilRead(i);

    if (estado == -1)
    {
      Serial.print("Error leyendo coil ");
      Serial.println(i);
      errorLectura = true;
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

  // Si hubo error en algún coil, no se envía
  if (errorLectura)
  {
    Serial.println("Error en lectura, no se envía");
    delay(500);
    return;
  }

  unsigned long ahora = millis();
  bool dataChange = (empaquetado != estadoAnterior);
  bool aliveTime = (ahora - ultimoEnvio >= INTERVALO_ALIVE); 
  //aliveTime es modificable por el usuario. Es el tiempo entre envíos de "control" para corroborar que el sistema de comunicación sigue conectado

  if (dataChange)
  {
    Serial.print("Cambio detectado! Enviando: 0b");
    Serial.println(empaquetado, BIN);
    Serial1.write(empaquetado);
    estadoAnterior = empaquetado;
    ultimoEnvio = ahora;
  }
  else if (aliveTime)
  {
    Serial.print("Sin cambios - I'm alive. Enviando: 0b");
    Serial.println(empaquetado, BIN);
    Serial1.write(empaquetado);
    ultimoEnvio = ahora;
  }
  else
  {
    Serial.println("Sin cambios, no se envía nada.");
  }

  delay(500); //ms
}