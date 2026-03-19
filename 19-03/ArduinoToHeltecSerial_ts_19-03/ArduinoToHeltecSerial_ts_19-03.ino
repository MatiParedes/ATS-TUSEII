#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>

// Configuración de red
byte mac[]          = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip        (192, 168, 0, 17);
IPAddress server    (192, 168, 0, 201);

EthernetClient   ethClient;
ModbusTCPClient  modbusTCPClient(ethClient);

// Control de envío
byte estadoAnterior           = 255;       // Valor imposible → fuerza envío inicial
unsigned long ultimoEnvio     = 0;
const unsigned long INTERVALO_ALIVE = 3600000; // 1 hora en ms

// ── Setup ───────────────────────────────────────────────────
void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);   // TX1 → pin 18 → Heltec GPIO 47

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

  Serial.println("Cliente ModbusTCP listo!");
}

// ── Loop ────────────────────────────────────────────────────
void loop()
{
  // Reconexión automática si se pierde el servidor Modbus
  if (!modbusTCPClient.connected())
  {
    Serial.println("Conectando al servidor Modbus...");

    if (!modbusTCPClient.begin(server, 502))
    {
      Serial.println("Falló la conexión, reintentando en 5s...");
      delay(5000);
      return;
    }

    Serial.println("Conectado al servidor Modbus!");
  }

  // Leer y empaquetar los 8 coils
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

  Serial.println("-----------------------");

  // Si hubo error en algún coil, no enviamos para no mandar datos corruptos
  if (errorLectura)
  {
    Serial.println("Error en lectura, no se envía.");
    delay(500);
    return;
  }

  unsigned long ahora    = millis();
  bool huboCambio        = (empaquetado != estadoAnterior);
  bool pasaUnaHora       = (ahora - ultimoEnvio >= INTERVALO_ALIVE);

  if (huboCambio)
  {
    Serial.print("Cambio detectado! Enviando: 0b");
    Serial.println(empaquetado, BIN);
    Serial1.write(empaquetado);
    estadoAnterior = empaquetado;
    ultimoEnvio    = ahora;
  }
  else if (pasaUnaHora)
  {
    Serial.print("Sin cambios - I'm alive. Enviando: 0b");
    Serial.println(empaquetado, BIN);
    Serial1.write(empaquetado);
    ultimoEnvio = ahora;
  }
  else
  {
    Serial.println("Sin cambios, no se envía.");
  }

  delay(500); // Polling cada 500ms
}