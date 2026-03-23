#include "LoRaWan_APP.h" //Maneja comunicación LoRa
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h> //Para Firebase
#include <ArduinoJson.h> //Manejo de json para Firebase
#include <time.h> //Para coordinar NTP

//WiFi, idealmente esto se haría desde una interfaz de usuario configurable
const char* ssid = "Matute";
const char* password = "martincito";

//URL de la base de datos en Firebase
String firebaseBase = "https://loraesp32-tuseii-default-rtdb.firebaseio.com";

//NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset = -10800;  // UTC-3 en segundos
const int daylightOffset = 0;

//Configuración de LoRa
uint32_t frecuencia = 915E6;
const int spreadingFactor = 7;
const int bandwidth = 0;
const int codingRate = 1;
const int preambleLength = 8;
const bool crc = true;

void processAndUpload(byte datosRecibidos);
String getTimestamp();
int getHistorialCount();

static RadioEvents_t RadioEvents;

//LoRa, callbacks y reporte de error en Serial
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Serial.print("\nRX Done. RSSI: ");
  Serial.print(rssi);
  Serial.print(", SNR: ");
  Serial.println(snr);

  if (size == 1)
  {
    byte byteRecibido = payload[0];
    Serial.print("Byte recibido: 0b");
    Serial.println(byteRecibido, BIN);
    processAndUpload(byteRecibido);
  }
  else
  {
    Serial.print("Error: tamaño inesperado (");
    Serial.print(size);
    Serial.println(" bytes)");
  }

  Radio.Rx(0);
}

void OnRxTimeout(void)
{
  Serial.println("RX Timeout");
  Radio.Rx(0);
}

void OnRxError(void)
{
  Serial.println("RX Error");
  Radio.Rx(0);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Receptor LoRa / Gateway Firebase");

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  //WiFi
  Serial.print("Conectando a Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());

  //Sincronizado de NTP
  Serial.println("Sincronizando hora con NTP...");
  configTime(gmtOffset, daylightOffset, ntpServer);

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.println("Esperando hora NTP...");
    delay(1000);
  }
  Serial.println("Hora sincronizada:");
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");

  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  Radio.Init(&RadioEvents);

  Radio.SetChannel(frecuencia);
  Radio.SetRxConfig(MODEM_LORA, bandwidth, spreadingFactor, codingRate, 0, preambleLength, 0, false, 0, crc, 0, 0, false, true);
  Radio.SetPublicNetwork(false);

  Serial.println("Radio RX lista. Esperando paquetes LoRa...");
  Radio.Rx(0);
}

void loop()
{
  Radio.IrqProcess();
}

String getTimestamp()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "sin_hora";
  }

  char buffer[30];

  strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  return String(buffer);
}

int getHistorialCount()
{
  HTTPClient http;
  String url = firebaseBase + "/historial.json?shallow=true";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200)
  {
    http.end();
    return 0;
  }

  String response = http.getString();
  http.end();

  // Contar cuántas claves hay en el JSON devuelto
  // Cada clave tiene formato "clave":true
  int count = 0;
  int pos   = 0;
  while ((pos = response.indexOf("true", pos)) != -1)
  {
    count++;
    pos += 4;
  }

  return count;
}

String getOldestKey()
{
  HTTPClient http;
  String url = firebaseBase + "/historial.json?orderBy=\"$key\"&limitToFirst=1";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET();

  if (httpCode != 200)
  {
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();

  int start = response.indexOf('"') + 1;
  int end   = response.indexOf('"', start);

  if (start == -1 || end == -1) return "";

  return response.substring(start, end);
}

//Firebase
void processAndUpload(byte datosRecibidos)
{
  //Construye JSON con los coils
  StaticJsonDocument<256> doc;
  for (int i = 0; i < 8; i++)
  {
    String clave = "coil_" + String(i + 1);
    doc[clave]   = bitRead(datosRecibidos, i) ? 1 : 0;
  }

  doc["timestamp"] = getTimestamp();

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.print("JSON: ");
  Serial.println(jsonString);

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Error: Wi-Fi no conectado.");
    return;
  }

  HTTPClient http;
  int httpCode;

  //PUT sobreescribe el ultimo dato
  //El otro que usabamos lo sumaba

  Serial.println("Subiendo ultimo_dato...");
  http.begin(firebaseBase + "/ultimo_dato.json");
  http.addHeader("Content-Type", "application/json");
  httpCode = http.sendRequest("PUT", jsonString);
  Serial.print("ultimo_dato response: ");
  Serial.println(httpCode);
  http.end();

  int count = getHistorialCount();
  Serial.print("Entradas en historial: ");
  Serial.println(count);

  if (count >= 100)
  {
    String oldestKey = getOldestKey();
    if (oldestKey != "")
    {
      Serial.print("Borrando entrada antigua: ");
      Serial.println(oldestKey);

      http.begin(firebaseBase + "/historial/" + oldestKey + ".json");
      httpCode = http.sendRequest("DELETE", "");
      Serial.print("DELETE response: ");
      Serial.println(httpCode);
      http.end();
    }
  }

  String timestamp = getTimestamp();
  Serial.print("Subiendo historial en: ");
  Serial.println(timestamp);

  http.begin(firebaseBase + "/historial/" + timestamp + ".json");
  http.addHeader("Content-Type", "application/json");
  httpCode = http.sendRequest("PUT", jsonString);
  Serial.print("historial response: ");
  Serial.println(httpCode);
  http.end();
}