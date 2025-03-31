#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "bsec.h"
#include "config.h"  // Inclure le fichier de configuration
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuration des pins I2C
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Création d'une instance BSEC
Bsec iaqSensor;

// Configuration de l'écran OLED
#define SCREEN_WIDTH 128     // Largeur de l'OLED en pixels
#define SCREEN_HEIGHT 64     // Hauteur de l'OLED en pixels
#define OLED_RESET -1        // Reset pin (ou -1 si partage de la ligne de reset Arduino)
#define SCREEN_ADDRESS 0x3C  // Adresse I2C typique pour l'écran SSD1306 (0x3C ou 0x3D)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  // Initialisation du port série
  Serial.begin(115200);
  delay(1000);
  Serial.println("Système de monitoring agricole");
  
  // Initialisation du bus I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialisation de l'écran OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Ne pas continuer si l'écran ne répond pas
  }
  
  // Affichage initial
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initialisation..."));
  display.display();
  
  // Connexion au WiFi
  Serial.print("Connexion au WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnecté au WiFi");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  
  // Initialisation du capteur BME680 avec BSEC
  iaqSensor.begin(0x77, Wire);
  Serial.println("BSEC version " + 
    String(iaqSensor.version.major) + "." +
    String(iaqSensor.version.minor) + "." +
    String(iaqSensor.version.major_bugfix) + "." +
    String(iaqSensor.version.minor_bugfix));

  // Configuration des capteurs virtuels à utiliser
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
  };
  
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  
  Serial.println("BSEC et BME680 initialisés avec succès");
}

void loop() {
  unsigned long time_trigger = millis();
  
  // Lecture des données du capteur à intervalles réguliers
  if (iaqSensor.run()) { // Si de nouvelles données sont disponibles
    Serial.println("---------------------------------------------------");
    Serial.println("Données du capteur BME680 - " + String(time_trigger/1000) + "s");
    
    // Affichage dans le moniteur série
    Serial.print("Température: ");
    Serial.print(iaqSensor.temperature);
    Serial.println(" °C");
    
    Serial.print("Humidité: ");
    Serial.print(iaqSensor.humidity);
    Serial.println(" %");
    
    Serial.print("Pression: ");
    Serial.print(iaqSensor.pressure / 100.0);
    Serial.println(" hPa");
    
    Serial.print("IAQ (Qualité de l'air): ");
    Serial.print(iaqSensor.iaq);
    Serial.print(" (Précision: ");
    Serial.print(iaqSensor.iaqAccuracy);
    Serial.println(")");
    
    Serial.print("CO2 équivalent: ");
    Serial.print(iaqSensor.co2Equivalent);
    Serial.println(" ppm");
    
    Serial.print("COV équivalent: ");
    Serial.print(iaqSensor.breathVocEquivalent);
    Serial.println(" ppm");
    
    Serial.print("Résistance du gaz: ");
    Serial.print(iaqSensor.gasResistance / 1000.0);
    Serial.println(" KOhms");
    
    Serial.print("Statut WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connecté");
    } else {
      Serial.println("Déconnecté");
    }
    
    // Mise à jour de l'affichage OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Titre
    display.setCursor(0, 0);
    display.println(F("Monitoring Agricole"));
    display.drawLine(0, 9, display.width(), 9, SSD1306_WHITE);
    
    // Température et humidité
    display.setCursor(0, 12);
    display.print(F("Temp: "));
    display.print(iaqSensor.temperature, 1);
    display.println(F(" C"));
    
    display.setCursor(0, 22);
    display.print(F("Hum: "));
    display.print(iaqSensor.humidity, 1);
    display.println(F(" %"));
    
    // Pression
    display.setCursor(0, 32);
    display.print(F("Pres: "));
    display.print(iaqSensor.pressure / 100.0, 0);
    display.println(F(" hPa"));
    
    // IAQ
    display.setCursor(0, 42);
    display.print(F("IAQ: "));
    display.print(iaqSensor.iaq, 0);
    display.print(F(" ("));
    display.print(iaqSensor.iaqAccuracy);
    display.println(F(")"));
    
    // WiFi et CO2
    display.setCursor(0, 52);
    display.print(F("CO2: "));
    display.print(iaqSensor.co2Equivalent, 0);
    display.print(F(" WiFi: "));
    if (WiFi.status() == WL_CONNECTED) {
      display.println(F("OK"));
    } else {
      display.println(F("NOK"));
    }
    
    display.display();
  }
  
  // Délai pour éviter trop de lectures
  delay(1000);
}