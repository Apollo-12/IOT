# Système de Monitoring Agricole IoT

Un système IoT pour l'agriculture connectée basé sur l'ESP32 et le capteur environnemental BME680. Ce dispositif permet la surveillance en temps réel des conditions critiques dans une serre ou un environnement agricole.

![Status](https://img.shields.io/badge/Statut-Opérationnel-brightgreen)

## Table des matières

- [Caractéristiques](#caractéristiques)
- [Composants matériels](#composants-matériels)
- [Installation](#installation)
- [Configuration](#configuration)
- [Utilisation](#utilisation)
- [Architecture du système](#architecture-du-système)
- [Dashboard Blynk](#dashboard-blynk)
- [Données mesurées](#données-mesurées)
- [Dépannage](#dépannage)

## Caractéristiques

- **Mesure précise** de la température, humidité, pression atmosphérique et qualité de l'air (COV)
- **Affichage local** des données sur écran OLED
- **Intégration de données météorologiques** externes via l'API OpenWeather
- **Dashboard complet** accessible à distance via la plateforme Blynk
- **Comparaison** en temps réel des conditions intérieures/extérieures

## Composants matériels

- ESP32 (Dev Module)
- Capteur BME680 (mesure température, humidité, pression et composés organiques volatils)
- Écran OLED SSD1306 128x64 pixels
- Câbles Dupont pour les connexions

## Installation

### Branchements

| Composant | Pin ESP32 |
|-----------|-----------|
| BME680 VCC | 3.3V |
| BME680 GND | GND |
| BME680 SCL | GPIO22 |
| BME680 SDA | GPIO21 |
| OLED VCC | 3.3V |
| OLED GND | GND |
| OLED SCL | GPIO22 |
| OLED SDA | GPIO21 |

### Logiciel

1. Récupérez le code
   - **Option A - Pour l'utiliser :** Clonez ce dépôt
     ```
     git clone https://github.com/Apollo-12/IOT.git
     cd IOT
     ```
   - **Option B - Pour contribuer :** Fork le dépôt sur GitHub puis clonez votre fork
     ```
     git clone https://github.com/votre-username/IOT.git
     cd IOT
     git remote add upstream https://github.com/Apollo-12/IOT.git
     ```

2. Ouvrez le projet dans PlatformIO (Visual Studio Code)

3. Créez un fichier `include/config.h` basé sur l'exemple fourni (`include/config.h.example`)
   ```cpp
   // Configuration Blynk
   #define BLYNK_TEMPLATE_ID "votre_template_id"
   #define BLYNK_TEMPLATE_NAME "votre_template_name"
   #define BLYNK_AUTH_TOKEN "votre_auth_token"

   // Configuration WiFi
   const char* WIFI_SSID = "votre_ssid";
   const char* WIFI_PASSWORD = "votre_password";

   // Clés API
   const char* OPENWEATHER_API_KEY = "votre_cle_openweather";

   // Configuration OpenWeather
   const char* CITY = "Paris";
   const char* COUNTRY_CODE = "FR";
   ```

4. Installez les bibliothèques requises via PlatformIO
   - BSEC Software Library
   - Adafruit SSD1306
   - Blynk
   - ArduinoJson

5. Compilez et téléversez le code sur votre ESP32

## Configuration

### Obtenir une clé API OpenWeather

1. Créez un compte sur [OpenWeather](https://openweathermap.org/)
2. Générez une nouvelle clé API
3. Ajoutez la clé dans votre fichier `config.h`

### Configurer Blynk

1. Créez un compte sur [Blynk](https://blynk.io/)
2. Créez un nouveau template avec les datastreams suivants :

| Virtual Pin | Nom | Type | Unité |
|-------------|-----|------|-------|
| V0 | Temperature | Double | °C |
| V1 | Humidity | Double | % |
| V2 | Pressure | Double | hPa |
| V3 | Air Quality | Integer | IAQ |
| V4 | CO2 | Integer | ppm |
| V5 | VOC | Double | ppm |
| V6 | Ext Temp | Double | °C |
| V7 | Ext Humidity | Double | % |
| V8 | Weather | String | - |
| V9 | Ext Pressure | Double | hPa |

3. Créez un nouveau périphérique basé sur ce template
4. Notez le Template ID, Template Name et Auth Token pour les ajouter dans `config.h`
5. Créez un dashboard avec les widgets appropriés pour visualiser les données

## Utilisation

Une fois l'appareil configuré et alimenté :

1. L'ESP32 se connecte au WiFi et à Blynk
2. L'écran OLED affiche les principales mesures environnementales
3. Les données sont envoyées à Blynk toutes les 5 secondes
4. Les données météorologiques sont mises à jour toutes les 10 minutes
5. Le moniteur série affiche des informations détaillées et des comparaisons

## Architecture du système

Le système est composé de plusieurs modules :

- **Acquisition de données** : Lecture du capteur BME680 via I2C
- **Connexion sans fil** : WiFi pour la connexion à Internet
- **API externe** : Intégration avec OpenWeather pour les conditions météorologiques
- **Interface utilisateur locale** : Affichage OLED
- **Interface distante** : Dashboard Blynk

## Dashboard Blynk

Le dashboard Blynk comprend :

- Jauges pour la température, l'humidité et la pression intérieures
- Indicateurs de qualité de l'air (IAQ), de CO2 et de COV
- Jauges pour les conditions météorologiques extérieures
- Un graphique d'historique des principales mesures

## Données mesurées

- **Température** : °C, précision de ±0.5°C
- **Humidité** : %, précision de ±3%
- **Pression** : hPa, précision de ±1 hPa
- **IAQ (Indice de Qualité de l'Air)** : 0-500
  - 0-50 : Excellent
  - 51-100 : Bon
  - 101-150 : Légèrement pollué
  - 151-200 : Modérément pollué
  - 201-300 : Fortement pollué
  - 301+ : Extrêmement pollué
- **CO2 équivalent** : ppm (parties par million)
- **COV équivalent** : ppm (parties par million)

## Dépannage

| Problème | Solution possible |
|----------|-------------------|
| L'écran OLED n'affiche rien | Vérifiez les connexions I2C et l'adresse (0x3C ou 0x3D) |
| Capteur BME680 ne répond pas | Vérifiez les connexions I2C et l'adresse (0x76 ou 0x77) |
| Pas de connexion WiFi | Vérifiez les identifiants dans config.h |
| Pas de données OpenWeather | Vérifiez la clé API et la connexion Internet |
| Pas de connexion à Blynk | Vérifiez l'Auth Token et que le périphérique est activé dans Blynk |
| IAQ toujours à 25 et précision à 0 | Le capteur est en cours de calibration, attendez 24-48h |

---

## Contributeurs

- Nicolas Antoine
- Paul Kuzmak Bourdet
- Alexandre Vachez

---

## Licence

Ce projet est sous licence MIT.

---

Développé avec ❤️ pour l'agriculture connectée.
