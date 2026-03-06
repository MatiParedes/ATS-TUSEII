/*
 * CODIGO 3: HELTEC V3.2 (Receptor LoRa P2P + Gateway Firebase)
 * -------------------------------------------
 * Recibe un byte por LoRa P2P.
 * Se conecta a Wi-Fi.
 * Desempaqueta el byte en 8 valores (coils).
 * Construye un JSON y lo sube a Firebase Realtime Database.
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Para construir el JSON fácilmente

// Configuración Wi-Fi
const char* ssid = "WiFi UNS";
const char* password = "";
// -------------------------------------------------

// Configuración Firebase
// Ve a tu Realtime Database -> Copia la URL
// Debe terminar en ".firebaseio.com"
// "/nombre_nodo.json" al final
String firebaseURL = "https://loraesp32-tuseii-default-rtdb.firebaseio.com/datosLoRa.json";
// Va a almacenar los valores en datosLoRa

// Configuración de Radio (DEBE COINCIDIR CON EL TX)
uint32_t frecuencia = 915E6; 
const int spreadingFactor = 7;
const int bandwidth = 0;
const int codingRate = 1;
const int preambleLength = 8;
const bool crc = true;
// -------------------------------------------------

// Declaración de la función que sube los datos
void processAndUpload(byte datosRecibidos);

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi,rxSize;

bool lora_idle = true;

// Callback: se llama cuando un paquete LoRa es recibido
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Serial.print("\nRX Done. RSSI: ");
  Serial.print(rssi);
  Serial.print(", SNR: ");
  Serial.println(snr);

  // Verificamos que el paquete tenga el tamaño esperado (1 byte)
  if (size == 1)
  {
    byte byteRecibido = payload[0];
    Serial.print("Byte recibido por LoRa: 0b");
    Serial.println(byteRecibido, BIN);

    // Llama a la función para procesar y subir a Firebase
    processAndUpload(byteRecibido);
    
  } 
  else
  {
    Serial.print("Error: Paquete de tamaño inesperado (");
    Serial.print(size);
    Serial.println(" bytes)");
  }
  
  // Vuelve a poner la radio en modo Recepción Continua
  Radio.Rx(0); 
}

// Callback: se llama si la recepción falla (timeout)
void OnRxTimeout(void)
{
  Serial.println("RX Timeout");
  // Vuelve a poner la radio en modo Recepción Continua
  Radio.Rx(0);
}

void OnRxError(void)
{
  Serial.println("RX Error");
  // Vuelve a poner la radio en modo Recepción Continua
  Radio.Rx(0);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    // esperar
  }
  
  Serial.println("Heltec V3.2 - Receptor LoRa P2P / Gateway Firebase");

  // Inicia la placa
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

  // Conectar a Wi-Fi
  Serial.print("Conectando a Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado. IP:");
  Serial.println(WiFi.localIP());

  // Configura los callbacks de la radio
  Radio.Init(&RadioEvents);
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  // Configura la radio para P2P (modo RX)
  Radio.SetChannel(frecuencia);
  Radio.SetRxConfig(MODEM_LORA, bandwidth, spreadingFactor, codingRate, 0, preambleLength, 0, false, 0, crc, 0, 0, false, true); // (último true = RX Continua)

  // Le dice a la librería que no estamos usando una red pública
  Radio.SetPublicNetwork(false); 
  
  // Inicia la recepción continua
  Serial.println("Radio RX lista. Esperando paquetes LoRa...");
  Radio.Rx(0); 
}

void loop() 
{
  // La radio necesita ser procesada en cada ciclo
  Radio.IrqProcess();
}

// --- Función de Procesamiento y Subida ---
void processAndUpload(byte datosRecibidos)
{
  // 1. --- Desempaquetado de Datos y Creación de JSON ---
  // Usamos ArduinoJson
  // Asigna un tamaño de memoria. 256 bytes es suficiente.
  StaticJsonDocument<256> doc;

  Serial.println("Desempaquetando byte y creando JSON...");

  for (int i = 0; i < 8; i++)
  {
    // Crea la clave (ej: "coil_1", "coil_2", ...)
    String clave = "coil_" + String(i + 1);

    // Lee el bit en la posición 'i' del byte recibido
    // bitRead(variable, posicion) -> devuelve 0 (false) o 1 (true)
    if (bitRead(datosRecibidos, i))
    {
      doc[clave] = 1;
    }
    else
    {
      doc[clave] = 0;
    }
  }

  // Convierte el documento JSON a un String
  String jsonString;
  serializeJson(doc, jsonString);

  Serial.print("JSON a enviar: ");
  Serial.println(jsonString);

  // 2. --- Envío a Firebase ---
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    Serial.println("Iniciando HTTP POST a Firebase...");
    http.begin(firebaseURL);
    // Firebase necesita este encabezado para saber que es JSON
    http.addHeader("Content-Type", "application/json"); 

    // Realiza la petición POST con el string JSON
    int httpCode = http.POST(jsonString); 

    if (httpCode > 0)
    {
      Serial.print("Respuesta de Firebase: ");
      Serial.println(httpCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.print("Error en POST HTTP: ");
      Serial.println(httpCode);
    }

    http.end(); // Libera los recursos
  }
  else
  {
    Serial.println("Error: Wi-Fi no conectado. No se pueden subir los datos.");
    //Este error deberia notificarse con luz indicadora
  }
}