#include "LoRaWan_APP.h"
#include "Arduino.h"

// Configuración de Radio
// Definimos la frecuencia para TX y RX, tiene que ser la misma
uint32_t frecuencia = 915E6; 
#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             -2        // dBm, el valor -2 es para fines de testeo

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,1: 250 kHz,2: 500 kHz,3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,2: 4/6,3: 4/7,4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Misma para TX y RX
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false     // Para casos donde los datos tienen la misma cantidad de bits siempre
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30        // Tamaño del payload

double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

// Parámetros P2P de LoRa
const int txPower = -2;          // Potencia de transmisión (dBm)
const int spreadingFactor = 7;   // (SF7 a SF12)
const int bandwidth = 0;         // 0=125KHz, 1=250KHz, 2=500KHz
const int codingRate = 1;        // 1=4/5, 2=4/6, 3=4/7, 4=4/8
const int preambleLength = 8;
const bool crc = true;           // Activar verificación por CRC
// -------------------------------------------------

// Buffer para enviar datos
static uint8_t txBuffer[1]; // Solo necesitamos enviar 1 byte
static bool txIniciada = false;

// Callback: se llama cuando la transmisión LoRa termina
void OnTxDone(void)
{
  Serial.println("TX Done");
  txIniciada = false;
  // No necesitamos poner la radio en RX, es solo TX
}

// Callback: se llama si la transmisión falla (timeout)
void OnTxTimeout(void)
{
  Serial.println("TX Timeout");
  txIniciada = false;
}

void setup() {
  // Inicia el Serial para depuración
  Serial.begin(115200);
  while (!Serial) {
    // esperar
  }
  
  // Inicia Serial1 para comunicarse con el Arduino Mega
  // Mismo baudrate (9600)
  // ESP32-S3 (Heltec V3) pines por defecto para Serial1:
  // RX = 47, TX = 48
  Serial1.begin(115200, SERIAL_8N1, 47, 48);

  Serial.println("Heltec V3.3 - Transmisor LoRa P2P");

  // Inicia la placa y la radio
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

  txNumber = 0;
  
  // Configura los callbacks de la radio
  Radio.Init(&RadioEvents); // Usamos NULL porque manejaremos los callbacks manualmente
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  // Configura la radio para P2P
  Radio.SetChannel(frecuencia);
  Radio.SetTxConfig(MODEM_LORA, txPower, 0, bandwidth, spreadingFactor, codingRate, preambleLength, false, crc, 0, 0, false, 3000); // Timeout de 3 seg

  // Le dice a la librería que no estamos usando una red pública LoRaWAN
  Radio.SetPublicNetwork(false); 
  
  Serial.println("Radio TX lista. Esperando byte por MODBUS...");
}

void loop() {
  // La radio necesita ser procesada en cada ciclo
  Radio.IrqProcess();

  // Si no estamos ya transmitiendo
  if (txIniciada == false) {
    // Revisa si hay un byte disponible
    if (Serial1.available() > 0) {
      byte byteDelArduino = Serial1.read();
      
      Serial.print("Byte recibido: 0b");
      Serial.println(byteDelArduino, BIN);

      // Prepara el buffer de transmisión
      txBuffer[0] = byteDelArduino;
      
      // Inicia la transmisión
      Serial.println("Iniciando transmisión LoRa...");
      Radio.Send(txBuffer, 1); // Envía el buffer de 1 byte
      txIniciada = true;
    }
  }
}