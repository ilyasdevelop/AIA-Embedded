#include "BluetoothSerial.h"
#include <string>
#include "Wire.h"

BluetoothSerial SerialBT;

/* =========================================================
   FP-8 : CAPTEUR DE COULEUR VEML6040
========================================================= */
#define VEML6040_I2C_ADDRESS 0x10
#define COMMAND_CODE_CONF 0x00
#define COMMAND_CODE_RED 0x08
#define COMMAND_CODE_GREEN 0x09
#define COMMAND_CODE_BLUE 0x0A

#define VEML6040_IT_640MS 0x40
#define VEML6040_AF_AUTO 0x00
#define VEML6040_SD_ENABLE 0x00

/* =========================================================
   PINS ET VARIABLES
========================================================= */
#define OBSTACLE 36
#define BALISE 5

// Définition des ports moteurs
#define motorRD 14 // RIN Droit [cite: 98, 99]
#define motorFD 32 // FIN Droit [cite: 98, 99]
#define motorRG 15 // RIN Gauche
#define motorFG 33 // FIN Gauche

// Configuration PWM (On donne des numéros de canaux distincts !)
const int CHANNEL_PWM_D = 0;
const int CHANNEL_PWM_G = 1;
const int FREQ = 1000;
const int RESOLUTION = 8;

// La fameuse machine à états (avec des mots, c'est plus clair)
enum tETAT {
  INIT,
  CHERCHE_LA_BALISE,
  AVANCER,
  EVITER_OBSTACLE,
  ARRIVER
};

// --- VARIABLES GLOBALES IMPORTANTES ---
// L'état est déclaré ICI pour ne pas être réinitialisé à chaque tour de boucle !
tETAT etat = INIT; 

bool derniere_esquive_droite = true; // Notre mémoire pour retrouver la balise plus vite
unsigned long dernier_affichage = 0; // Pour remplacer le delay(1000)

/* =========================================================
   FONCTIONS CAPTEUR COULEUR
========================================================= */
bool VEML6040begin(void) {
  bool sensorExists = false;
  Wire.begin();
  Wire.beginTransmission(VEML6040_I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    sensorExists = true;
  }
  return sensorExists;
}

uint16_t VEML6040read(uint8_t commandCode) {
  uint16_t data = 0;
  Wire.beginTransmission(VEML6040_I2C_ADDRESS);
  Wire.write(commandCode);
  Wire.endTransmission(false);
  Wire.requestFrom(VEML6040_I2C_ADDRESS, 2);
  while (Wire.available()) {
    data = Wire.read();
    data |= Wire.read() << 8;
  }
  return data;
}

void VEML6040setConfiguration(uint8_t configuration) {
  Wire.beginTransmission(VEML6040_I2C_ADDRESS);
  Wire.write(COMMAND_CODE_CONF);
  Wire.write(configuration);
  Wire.write(0);
  Wire.endTransmission();
}

/* =========================================================
   FONCTIONS DE MOUVEMENT CORRIGÉES
========================================================= */
// Avec le driver BD6211F, si FIN est HIGH, un PWM de 0 sur RIN donne la vitesse max [cite: 274, 278, 285-288].
// On inverse mathématiquement la vélocité (255 - VELOCITY) pour que 255 soit vraiment le max !

void forward(int VELOCITY) {
  int pwm_reel = 255 - VELOCITY; // Inversion logique
  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, HIGH);
  ledcWrite(CHANNEL_PWM_D, pwm_reel);
  ledcWrite(CHANNEL_PWM_G, pwm_reel);
}

void stop() {
  // Freinage d'urgence (FIN et RIN à HIGH) [cite: 274, 278, 285-288]
  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, HIGH);
  ledcWrite(CHANNEL_PWM_D, 255);
  ledcWrite(CHANNEL_PWM_G, 255);
}

void right(int VELOCITY) {
  // Moteur gauche avance (FIN=1, RIN=PWM inversé), Moteur droit recule (FIN=0, RIN=PWM normal)
  digitalWrite(motorFG, HIGH);
  ledcWrite(CHANNEL_PWM_G, 255 - VELOCITY);
  
  digitalWrite(motorFD, LOW);
  ledcWrite(CHANNEL_PWM_D, VELOCITY); 
}

void left(int VELOCITY) {
  // Inverse de la droite
  digitalWrite(motorFG, LOW);
  ledcWrite(CHANNEL_PWM_G, VELOCITY);
  
  digitalWrite(motorFD, HIGH);
  ledcWrite(CHANNEL_PWM_D, 255 - VELOCITY);
}

/* =========================================================
   SETUP
========================================================= */
void setup() {
  Serial.begin(9600);
  
  if (VEML6040begin() == true) Serial.println("Capteur Couleur OK");
  else Serial.println("Capteur Couleur KO");

  VEML6040setConfiguration(VEML6040_IT_640MS + VEML6040_AF_AUTO + VEML6040_SD_ENABLE); [cite: 545, 546]

  SerialBT.begin("Robot_LEO");
  Serial.println("Bluetooth démarré !");

  pinMode(motorRD, OUTPUT);
  pinMode(motorFD, OUTPUT);
  pinMode(motorRG, OUTPUT);
  pinMode(motorFG, OUTPUT);
  pinMode(OBSTACLE, INPUT_PULLUP);
  pinMode(BALISE, INPUT_PULLUP);

  ledcSetup(CHANNEL_PWM_D, FREQ, RESOLUTION);
  ledcSetup(CHANNEL_PWM_G, FREQ, RESOLUTION);
  ledcAttachPin(motorRD, CHANNEL_PWM_D);
  ledcAttachPin(motorRG, CHANNEL_PWM_G);
  
  // On s'assure que le robot est à l'arrêt au démarrage
  stop();
}

/* =========================================================
   BOUCLE PRINCIPALE (STRATÉGIE)
========================================================= */
void loop() {
  // 1. Lecture non-bloquante des logs (remplace ton delay(1000))
  if (millis() - dernier_affichage > 1000) {
    Serial.print("R="); Serial.print(VEML6040read(COMMAND_CODE_RED));
    Serial.print(" G="); Serial.print(VEML6040read(COMMAND_CODE_GREEN));
    Serial.print(" B="); Serial.println(VEML6040read(COMMAND_CODE_BLUE));
    Serial.print("Etat: "); Serial.println(etat);
    dernier_affichage = millis();
  }

  // 2. Lecture des capteurs
  int obstacle = digitalRead(OBSTACLE);
  int balise = digitalRead(BALISE);
  
  int R = VEML6040read(COMMAND_CODE_RED);
  int G = VEML6040read(COMMAND_CODE_GREEN);
  int B = VEML6040read(COMMAND_CODE_BLUE);

  // 3. Priorité absolue : Vérification de la ligne d'arrivée
  // Ratio : Le rouge doit être nettement supérieur au vert + bleu combinés
  if (R > (G + B) * 1.5 && R > 1500) {
    etat = ARRIVER;
  }

  // 4. La Machine à états
  switch (etat) {
      
    case INIT:
      // Le robot attend un signal pour partir. 
      // Pour l'instant, on lance direct après 3 secondes pour tester.
      stop();
      if (millis() > 3000) { 
        etat = CHERCHE_LA_BALISE;
      }
      break;

    case CHERCHE_LA_BALISE:
      if (balise == 1) {
        etat = AVANCER;
      } else {
        // C'est ici qu'on utilise la "mémoire" d'esquive !
        if (derniere_esquive_droite) {
          left(180); // Si on a esquivé par la droite, la balise est à gauche
        } else {
          right(180);
        }
      }
      break;

    case AVANCER:
      if (obstacle == 1) {
        etat = EVITER_OBSTACLE;
      } else if (balise == 0) {
        etat = CHERCHE_LA_BALISE;
      } else {
        forward(200); // 200 sur 255, une bonne vitesse de croisière
      }
      break;

    case EVITER_OBSTACLE:
      // ÉTAPE 1 : Tourner tant que l'obstacle est vu
      while (digitalRead(OBSTACLE) == 1) {
        right(180); 
      }
      
      // ÉTAPE 2 : L'obstacle n'est plus vu, on dégage "l'épaule" (Marge de sécurité)
      right(180);
      delay(250); // Petit délai bloquant assumé pour la manœuvre
      
      // ÉTAPE 3 : On avance pour dépasser l'obstacle
      forward(200);
      delay(800); 
      
      // ÉTAPE 4 : Mémorisation et reprise
      derniere_esquive_droite = true; // On retient qu'on a contourné par la droite
      etat = CHERCHE_LA_BALISE;
      break;

    case ARRIVER:
      stop();
      // On reste bloqué ici, mission accomplie !
      break;
  }
}
