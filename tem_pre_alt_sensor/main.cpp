#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

// --------------------------------------------------
// Pines I2C ESP32 DEVKIT V1
// --------------------------------------------------

#define I2C_SDA 21
#define I2C_SCL 22

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

#define BMP280_ADDRESS 0x76

// Presion nivel del mar
#define SEA_LEVEL_HPA 1013.25

// Offset de calibracion de temperatura
#define OFFSET_CALIBRACION 4.0

// --------------------------------------------------

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

Adafruit_BMP280 bmp;

// --------------------------------------------------
// Variables
// --------------------------------------------------

float temperatura = 0.0;

float temperaturaCalibrada = 0.0;

float presion_hPa = 0.0;

float altitud_m = 0.0;

// --------------------------------------------------
// Mostrar mensajes en OLED
// --------------------------------------------------

void mostrarMensaje(
    String linea1,
    String linea2 = "",
    String linea3 = ""
) {

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
// Leer BMP280
// --------------------------------------------------

void leerBMP280() {

    // Temperatura real
    temperatura = bmp.readTemperature();

    // Temperatura calibrada
    temperaturaCalibrada =
        temperatura - OFFSET_CALIBRACION;

    // Presion
    presion_hPa =
        bmp.readPressure() / 100.0;

    // Altitud
    altitud_m =
        bmp.readAltitude(SEA_LEVEL_HPA);
}

// --------------------------------------------------
// Mostrar datos en OLED
// --------------------------------------------------

void mostrarEnOLED() {

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("ESP32 + BMP280");

    display.setCursor(0, 12);
    display.print("Temp real: ");
    display.print(temperatura, 1);
    display.println(" C");

    display.setCursor(0, 24);
    display.print("Temp calib: ");
    display.print(temperaturaCalibrada, 1);
    display.println(" C");

    display.setCursor(0, 36);
    display.print("Pres: ");
    display.print(presion_hPa, 1);
    display.println(" hPa");

    display.setCursor(0, 48);
    display.print("Alt: ");
    display.print(altitud_m, 1);
    display.println(" m");

    display.display();
}

// --------------------------------------------------
// Mostrar datos en monitor serial
// --------------------------------------------------

void mostrarEnSerial() {

    Serial.println("-------------------------");

    Serial.print("Temperatura real: ");
    Serial.print(temperatura);
    Serial.println(" C");

    Serial.print("Temperatura calibrada: ");
    Serial.print(temperaturaCalibrada);
    Serial.println(" C");

    Serial.print("Presion: ");
    Serial.print(presion_hPa);
    Serial.println(" hPa");

    Serial.print("Altitud aproximada: ");
    Serial.print(altitud_m);
    Serial.println(" m");
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

    Serial.begin(115200);

    delay(1000);

    // Inicializar I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    Serial.println(
        "Iniciando ESP32 + OLED + BMP280"
    );

    // Inicializar OLED
    if (!display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDRESS
        )) {

        Serial.println(
            "Error: OLED no detectada"
        );

        while (true) {
            delay(1000);
        }
    }

    mostrarMensaje(
        "Iniciando...",
        "OLED OK",
        "Buscando BMP280"
    );

    delay(1500);

    // Inicializar BMP280
    if (!bmp.begin(0x76)) {

        Serial.println(
            "No encontrado en 0x76,"
            " probando 0x77..."
        );

        if (!bmp.begin(0x77)) {

            Serial.println(
                "Error: BMP280 no detectado"
            );

            mostrarMensaje(
                "Error BMP280",
                "No detectado",
                "Revise cables"
            );

            while (true) {
                delay(1000);
            }

        } else {

            Serial.println(
                "BMP280 encontrado en 0x77"
            );
        }

    } else {

        Serial.println(
            "BMP280 encontrado en 0x76"
        );
    }

    mostrarMensaje(
        "BMP280 OK",
        "Midiendo...",
        "Todo correcto"
    );

    delay(1500);
}

// --------------------------------------------------
// Loop principal
// --------------------------------------------------

void loop() {

    leerBMP280();

    mostrarEnSerial();

    mostrarEnOLED();

    // Actualizacion cada 2 segundos
    delay(2000);
}
