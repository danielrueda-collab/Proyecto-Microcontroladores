//------ANTERIOR: SIN SENSORES NI DATOS REALES, SOLO SIMULADOS------


// #include <Arduino.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>
// #include "SPIFFS.h" // Para leer la carpeta 'data'

// const char* ssid = "Mao";
// const char* password = "123cuatro";

// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

// void setup() {
//     Serial.begin(115200);

//     if(!SPIFFS.begin(true)){ 
//         Serial.println("Error SPIFFS"); 
//         return; 
//     }

//     // La ESP32 crea su propia red WiFi
//     const char* local_ssid = "Telemetria_Cohete";
//     const char* local_pass = "12345678";
    
//     WiFi.softAP(local_ssid, local_pass);

//     IPAddress IP = WiFi.softAPIP();

//     Serial.println("Servidor creado correctamente!");
//     Serial.print("Conéctate a la red: ");
//     Serial.println(local_ssid);
//     Serial.print("IP del servidor: ");
//     Serial.println(IP); // Normalmente será 192.168.4.1

//     server.addHandler(&ws);

//     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//         request->send(SPIFFS, "/index.html", "text/html");
//     });

//     server.serveStatic("/", SPIFFS, "/");

//     server.begin();
// }

// void loop() {
//     ws.cleanupClients();

//     static unsigned long lastTime = 0;

//     if (millis() - lastTime > 200) { // Enviar datos cada 200 ms
//         lastTime = millis();
        
//         StaticJsonDocument<200> doc;

//         doc["temp"] = random(20, 30);
//         doc["acel"] = random(0, 500) / 100.0;
//         doc["alt"]  = random(0, 1000);
//         doc["baro"] = random(950, 1050);

//         String jsonString;
//         serializeJson(doc, jsonString);

//         ws.textAll(jsonString);
//     }
// }
// -------------------------------------------------------------------------------


//-------NUEVO: CON SENSORES REALES ----------------------------

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <TinyGPS++.h>

// --------------------------------------------------
// Pines I2C ESP32 DEVKIT V1
// --------------------------------------------------

#define I2C_SDA 21
#define I2C_SCL 22

// --------------------------------------------------
// Pines GPS UART2
// --------------------------------------------------

#define GPS_RX_PIN 16   // Conectar al TX del GPS
#define GPS_TX_PIN 17   // Conectar al RX del GPS, opcional
#define GPS_BAUD 9600

// --------------------------------------------------
// OLED SSD1306
// --------------------------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// --------------------------------------------------
// BMP280
// --------------------------------------------------

#define SEA_LEVEL_HPA 1013.25
#define OFFSET_CALIBRACION 4.0

// --------------------------------------------------
// MPU6050
// --------------------------------------------------

#define GRAVEDAD_TIERRA 9.80665

// --------------------------------------------------
// WiFi modo Access Point
// --------------------------------------------------

const char* local_ssid = "Telemetria_Cohete";
const char* local_pass = "12345678";

// --------------------------------------------------
// Servidor web y WebSocket
// --------------------------------------------------

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --------------------------------------------------
// Objetos sensores / pantalla
// --------------------------------------------------

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// --------------------------------------------------
// Estados de dispositivos
// --------------------------------------------------

bool oledOK = false;
bool bmpOK = false;
bool mpuOK = false;
bool gpsConDatos = false;
bool gpsFixValido = false;

// --------------------------------------------------
// Variables BMP280
// --------------------------------------------------

float temperatura = 0.0;
float temperaturaCalibrada = 0.0;
float presion_hPa = 0.0;
float altitud_m = 0.0;

// --------------------------------------------------
// Variables MPU6050
// --------------------------------------------------

float ax_ms2 = 0.0;
float ay_ms2 = 0.0;
float az_ms2 = 0.0;

float gx_dps = 0.0;
float gy_dps = 0.0;
float gz_dps = 0.0;

float aceleracion_g = 0.0;

// --------------------------------------------------
// Variables GPS
// --------------------------------------------------

double gps_latitud = 0.0;
double gps_longitud = 0.0;
double gps_altitud_m = 0.0;
double gps_velocidad_kmph = 0.0;

uint32_t gps_satelites = 0;
uint32_t gps_chars = 0;
uint32_t gps_checksums_fallidos = 0;

String gps_hora_utc = "--:--:--";
String gps_fecha_utc = "--/--/----";

// --------------------------------------------------
// Variables de tiempo
// --------------------------------------------------

unsigned long lastSensorTime = 0;
unsigned long lastWebSocketTime = 0;
unsigned long lastOLEDTime = 0;
unsigned long lastSerialTime = 0;

const unsigned long SENSOR_INTERVAL_MS = 200;
const unsigned long WEBSOCKET_INTERVAL_MS = 200;
const unsigned long OLED_INTERVAL_MS = 1000;
const unsigned long SERIAL_INTERVAL_MS = 2000;

// --------------------------------------------------
// Mostrar mensajes en OLED
// --------------------------------------------------

void mostrarMensajeOLED(
    String linea1,
    String linea2 = "",
    String linea3 = ""
) {
    if (!oledOK) {
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println(linea1);

    display.setCursor(0, 16);
    display.println(linea2);

    display.setCursor(0, 32);
    display.println(linea3);

    display.display();
}

// --------------------------------------------------
// Inicializar OLED
// --------------------------------------------------

void inicializarOLED() {
    Serial.println("Buscando OLED SSD1306...");

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("Advertencia: OLED no detectada.");
        Serial.println("El servidor continuara funcionando sin OLED.");
        oledOK = false;
        return;
    }

    oledOK = true;

    mostrarMensajeOLED(
        "OLED OK",
        "Iniciando sistema",
        "Telemetria ESP32"
    );

    Serial.println("OLED detectada correctamente.");
}

// --------------------------------------------------
// Inicializar BMP280
// --------------------------------------------------

void inicializarBMP280() {
    Serial.println("Buscando BMP280...");

    if (bmp.begin(0x76)) {
        bmpOK = true;
        Serial.println("BMP280 encontrado en direccion 0x76.");
    } 
    else if (bmp.begin(0x77)) {
        bmpOK = true;
        Serial.println("BMP280 encontrado en direccion 0x77.");
    } 
    else {
        bmpOK = false;
        Serial.println("Error: BMP280 no detectado.");
        Serial.println("Revise alimentacion, SDA, SCL y direccion I2C.");

        mostrarMensajeOLED(
            "Error BMP280",
            "No detectado",
            "Revise cables"
        );

        return;
    }

    bmp.setSampling(
        Adafruit_BMP280::MODE_NORMAL,
        Adafruit_BMP280::SAMPLING_X2,
        Adafruit_BMP280::SAMPLING_X16,
        Adafruit_BMP280::FILTER_X16,
        Adafruit_BMP280::STANDBY_MS_125
    );

    mostrarMensajeOLED(
        "BMP280 OK",
        "Buscando MPU6050",
        "Espere..."
    );
}

// --------------------------------------------------
// Inicializar MPU6050
// --------------------------------------------------

void inicializarMPU6050() {
    Serial.println("Buscando MPU6050...");

    if (!mpu.begin()) {
        mpuOK = false;

        Serial.println("Error: MPU6050 no detectado.");
        Serial.println("Revise alimentacion, SDA, SCL y direccion I2C.");

        mostrarMensajeOLED(
            "Error MPU6050",
            "No detectado",
            "Revise cables"
        );

        return;
    }

    mpuOK = true;

    Serial.println("MPU6050 encontrado correctamente.");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    mostrarMensajeOLED(
        "MPU6050 OK",
        "Iniciando GPS",
        "Espere..."
    );
}

// --------------------------------------------------
// Inicializar GPS
// --------------------------------------------------

void inicializarGPS() {
    Serial.println("Iniciando GPS Neo-6M...");

    gpsSerial.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX_PIN,
        GPS_TX_PIN
    );

    Serial.println("GPS iniciado por UART2.");
    Serial.println("GPS TX -> GPIO16 ESP32.");
    Serial.println("GPS RX -> GPIO17 ESP32 opcional.");
    Serial.println("Baudrate GPS: 9600.");
    Serial.println("Para obtener coordenadas, pruebe en exterior.");

    mostrarMensajeOLED(
        "GPS iniciado",
        "Esperando datos",
        "Use exterior"
    );
}

// --------------------------------------------------
// Leer BMP280
// --------------------------------------------------

void leerBMP280() {
    if (!bmpOK) {
        temperatura = 0.0;
        temperaturaCalibrada = 0.0;
        presion_hPa = 0.0;
        altitud_m = 0.0;
        return;
    }

    temperatura = bmp.readTemperature();
    temperaturaCalibrada = temperatura - OFFSET_CALIBRACION;
    presion_hPa = bmp.readPressure() / 100.0;
    altitud_m = bmp.readAltitude(SEA_LEVEL_HPA);
}

// --------------------------------------------------
// Leer MPU6050
// --------------------------------------------------

void leerMPU6050() {
    if (!mpuOK) {
        ax_ms2 = 0.0;
        ay_ms2 = 0.0;
        az_ms2 = 0.0;

        gx_dps = 0.0;
        gy_dps = 0.0;
        gz_dps = 0.0;

        aceleracion_g = 0.0;
        return;
    }

    sensors_event_t aceleracion;
    sensors_event_t giro;
    sensors_event_t temperaturaMPU;

    mpu.getEvent(&aceleracion, &giro, &temperaturaMPU);

    ax_ms2 = aceleracion.acceleration.x;
    ay_ms2 = aceleracion.acceleration.y;
    az_ms2 = aceleracion.acceleration.z;

    gx_dps = giro.gyro.x * 180.0 / PI;
    gy_dps = giro.gyro.y * 180.0 / PI;
    gz_dps = giro.gyro.z * 180.0 / PI;

    aceleracion_g = sqrt(
        ax_ms2 * ax_ms2 +
        ay_ms2 * ay_ms2 +
        az_ms2 * az_ms2
    ) / GRAVEDAD_TIERRA;
}

// --------------------------------------------------
// Leer caracteres del GPS
// --------------------------------------------------

void procesarGPS() {
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        gps.encode(c);
    }

    gps_chars = gps.charsProcessed();
    gps_checksums_fallidos = gps.failedChecksum();

    gpsConDatos = gps_chars > 10;
    gpsFixValido = gps.location.isValid();

    if (gps.satellites.isValid()) {
        gps_satelites = gps.satellites.value();
    }

    if (gps.location.isValid()) {
        gps_latitud = gps.location.lat();
        gps_longitud = gps.location.lng();
    }

    if (gps.altitude.isValid()) {
        gps_altitud_m = gps.altitude.meters();
    }

    if (gps.speed.isValid()) {
        gps_velocidad_kmph = gps.speed.kmph();
    }

    if (gps.time.isValid()) {
        char bufferHora[12];

        snprintf(
            bufferHora,
            sizeof(bufferHora),
            "%02d:%02d:%02d",
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second()
        );

        gps_hora_utc = String(bufferHora);
    }

    if (gps.date.isValid()) {
        char bufferFecha[16];

        snprintf(
            bufferFecha,
            sizeof(bufferFecha),
            "%02d/%02d/%04d",
            gps.date.day(),
            gps.date.month(),
            gps.date.year()
        );

        gps_fecha_utc = String(bufferFecha);
    }
}

// --------------------------------------------------
// Leer todos los sensores
// --------------------------------------------------

void leerSensores() {
    leerBMP280();
    leerMPU6050();
}

// --------------------------------------------------
// Mostrar datos en OLED
// --------------------------------------------------

void mostrarEnOLED() {
    if (!oledOK) {
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("Telemetria ESP32");

    display.setCursor(0, 12);
    display.print("T:");
    display.print(temperaturaCalibrada, 1);
    display.print("C ");

    display.print("A:");
    display.print(aceleracion_g, 1);
    display.print("G");

    display.setCursor(0, 24);
    display.print("AltB:");
    display.print(altitud_m, 1);
    display.print("m");

    display.setCursor(0, 36);
    display.print("GPS:");
    display.print(gpsFixValido ? "FIX" : "NOFIX");
    display.print(" Sat:");
    display.print(gps_satelites);

    display.setCursor(0, 48);
    display.print("Lat:");
    if (gpsFixValido) {
        display.print(gps_latitud, 4);
    } else {
        display.print("--");
    }

    display.setCursor(0, 56);
    display.print("Lon:");
    if (gpsFixValido) {
        display.print(gps_longitud, 4);
    } else {
        display.print("--");
    }

    display.display();
}

// --------------------------------------------------
// Mostrar datos en monitor serial
// --------------------------------------------------

void mostrarEnSerial() {
    Serial.println("-------------------------");

    Serial.print("BMP280: ");
    Serial.println(bmpOK ? "OK" : "NO DETECTADO");

    Serial.print("Temperatura calibrada: ");
    Serial.print(temperaturaCalibrada);
    Serial.println(" C");

    Serial.print("Presion: ");
    Serial.print(presion_hPa);
    Serial.println(" hPa");

    Serial.print("Altitud barometrica: ");
    Serial.print(altitud_m);
    Serial.println(" m");

    Serial.print("MPU6050: ");
    Serial.println(mpuOK ? "OK" : "NO DETECTADO");

    Serial.print("Aceleracion total: ");
    Serial.print(aceleracion_g);
    Serial.println(" G");

    Serial.print("Giro X: ");
    Serial.print(gx_dps);
    Serial.println(" deg/s");

    Serial.print("Giro Y: ");
    Serial.print(gy_dps);
    Serial.println(" deg/s");

    Serial.print("Giro Z: ");
    Serial.print(gz_dps);
    Serial.println(" deg/s");

    Serial.println("----- GPS -----");

    Serial.print("Caracteres recibidos: ");
    Serial.println(gps_chars);

    Serial.print("Checksums fallidos: ");
    Serial.println(gps_checksums_fallidos);

    Serial.print("GPS recibe datos: ");
    Serial.println(gpsConDatos ? "SI" : "NO");

    Serial.print("GPS fix valido: ");
    Serial.println(gpsFixValido ? "SI" : "NO");

    Serial.print("Satelites: ");
    Serial.println(gps_satelites);

    if (gpsFixValido) {
        Serial.print("Latitud: ");
        Serial.println(gps_latitud, 6);

        Serial.print("Longitud: ");
        Serial.println(gps_longitud, 6);
    } else {
        Serial.println("Ubicacion: No valida todavia");
    }

    Serial.print("Altitud GPS: ");
    Serial.print(gps_altitud_m);
    Serial.println(" m");

    Serial.print("Velocidad GPS: ");
    Serial.print(gps_velocidad_kmph);
    Serial.println(" km/h");

    Serial.print("Hora UTC: ");
    Serial.println(gps_hora_utc);

    Serial.print("Fecha UTC: ");
    Serial.println(gps_fecha_utc);

    if (millis() > 5000 && gps_chars < 10) {
        Serial.println();
        Serial.println("ERROR: No se reciben datos del GPS.");
        Serial.println("Revise GPS TX -> GPIO16, GND comun y baudrate 9600.");
    }
}

// --------------------------------------------------
// Enviar datos por WebSocket
// --------------------------------------------------

void enviarDatosWebSocket() {
    StaticJsonDocument<768> doc;

    // Datos BMP280
    doc["temp"] = temperaturaCalibrada;
    doc["temp_real"] = temperatura;
    doc["baro"] = presion_hPa;
    doc["alt"] = altitud_m;
    doc["bmp_ok"] = bmpOK;

    // Datos MPU6050
    doc["acel"] = aceleracion_g;

    doc["ax"] = ax_ms2;
    doc["ay"] = ay_ms2;
    doc["az"] = az_ms2;

    doc["gx"] = gx_dps;
    doc["gy"] = gy_dps;
    doc["gz"] = gz_dps;

    doc["mpu_ok"] = mpuOK;

    // Datos GPS
    doc["gps_ok"] = gpsConDatos;
    doc["gps_fix"] = gpsFixValido;
    doc["gps_lat"] = gps_latitud;
    doc["gps_lon"] = gps_longitud;
    doc["gps_alt"] = gps_altitud_m;
    doc["gps_speed"] = gps_velocidad_kmph;
    doc["gps_sat"] = gps_satelites;
    doc["gps_chars"] = gps_chars;
    doc["gps_time"] = gps_hora_utc;
    doc["gps_date"] = gps_fecha_utc;

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);
}

// --------------------------------------------------
// Inicializar servidor web
// --------------------------------------------------

void inicializarServidorWeb() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Error montando SPIFFS.");
        return;
    }

    WiFi.softAP(local_ssid, local_pass);

    IPAddress IP = WiFi.softAPIP();

    Serial.println();
    Serial.println("Servidor creado correctamente.");
    Serial.print("Conectate a la red: ");
    Serial.println(local_ssid);
    Serial.print("Contrasena: ");
    Serial.println(local_pass);
    Serial.print("IP del servidor: ");
    Serial.println(IP);

    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.serveStatic("/", SPIFFS, "/");

    server.begin();

    Serial.println("Servidor web iniciado.");
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("Iniciando sistema de telemetria ESP32...");

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    inicializarOLED();

    mostrarMensajeOLED(
        "Iniciando...",
        "Buscando sensores",
        "Espere..."
    );

    inicializarBMP280();
    inicializarMPU6050();
    inicializarGPS();

    inicializarServidorWeb();

    Serial.println("Sistema listo.");
}

// --------------------------------------------------
// Loop principal
// --------------------------------------------------

void loop() {
    ws.cleanupClients();

    // El GPS debe procesarse lo mas seguido posible.
    procesarGPS();

    unsigned long ahora = millis();

    if (ahora - lastSensorTime >= SENSOR_INTERVAL_MS) {
        lastSensorTime = ahora;
        leerSensores();
    }

    if (ahora - lastWebSocketTime >= WEBSOCKET_INTERVAL_MS) {
        lastWebSocketTime = ahora;
        enviarDatosWebSocket();
    }

    if (ahora - lastOLEDTime >= OLED_INTERVAL_MS) {
        lastOLEDTime = ahora;
        mostrarEnOLED();
    }

    if (ahora - lastSerialTime >= SERIAL_INTERVAL_MS) {
        lastSerialTime = ahora;
        mostrarEnSerial();
    }
}
