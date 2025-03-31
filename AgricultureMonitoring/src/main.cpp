#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "bsec.h"
#include "config.h"  // Inclure le fichier de configuration

// Configuration des pins I2C
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Création d'une instance BSEC
Bsec iaqSensor;

void setup() {
  // Initialisation du port série
  Serial.begin(115200);
  delay(1000);
  Serial.println("Système de monitoring agricole");
  
  // Initialisation du bus I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
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
  }
  
  // Délai pour éviter trop de lectures
  delay(1000);
}