#include "BluetoothSerial.h"
#include <string>
BluetoothSerial SerialBT;

/*
/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-

FP-8  sonar et balise


/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
*/

#define OBSTACLE 36
#define BALISE 5


/*
/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-

FP-8  CAPTEUR DETECTION COULEUR 


/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
*/

#include "Wire.h"
#define VEML6040_I2C_ADDRESS 0x10
#define COMMAND_CODE_CONF 0x00
#define COMMAND_CODE_RED 0x08
#define COMMAND_CODE_GREEN 0x09
#define COMMAND_CODE_BLUE 0x0A
#define COMMAND_CODE_WHITE 0x0B

#define VEML6040_IT_40MS 0x00
#define VEML6040_IT_80MS 0x10
#define VEML6040_IT_160MS 0x20
#define VEML6040_IT_320MS 0x30
#define VEML6040_IT_640MS 0x40
#define VEML6040_IT_1280MS 0x50

#define VEML6040_TRIG_DISABLE 0x00
#define VEML6040_TRIG_ENABLE 0x04

#define VEML6040_AF_AUTO 0x00
#define VEML6040_AF_FORCE 0x02

#define VEML6040_SD_ENABLE 0x00
#define VEML6040_SD_DISABLE 0x01


enum tETAT {
  CHERCHE_LA_BALISE,
  AVANCER,
  EVITER_OBSTACLE,
  ARRIVER
};


int obstacle;
int balise;

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
  //lastConfiguration = configuration;
}


/*
/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-

GESTION DU ROBOT

/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
*/



// definition des ports
#define motorRD 14
#define motorFD 32
#define motorRG 15
#define motorFG 33

int CHANNEL_PWM_D;
int CHANNEL_PWM_G;
const int FREQ = 1000;
const int RESOLUTION = 8;



void setup() {

  // SETUP CAPTEUR COULEUR

  Serial.flush();
  Serial.begin(9600);
  if (VEML6040begin() == true) Serial.println("OK");
  else Serial.println("KO");

  // En fonction de la luminosité sous le robot, il est possible de changer le temps d'intégration du capteur couleur en changeant
  // VEML6040_IT_640MS par la définition adaptée au besoin : VEML6040_IT_xxxMS (voir liste des temps d'intégration disponibles en
  // en début de programme).
  VEML6040setConfiguration(VEML6040_IT_640MS + VEML6040_AF_AUTO + VEML6040_SD_ENABLE);

  // SETUP du bluetooth

  Serial.begin(9600);
  SerialBT.begin("AIA");
  Serial.println("The device ESP32 started, now you can pair it with bluetooth!");

  // SETUP DES PINS

  pinMode(motorRD, OUTPUT);
  pinMode(motorFD, OUTPUT);
  pinMode(motorRG, OUTPUT);
  pinMode(motorFG, OUTPUT);

  pinMode(OBSTACLE, INPUT_PULLUP);
  pinMode(BALISE, INPUT_PULLUP);


  // SETUP DU PWM

  ledcSetup(CHANNEL_PWM_D, FREQ, RESOLUTION);
  ledcSetup(CHANNEL_PWM_G, FREQ, RESOLUTION);
  ledcAttachPin(motorRD, CHANNEL_PWM_D);
  ledcAttachPin(motorRG, CHANNEL_PWM_G);
}

/*
#define ETAT1
#define ETAT2
#define ETAT3
#define ETAT4
#define ETAT5
#define ETAT6*/

void delaiAvecSortieSurObstacle(long duree) {


}

void loop() {

  //CAPTEUR COULEUR
  Serial.print("R=");
  Serial.print(VEML6040read(COMMAND_CODE_RED));
  Serial.print(" G=");
  Serial.print(VEML6040read(COMMAND_CODE_GREEN));
  Serial.print(" B=");
  Serial.println(VEML6040read(COMMAND_CODE_BLUE));
  delay(1000);

 

  int etat = 0;

  int R = VEML6040read(COMMAND_CODE_RED);
  int G = VEML6040read(COMMAND_CODE_GREEN);
  int B = VEML6040read(COMMAND_CODE_BLUE);

  int color = 0;
  int Stdvelocity = 0;

  obstacle = digitalRead(OBSTACLE);
  Serial.println(obstacle);

  balise = digitalRead(BALISE);
  Serial.println(balise);

  switch (etat) {
    case CHERCHE_LA_BALISE: // CHERCHE LA BALISE 
      if (balise == 0) {
        // Un moteur dans chaque sens à vitesse moyenne
        right(255);
        if (balise == 1) {
          forward(0);
          etat = 1;
        }
        
      }
      break;
    case 1: // AVANCE  
      if (balise == 1 && obstacle == 0) {
        forward(0);
      }
      else {
        etat = 2;
      }
      break;
    case 2: // EVITE OBSTACLE
      if (obstacle == 1) {
        eviter_obstacle();
        etat = 3;
        
      }
      break;
    case 3: // ARRIVER 
      if (R > G && R > B and R > 50000) {
        stop();
      }
      break;
  }   
  

}

/*
  char commande;
  if(SerialBT.available()){
    commande = SerialBT.read();
    switch(commande){

      case'0':
      stop(); 
      break;  

      case'1':
      forward(0);
      break;

      case'2':
      left(255);
      delay(250);
      forward(0);
      break;

      case'3':
      right(255);
      delay(250);
      forward(0);
      break;

      case'4':
      backward(255);
      break;

      case'5':
      Stdvelocity += 50;
      break;
      
      case'6':
      Stdvelocity -= 50;
      break;

      case'7':
      Stdvelocity = 125;
      break;

    }
  }
  */







/*
/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-

  FONCTION DE MOUVEMENT 


/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
*/





// fonction avance
void forward(int VELOCITY) {
  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, HIGH);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, LOW);

  ledcWrite(CHANNEL_PWM_D, VELOCITY);
  ledcWrite(CHANNEL_PWM_G, VELOCITY);
}
// fonction recule
void backward(int VELOCITY) {
  digitalWrite(motorRD, HIGH);
  digitalWrite(motorRG, HIGH);
  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, LOW);

  ledcWrite(CHANNEL_PWM_D, VELOCITY);
  ledcWrite(CHANNEL_PWM_G, VELOCITY);
}
// fonction tourne a droite
void right(int VELOCITY) {

  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, HIGH);
  digitalWrite(motorRD, HIGH);
  digitalWrite(motorRG, LOW);

  ledcWrite(CHANNEL_PWM_D, VELOCITY);
  ledcWrite(CHANNEL_PWM_G, VELOCITY);
}
// fonction tourne à gauche
void left(int VELOCITY) {

  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, LOW);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, HIGH);

  ledcWrite(CHANNEL_PWM_D, VELOCITY);
  ledcWrite(CHANNEL_PWM_G, VELOCITY);
}
void stop() {
  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, LOW);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, LOW);

  ledcWrite(CHANNEL_PWM_D, 0);
  ledcWrite(CHANNEL_PWM_G, 0);
}

void eviter_obstacle(){
  right(255);
  delay(5000);
  forward(255);
  
}

/*
void strategie(tETAT etat) {

  int R = VEML6040read(COMMAND_CODE_RED);
  int G = VEML6040read(COMMAND_CODE_GREEN);
  int B = VEML6040read(COMMAND_CODE_BLUE);

  int color = 0;
  int Stdvelocity = 0;

  obstacle = digitalRead(OBSTACLE);
  Serial.println(obstacle);

  balise = digitalRead(BALISE);
  Serial.println(balise);

  switch (etat) {
    case CHERCHE_LA_BALISE:
      if (balise == 0) {
        right(255);
        if (balise == 1) {
          forward(0);
        }
      }
      break;
    case AVANCER:
      if (balise == 1 && obstacle == 0) {
        forward(0);
      }
      break;
    case EVITER_OBSTACLE:
      if (obstacle == 1) {
        stop();
        delay(500);
        forward(0);
      }
      break;
    case ARRIVER:
      if (R > G && R > B and R > 50000) {
        stop();
      }
      break;
  }

}
*/
//void search(){}
