#include <Arduino.h>
#include <Wire.h>

#include <WiFi.h>
#include <esp_now.h>

#include <TinyGPS++.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <Adafruit_BMP280.h>

#include <U8G2lib.h>

// =====================================================
// ESP-NOW
// =====================================================

// MAC del receptor
uint8_t RECEIVER_MAC[] =
{
    0xEC, 0xE3, 0x34,
    0x7B, 0xEF, 0xF0
};

// =====================================================
// ESTRUCTURA DE DATOS
// =====================================================

typedef struct struct_message {

    float temperatura;
    float presion;
    float altitud;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    double latitud;
    double longitud;

    int satelites;

    unsigned long caracteres;

} struct_message;

struct_message message;

// =====================================================
// I2C ESP32
// =====================================================

#define SDA_PIN 21
#define SCL_PIN 22

// =====================================================
// GPS
// =====================================================

TinyGPSPlus gps;

HardwareSerial gpsSerial(2);

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

#define GPS_BAUD 9600

// =====================================================
// MPU6050
// =====================================================

Adafruit_MPU6050 mpu;

// =====================================================
// BMP280
// =====================================================

Adafruit_BMP280 bmp;

bool bmpOK = false;

#define SEA_LEVEL_HPA 1013.25

// DEJADO ORIGINAL
#define OFFSET_CALIBRACION 4.0

// =====================================================
// OLED SH1106
// =====================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE
);

// =====================================================
// TIEMPOS
// =====================================================

// ENVÍO CADA 500 ms
unsigned long sensorMillis = 0;

const unsigned long sensorInterval = 500;

// OLED CADA 4 SEGUNDOS
unsigned long oledMillis = 0;

const unsigned long oledInterval = 4000;

int pantallaActual = 0;

// =====================================================
// VARIABLES
// =====================================================

float ax, ay, az;

float gx, gy, gz;

float temperatura = 0;

float temperaturaCalibrada = 0;

float presion = 0;

float altitud = 0;

// =====================================================
// PEER INFO
// =====================================================

esp_now_peer_info_t peerInfo;

// =====================================================
// CALLBACK ENVÍO
// =====================================================

void onDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status
) {

    Serial.print("ESP-NOW: ");

    if (status == ESP_NOW_SEND_SUCCESS) {

        Serial.println("Enviado");

    } else {

        Serial.println("Error");
    }
}

// =====================================================
// MENSAJES OLED
// =====================================================

void mostrarMensaje(const char *mensaje) {

    u8g2.clearBuffer();

    u8g2.setFont(
        u8g2_font_6x10_tr
    );

    u8g2.drawStr(
        0,
        20,
        mensaje
    );

    u8g2.sendBuffer();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

    Serial.begin(115200);

    // =================================================
    // I2C
    // =================================================

    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    Wire.setClock(100000);

    // =================================================
    // OLED
    // =================================================

    u8g2.begin();

    mostrarMensaje(
        "Iniciando..."
    );

    // =================================================
    // WIFI + ESP-NOW
    // =================================================

    WiFi.mode(WIFI_STA);

    Serial.print(
        "MAC TX: "
    );

    Serial.println(
        WiFi.macAddress()
    );

    if (esp_now_init() != ESP_OK) {

        Serial.println(
            "Error ESP-NOW"
        );

        return;
    }

    esp_now_register_send_cb(
        onDataSent
    );

    memcpy(
        peerInfo.peer_addr,
        RECEIVER_MAC,
        6
    );

    peerInfo.channel = 0;

    peerInfo.encrypt = false;

    if (
        esp_now_add_peer(
            &peerInfo
        ) != ESP_OK
    ) {

        Serial.println(
            "Error peer"
        );

        return;
    }

    // =================================================
    // GPS
    // =================================================

    gpsSerial.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX_PIN,
        GPS_TX_PIN
    );

    // =================================================
    // MPU6050
    // =================================================

    if (!mpu.begin()) {

        mostrarMensaje(
            "Error MPU6050"
        );

    } else {

        mpu.setAccelerometerRange(
            MPU6050_RANGE_8_G
        );

        mpu.setGyroRange(
            MPU6050_RANGE_500_DEG
        );

        mpu.setFilterBandwidth(
            MPU6050_BAND_21_HZ
        );
    }

    // =================================================
    // BMP280
    // =================================================

    if (bmp.begin(0x76)) {

        bmpOK = true;

    } else if (bmp.begin(0x77)) {

        bmpOK = true;
    }

    mostrarMensaje(
        "Sistema listo"
    );
}

// =====================================================
// LOOP
// =====================================================

void loop() {

    // =================================================
    // LEER GPS
    // =================================================

    while (gpsSerial.available()) {

        gps.encode(
            gpsSerial.read()
        );
    }

    // =================================================
    // LECTURA + ENVÍO
    // =================================================

    if (
        millis() - sensorMillis
        >= sensorInterval
    ) {

        sensorMillis = millis();

        // =============================================
        // MPU6050
        // =============================================

        sensors_event_t a, g, t;

        mpu.getEvent(
            &a,
            &g,
            &t
        );

        // =================================================
        // CONFIGURACIÓN ORIGINAL CORRECTA
        // =================================================

        ax = a.acceleration.x;

        ay = a.acceleration.y;

        az = -a.acceleration.z;

        // ORIGINAL

        gx = g.gyro.z * 180.0 / PI;

        gy = g.gyro.y * 180.0 / PI;

        gz = g.gyro.x * 180.0 / PI;

        // =============================================
        // BMP280
        // =============================================

        if (bmpOK) {

            temperatura =
                bmp.readTemperature();

            temperaturaCalibrada =
                temperatura
                - OFFSET_CALIBRACION;

            presion =
                bmp.readPressure()
                / 100.0;

            altitud =
                bmp.readAltitude(
                    SEA_LEVEL_HPA
                );
        }

        // =============================================
        // GUARDAR DATOS
        // =============================================

        message.temperatura =
            temperaturaCalibrada;

        message.presion =
            presion;

        message.altitud =
            altitud;

        message.ax = ax;
        message.ay = ay;
        message.az = az;

        message.gx = gx;
        message.gy = gy;
        message.gz = gz;

        message.latitud =
            gps.location.lat();

        message.longitud =
            gps.location.lng();

        message.satelites =
            gps.satellites.value();

        message.caracteres =
            gps.charsProcessed();

        // =============================================
        // ENVIAR ESP-NOW
        // =============================================

        esp_err_t result = esp_now_send(
            RECEIVER_MAC,
            (uint8_t *) &message,
            sizeof(message)
        );

        if (result == ESP_OK) {

            Serial.println(
                "Datos enviados"
            );

        } else {

            Serial.println(
                "Error al enviar"
            );
        }
    }

    // =================================================
    // CAMBIO OLED
    // =================================================

    if (
        millis() - oledMillis
        >= oledInterval
    ) {

        oledMillis = millis();

        pantallaActual++;

        if (pantallaActual > 2) {

            pantallaActual = 0;
        }
    }

    // =================================================
    // OLED
    // =================================================

    char linea[32];

    u8g2.clearBuffer();

    u8g2.setFont(
        u8g2_font_6x10_tr
    );

    // =================================================
    // BMP280
    // =================================================

    if (pantallaActual == 0) {

        u8g2.drawStr(
            0,
            10,
            "BMP280"
        );

        snprintf(
            linea,
            sizeof(linea),
            "TEMP: %.2f C",
            temperaturaCalibrada
        );

        u8g2.drawStr(
            0,
            28,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "PRES: %.1f hPa",
            presion
        );

        u8g2.drawStr(
            0,
            42,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "ALT: %.1f m",
            altitud
        );

        u8g2.drawStr(
            0,
            56,
            linea
        );
    }

    // =================================================
    // MPU6050
    // =================================================

    else if (pantallaActual == 1) {

        u8g2.drawStr(
            0,
            10,
            "MPU6050"
        );

        // ORIENTACIÓN ORIGINAL

        u8g2.drawStr(
            0,
            20,
            "Aceleracion"
        );

        snprintf(
            linea,
            sizeof(linea),
            "X:%5.1f Y:%5.1f",
            az,
            ay
        );

        u8g2.drawStr(
            0,
            34,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "Z:%5.1f",
            ax
        );

        u8g2.drawStr(
            0,
            46,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "GX:%4.0f",
            gx
        );

        u8g2.drawStr(
            0,
            60,
            linea
        );
    }

    // =================================================
    // GPS
    // =================================================

    else {

        u8g2.drawStr(
            0,
            10,
            "GPS"
        );

        snprintf(
            linea,
            sizeof(linea),
            "SAT: %d",
            gps.satellites.value()
        );

        u8g2.drawStr(
            0,
            24,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "CH: %lu",
            gps.charsProcessed()
        );

        u8g2.drawStr(
            0,
            36,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "LAT %.4f",
            gps.location.lat()
        );

        u8g2.drawStr(
            0,
            50,
            linea
        );

        snprintf(
            linea,
            sizeof(linea),
            "LON %.4f",
            gps.location.lng()
        );

        u8g2.drawStr(
            0,
            62,
            linea
        );
    }

    u8g2.sendBuffer();
}
