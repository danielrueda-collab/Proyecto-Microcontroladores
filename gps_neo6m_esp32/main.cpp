#include <Arduino.h>
#include <TinyGPS++.h>

// Objeto principal de la libreria TinyGPSPlus
TinyGPSPlus gps;

// UART2 del ESP32
HardwareSerial gpsSerial(2);

// Pines recomendados para UART2 en ESP32
#define GPS_RX_PIN 16   // Conectar al TX del GPS
#define GPS_TX_PIN 17   // Conectar al RX del GPS, opcional
#define GPS_BAUD   9600

// Tiempo para imprimir datos cada cierto intervalo
unsigned long previousMillis = 0;
const unsigned long interval = 2000;  // Variar tiempo de recepcion de Datos

void setup() {
  // Serial hacia el computador
  Serial.begin(115200);
  delay(1000);

  // Serial hacia el modulo GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" Prueba GPS NEO-6M V2 con ESP32");
  Serial.println(" PlatformIO + TinyGPSPlus");
  Serial.println("====================================");
  Serial.println("Conexiones:");
  Serial.println("GPS VCC -> 3.3V o 5V segun modulo");
  Serial.println("GPS GND -> GND ESP32");
  Serial.println("GPS TX  -> GPIO16 ESP32");
  Serial.println("GPS RX  -> GPIO17 ESP32 opcional");
  Serial.println("------------------------------------");
  Serial.println("Esperando datos del GPS...");
  Serial.println("Haz la prueba en exterior para obtener coordenadas.");
  Serial.println();
}

void loop() {
  // Leer todos los caracteres disponibles desde el GPS
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  // Imprimir informacion cada 1 segundo
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    Serial.println("========== ESTADO GPS ==========");

    // Verifica si el ESP32 esta recibiendo caracteres del GPS
    Serial.print("Caracteres recibidos: ");
    Serial.println(gps.charsProcessed());

    // Verifica si las tramas NMEA tienen checksum correcto
    Serial.print("Tramas validas: ");
    Serial.println(gps.sentencesWithFix());

    Serial.print("Checksums fallidos: ");
    Serial.println(gps.failedChecksum());

    // Satelites
    Serial.print("Satelites: ");
    if (gps.satellites.isValid()) {
      Serial.println(gps.satellites.value());
    } else {
      Serial.println("No disponible");
    }

    // Latitud y longitud
    if (gps.location.isValid()) {
      Serial.print("Latitud: ");
      Serial.println(gps.location.lat(), 6);

      Serial.print("Longitud: ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("Ubicacion: No valida todavia");
    }

    // Altitud
    Serial.print("Altitud: ");
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
    } else {
      Serial.println("No disponible");
    }

    // Hora UTC
    Serial.print("Hora UTC: ");
    if (gps.time.isValid()) {
      if (gps.time.hour() < 10) Serial.print("0");
      Serial.print(gps.time.hour());
      Serial.print(":");

      if (gps.time.minute() < 10) Serial.print("0");
      Serial.print(gps.time.minute());
      Serial.print(":");

      if (gps.time.second() < 10) Serial.print("0");
      Serial.println(gps.time.second());
    } else {
      Serial.println("No valida");
    }

    // Fecha UTC
    Serial.print("Fecha UTC: ");
    if (gps.date.isValid()) {
      if (gps.date.day() < 10) Serial.print("0");
      Serial.print(gps.date.day());
      Serial.print("/");

      if (gps.date.month() < 10) Serial.print("0");
      Serial.print(gps.date.month());
      Serial.print("/");

      Serial.println(gps.date.year());
    } else {
      Serial.println("No valida");
    }

    // Trama lista para telemetria
    if (gps.location.isValid()) {
      String tramaGPS = "LAT:" + String(gps.location.lat(), 6) +
                        ",LON:" + String(gps.location.lng(), 6) +
                        ",SAT:" + String(gps.satellites.value());

      Serial.print("Trama GPS: ");
      Serial.println(tramaGPS);
    } else {
      Serial.println("Trama GPS: Sin coordenadas validas");
    }

    // Diagnostico si no llegan datos
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println();
      Serial.println("ERROR: No se reciben datos del GPS.");
      //Serial.println("Revisa:");
      //Serial.println("1. GPS TX conectado a GPIO16");
      //Serial.println("2. GPS GND conectado a GND del ESP32");
      //Serial.println("3. Alimentacion correcta del modulo");
      //Serial.println("4. Baudrate del GPS en 9600");
      //Serial.println();
    }

    Serial.println("================================");
    Serial.println();
  }
}
