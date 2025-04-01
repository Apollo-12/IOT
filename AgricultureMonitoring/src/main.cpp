#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"  // Inclure le fichier de configuration
#include <BlynkSimpleEsp32.h>
#include "bsec.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>  // Ajout de la bibliothèque pour le servomoteur

// Configuration des pins I2C
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Configuration du servomoteur
Servo valve;          // Création de l'objet servo
const int SERVO_PIN = 13;  // Pin pour le contrôle du servo
bool valveOpen = false;    // État de la valve (fermée par défaut)

// Création d'une instance BSEC
Bsec iaqSensor;

// Configuration de l'écran OLED
#define SCREEN_WIDTH 128     // Largeur de l'OLED en pixels
#define SCREEN_HEIGHT 64     // Hauteur de l'OLED en pixels
#define OLED_RESET -1        // Reset pin (ou -1 si partage de la ligne de reset Arduino)
#define SCREEN_ADDRESS 0x3C  // Adresse I2C typique pour l'écran SSD1306 (0x3C ou 0x3D)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables pour stocker les données météo
float weatherTemp = 0;
float weatherHumidity = 0;
float weatherPressure = 0;
String weatherDescription = "";
String weatherIcon = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 600000; // 10 minutes en millisecondes

// Variables pour Blynk
BlynkTimer timer;
unsigned long lastBlynkUpdate = 0;
const unsigned long blynkUpdateInterval = 5000; // 5 secondes en millisecondes

// Déclaration préalable des fonctions
void updateWeatherData();
void sendDataToBlynk();

// Fonction pour contrôler le servomoteur depuis Blynk
BLYNK_WRITE(V10) {  // V10 est la pin virtuelle pour contrôler la valve
  int value = param.asInt();
  if (value == 1) {
    valve.write(180);  // Ouvrir la valve (position 180°)
    valveOpen = true;
    Serial.println("Valve ouverte");
  } else {
    valve.write(0);    // Fermer la valve (position 0°)
    valveOpen = false;
    Serial.println("Valve fermée");
  }
}

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
  
  // Connexion à Blynk
  Serial.println("Connexion à Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
      
  // Configuration du timer Blynk pour envoyer les données périodiquement
  timer.setInterval(blynkUpdateInterval, sendDataToBlynk);
  
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
  
  // Initialisation du servo
  ESP32PWM::allocateTimer(0);
  valve.setPeriodHertz(50);      // PWM à 50Hz standard pour les servos
  valve.attach(SERVO_PIN, 500, 2400);  // Ajustez min/max selon votre servo
  valve.write(0);                // Initialiser à la position fermée
  
  Serial.println("BSEC, BME680 et servomoteur initialisés avec succès");
  
  // Première mise à jour météo
  updateWeatherData();
}

// Fonction pour envoyer les données à Blynk
void sendDataToBlynk() {
  if (WiFi.status() == WL_CONNECTED) {
    // Envoi des données du capteur BME680
    Blynk.virtualWrite(V0, iaqSensor.temperature);       // Température
    Blynk.virtualWrite(V1, iaqSensor.humidity);          // Humidité
    Blynk.virtualWrite(V2, iaqSensor.pressure / 100.0);  // Pression
    Blynk.virtualWrite(V3, iaqSensor.iaq);               // IAQ
    Blynk.virtualWrite(V4, iaqSensor.co2Equivalent);     // CO2 équivalent
    Blynk.virtualWrite(V5, iaqSensor.breathVocEquivalent); // VOC équivalent
    
    // Envoi des données météo si disponibles
    if (weatherTemp != 0) {
      Blynk.virtualWrite(V6, weatherTemp);                 // Température extérieure
      Blynk.virtualWrite(V7, weatherHumidity);             // Humidité extérieure
      Blynk.virtualWrite(V8, weatherDescription);          // Description météo
      Blynk.virtualWrite(V9, weatherPressure);             // Pression extérieure
    }
    
    // Envoi de l'état de la valve
    Blynk.virtualWrite(V10, valveOpen ? 1 : 0);          // État de la valve
    
    Serial.println("Données envoyées à Blynk");
  }
}

// Fonction pour récupérer les données météo d'OpenWeather
void updateWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += CITY;
    url += ",";
    url += COUNTRY_CODE;
    url += "&units=metric&appid=";
    url += OPENWEATHER_API_KEY;
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      
      // Utiliser ArduinoJson pour analyser la réponse
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        weatherTemp = doc["main"]["temp"];
        weatherHumidity = doc["main"]["humidity"];
        weatherPressure = doc["main"]["pressure"];
        weatherDescription = doc["weather"][0]["description"].as<String>();
        weatherIcon = doc["weather"][0]["icon"].as<String>();
        
        Serial.println("---------------------------------------------------");
        Serial.println("Données météo externes récupérées");
        Serial.print("Température: ");
        Serial.print(weatherTemp);
        Serial.println(" °C");
        Serial.print("Humidité: ");
        Serial.print(weatherHumidity);
        Serial.println(" %");
        Serial.print("Pression: ");
        Serial.print(weatherPressure);
        Serial.println(" hPa");
        Serial.print("Description: ");
        Serial.println(weatherDescription);
      } else {
        Serial.print("Erreur de désérialisation JSON: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("Erreur HTTP: ");
      Serial.println(httpCode);
    }
    
    http.end();
    lastWeatherUpdate = millis();
  }
}

void loop() {
  unsigned long time_trigger = millis();
  
  // Exécution de Blynk et du timer
  Blynk.run();
  timer.run();
  
  // Mise à jour des données météo toutes les 10 minutes
  if (time_trigger - lastWeatherUpdate > weatherUpdateInterval) {
    updateWeatherData();
  }
  
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
    
    Serial.print("État de la valve: ");
    Serial.println(valveOpen ? "Ouverte" : "Fermée");
    
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
    
    // CO2
    display.setCursor(0, 52);
    display.print(F("CO2: "));
    display.print(iaqSensor.co2Equivalent, 0);
    display.print(F(" ppm"));
    
    // Si nous avons des données météo, afficher une comparaison dans le moniteur série uniquement
    if (weatherTemp != 0) {
      Serial.println("\n---------------------------------------------------");
      Serial.println("Comparaison intérieur/extérieur:");
      Serial.println("                Intérieur   Extérieur   Différence");
      Serial.print("Température:    ");
      Serial.print(iaqSensor.temperature, 1);
      Serial.print(" °C     ");
      Serial.print(weatherTemp, 1);
      Serial.print(" °C     ");
      Serial.print(iaqSensor.temperature - weatherTemp, 1);
      Serial.println(" °C");
      
      Serial.print("Humidité:       ");
      Serial.print(iaqSensor.humidity, 1);
      Serial.print(" %      ");
      Serial.print(weatherHumidity, 1);
      Serial.print(" %      ");
      Serial.print(iaqSensor.humidity - weatherHumidity, 1);
      Serial.println(" %");
      
      Serial.print("Pression:       ");
      Serial.print(iaqSensor.pressure / 100.0, 1);
      Serial.print(" hPa    ");
      Serial.print(weatherPressure, 1);
      Serial.print(" hPa    ");
      Serial.print((iaqSensor.pressure / 100.0) - weatherPressure, 1);
      Serial.println(" hPa");
      
      Serial.print("Météo extérieure: ");
      Serial.println(weatherDescription);
    }
    
    // Statut WiFi sur l'écran OLED
    display.setCursor(70, 52);
    display.print(F("WiFi: "));
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