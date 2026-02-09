#include <SoftwareSerial.h>

#define RX_PIN 48
#define TX_PIN 47 
#define LED_PIN 35

SoftwareSerial SerialFromArduino(RX_PIN, TX_PIN);

void setup() {
  Serial.begin(9600);               
  SerialFromArduino.begin(9600);    
  pinMode(LED_PIN, OUTPUT);
  Serial.println("Esperando datos...");
}

void loop() {
  if (SerialFromArduino.available()) {
    String mensaje = SerialFromArduino.readStringUntil('\n');
    Serial.print("Recibimos: ");
    Serial.println(mensaje);

    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
  }

}