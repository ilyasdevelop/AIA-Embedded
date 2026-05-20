#include "BluetoothSerial.h";
#include <string>;
BluetoothSerial SerialBT;
/*


*/

// definition des ports         
#define motorRD 14  
#define motorFD 32      
#define motorRG 15    
#define motorFG 33    

void setup() {

  // SETUP du bluetooth 

  Serial.begin(9600);
  SerialBT.begin("AIA");
  Serial.println("The device ESP32 started, now you can pair it with bluetooth!");

  pinMode(motorRD, OUTPUT); 
  pinMode(motorFD, OUTPUT);  
  pinMode(motorRG, OUTPUT); 
  pinMode(motorFG, OUTPUT); 
  
}

void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    String cmde = SerialBT.readStringUntil('\n');
    cmde.trim(); 
  }
  String cmde = SerialBT.readStringUntil('\n');
  int choix;
  choix = cmde.toInt();
  if (choix == 0){
    stop();
  }
  if (choix == 1){
    forward();
  }
  if (choix == 2){
    backward();
  }
  if (choix == 3){
    left();
  }
  if (choix == 4){
    right();
  }
}

// fonction avance
void forward(){
  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, HIGH);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, LOW);
}
// fonction recule 
void backward(){
  digitalWrite(motorRD, HIGH);
  digitalWrite(motorRG, HIGH);
  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, LOW);
}
// fonction tourne a droite 
void right(){
  delay(5000);
  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, HIGH);
  digitalWrite(motorRD, HIGH);
  digitalWrite(motorRG, LOW);
  delay(5000);
}
// fonction tourne à gauche
void left(){
  digitalWrite(motorFD, HIGH);
  digitalWrite(motorFG, LOW);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, HIGH);
}
void stop(){
  digitalWrite(motorFD, LOW);
  digitalWrite(motorFG, LOW);
  digitalWrite(motorRD, LOW);
  digitalWrite(motorRG, LOW);
}




