#include <Arduino.h> 
#include <Wire.h> 
#include <Adafruit_MPU6050.h> 
#include <Adafruit_Sensor.h> 
#include <U8g2lib.h> 
 
// Pines I2C del ESP32 
#define SDA_PIN 21 
#define SCL_PIN 22 
 
// Actualización cada 2 segundos 
#define UPDATE_TIME 2000 
 
Adafruit_MPU6050 mpu; 
 
// OLED SH1106 128x64 usando Hardware I2C 
// Usa el bus Wire de la ESP32 
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2( 
  U8G2_R0, 
  U8X8_PIN_NONE 
); 
 
void mostrarMensaje(const char *mensaje) { 
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(0, 20, mensaje); 
  u8g2.sendBuffer(); 
} 
 
void setup() { 
  Serial.begin(115200); 
  delay(1000); 
 
  // Iniciar bus I2C en pines del ESP32 
  Wire.begin(SDA_PIN, SCL_PIN); 
 
  // Velocidad I2C estable 
  Wire.setClock(100000); 
 
  // Iniciar OLED 
  u8g2.begin(); 
  mostrarMensaje("Iniciando..."); 
  delay(1000); 
 
  // Iniciar MPU6050 
  if (!mpu.begin()) { 
    Serial.println("No se encontro el MPU6050"); 
    mostrarMensaje("Error MPU6050"); 
 
    while (true) { 
      delay(10); 
    } 
  } 
 
  Serial.println("MPU6050 encontrado correctamente"); 
 
  // Configuracion del acelerometro 
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); 
 
  // Configuracion del giroscopio 
  mpu.setGyroRange(MPU6050_RANGE_500_DEG); 
 
  // Filtro interno 
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); 
 
  mostrarMensaje("Sistema listo"); 
  delay(1000); 
} 
 
void loop() { 
  sensors_event_t aceleracion; 
  sensors_event_t giro; 
  sensors_event_t temperatura; 
 
  // Obtener las lecturas del acelerómetro y giroscopio 
  mpu.getEvent(&aceleracion, &giro, &temperatura); 
 
  // Invertir el valor de X y Z para que Z apunte hacia arriba y X hacia 
la derecha 
  float ax = aceleracion.acceleration.x;  // No cambiar X, debería ir 
hacia la derecha 
  float ay = aceleracion.acceleration.y; 
  float az = -aceleracion.acceleration.z;  // Invertir el valor de Z para 
que apunte hacia arriba 
 
  // Calcular el giro en grados por segundo 
  float gx = giro.gyro.z * 180.0 / PI; 
  float gy = giro.gyro.y * 180.0 / PI; 
  float gz = giro.gyro.x * 180.0 / PI; 
 
  // Mostrar en el monitor serial 
  Serial.println("----- MPU6050 -----"); 
 
  Serial.print("Aceleracion X: "); 
  Serial.print(az);  // Mostrar Z en X 
  Serial.println(" m/s^2"); 
 
  Serial.print("Aceleracion Y: "); 
  Serial.print(ay); 
  Serial.println(" m/s^2"); 
 
  Serial.print("Aceleracion Z: "); 
  Serial.print(ax);  // Mostrar X en Z 
  Serial.println(" m/s^2"); 
 
  Serial.print("Giro X: "); 
  Serial.print(gx); 
  Serial.println(" deg/s"); 
 
  Serial.print("Giro Y: "); 
  Serial.print(gy); 
  Serial.println(" deg/s"); 
 
  Serial.print("Giro Z: "); 
  Serial.print(gz); 
  Serial.println(" deg/s"); 
 
  Serial.println(); 
 
  // Mostrar en la OLED 
  char linea[32]; 
 
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
 
  u8g2.drawStr(0, 8, "MPU6050 + ESP32"); 
 
  u8g2.drawStr(0, 20, "Aceleracion m/s2"); 
 
  // Intercambiar X y Z en la visualización de la pantalla OLED 
  snprintf(linea, sizeof(linea), "X:%5.1f Y:%5.1f", az, ay);  // Z debe 
ser el valor de X 
  u8g2.drawStr(0, 31, linea); 
 
  snprintf(linea, sizeof(linea), "Z:%5.1f", ax);  // X debe ser el valor 
de Z 
  u8g2.drawStr(0, 42, linea); 
 
  u8g2.drawStr(0, 53, "Giro deg/s"); 
 
  // Intercambiar X y Z para los giros en la pantalla OLED 
  snprintf(linea, sizeof(linea), "X:%4.0f Y:%4.0f Z:%4.0f", gx, gy, 
gz);  // Intercambiado para giros 
  u8g2.drawStr(0, 64, linea); 
 
  u8g2.sendBuffer(); 
 
  // Actualizar cada 2 segundos> 
  delay(UPDATE_TIME); 
} 
