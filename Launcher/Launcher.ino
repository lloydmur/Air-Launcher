#include "LiquidCrystal_PCF8574.h"
#include "Button.h"
#include "Servo.h"
#include "Relay.h"

#define JOYSTICK_1_UP  30
#define JOYSTICK_1_DOWN 31
#define JOYSTICK_2_UP 32
#define JOYSTICK_2_DOWN 33
#define LCD_PIN_RS  13
#define TOGGLE_SWITCH_PIN 35
#define POWER_LED_PIN 36
#define BIG_RED_BUTTON_PIN 39
#define BIG_RED_BUTTON_LED 38
#define DEADMAN_SWITCH_PIN 43
#define TRIGGER_PIN 45
#define ECHO_PIN_1 46
#define ECHO_PIN_2 47
#define ECHO_PIN_3 48
#define ECHO_PIN_4 49
#define SERVOMD_PIN_SIG  6
#define ALARM_PIN 6

#define VERTI_PIN 2
#define HORIZON_PIN 3
#define LJS_U 12
#define LJS_D 11
#define LJS_L 10
#define LJS_R 9

//lcd stuffs
#define LCD_ADDRESS 0x27
// Define LCD characteristics
#define LCD_ROWS 2
#define LCD_COLUMNS 16
#define SCROLL_DELAY 150
#define BACKLIGHT 255

//servo motor stuffs
const int servoRestPosition   = 20;  //Starting position
const int servoTargetPosition = 150; //Position when event is detected

Servo verti;
Servo horizon;
int hAngle = 0;
int vAngle = 0;

long duration1, duration2, duration3, duration4, inches1, inches2, inches3, inches4;
int state = 0;
int pressureCounter = 0;
int angleCounter = 0;
int transana = 0; //Store analog reading from A0
int pressure_output = 0; // Stores converted pressure reading.

Button joystick1_up(JOYSTICK_1_UP);
Button joystick1_down(JOYSTICK_1_DOWN);
Button joystick2_up(JOYSTICK_2_UP);
Button joystick2_down(JOYSTICK_2_DOWN);
Button toggleSwitch(TOGGLE_SWITCH_PIN);
Button bigRedButton(BIG_RED_BUTTON_PIN);
Button deadmanSwitch(DEADMAN_SWITCH_PIN);

LiquidCrystal_PCF8574 lcdI2C;

Relay BV(8, true);
Relay EV(4, true);
Relay FV(7, true);


void setup() {


  Serial.begin(9600);
  //while (!Serial);

  pinMode(JOYSTICK_1_UP, INPUT_PULLUP);
  pinMode(JOYSTICK_1_DOWN, INPUT_PULLUP);
  pinMode(JOYSTICK_2_UP, INPUT_PULLUP);
  pinMode(JOYSTICK_2_DOWN, INPUT_PULLUP);

  pinMode(LJS_U, INPUT_PULLUP);
  pinMode(LJS_D, INPUT_PULLUP);
  pinMode(LJS_L, INPUT_PULLUP);
  pinMode(LJS_R, INPUT_PULLUP);

  pinMode(BIG_RED_BUTTON_LED, OUTPUT);
  EV.begin();
  FV.begin();
  BV.begin();

  EV.turnOff();
  FV.turnOff();
  BV.turnOff();

   pinMode(ALARM_PIN, OUTPUT);
   digitalWrite(ALARM_PIN, HIGH);
   verti.attach(VERTI_PIN);
   horizon.attach(HORIZON_PIN);
   reset_Angle();

  toggleSwitch.init();
  bigRedButton.init();
  deadmanSwitch.init();

  pressureCounter = 0;
  transana = 0; //Store analog reading from A0
  pressure_output = 20; // Stores converted pressure reading.

  lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  printLCD();

}

void loop() {
  if (toggleSwitch.read()) {
    lcdI2C.setBacklight(BACKLIGHT);
    lcdI2C.display();
    //lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
    //sensorCheck();
    //angleMATH();
    LJS_Handler();
    Serial_Switch();
    pressureMATH();
    //printLCD();
    delay(100);

    if (deadmanSwitch.read()) {
      digitalWrite(BIG_RED_BUTTON_LED, HIGH);
      Serial.println("DM");
      if (bigRedButton.read()) {
        Serial.println("red button");

        launching();
        digitalWrite(BIG_RED_BUTTON_LED, LOW);

      }
    }else{
      digitalWrite(BIG_RED_BUTTON_LED, LOW);

    }
  } else {
    //lcdI2C.clear();
    lcdI2C.setBacklight(0);
    lcdI2C.noDisplay();
    //digitalWrite(bigRedButton, LOW);
  }

}
void launching() {
  lcdI2C.clear();
  lcdI2C.print("3!");
  digitalWrite(ALARM_PIN, LOW);
  delay(900);
  digitalWrite(ALARM_PIN, HIGH);
  delay(100);
  lcdI2C.clear();
  lcdI2C.print("2!");
  digitalWrite(ALARM_PIN, LOW);
  delay(900);
  digitalWrite(ALARM_PIN, HIGH);
  delay(100);
  lcdI2C.clear();
  lcdI2C.print("1...");
  digitalWrite(ALARM_PIN, LOW);
  delay(900);
  digitalWrite(ALARM_PIN, HIGH);
  delay(100);

  lcdI2C.clear();
  lcdI2C.print("Pressurizing");

  BV.turnOn(); //close bleed valve
  delay(50);
  EV.turnOn(); //open enterance valve to pressure up!
  Serial.println("Pressurizing!!");
  while (deadmanSwitch.read() && toggleSwitch.read() && pressure_output - 30 <= pressureCounter) {
    if(!deadmanSwitch.read()){
        BV.turnOff();
        delay(2000);
    }

    readTransducer();

    delay(100);
  }


  EV.turnOff();
  delay(50);
  lcdI2C.clear();
  lcdI2C.print("Fire!");
  //  digitalWrite(BIG_RED_BUTTON_LED,HIGH);
  // lcdI2C.noBlink();
  Serial.println("firing");
  FV.turnOn();

  delay(100);
  //done firing, reset

  //close firing valve and open bleed valve
  FV.turnOff();
  BV.turnOff();
  //reset pressure counters

  pressureCounter = 0;
  pressureCounter = 0;
  transana = 0; //Store analog reading from A0
  pressure_output = 0; // Stores converted pressure reading.
  printLCD();


}


/*void sensorCheck(){
  digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    duration1 = pulseIn(ECHO_PIN_1, HIGH);
    duration2 = pulseIn(ECHO_PIN_2, HIGH);
    duration3 = pulseIn(ECHO_PIN_3, HIGH);
    duration4 = pulseIn(ECHO_PIN_4, HIGH);

    inches1 = microsecondsToInches(duration1);
    inches2 = microsecondsToInches(duration2);
    inches3 = microsecondsToInches(duration3);
    inches4 = microsecondsToInches(duration4);
    //If nobody is too close
    if(inches1 > 20 || inches2 > 20 || inches3 > 20 || inches4 > 20){

      state = 0;

    }
  }*/

void readTransducer() {
  transana = analogRead(A0);
  pressure_output = map(transana, 100, 1023, 0, 174);
  //at 0 PSI the transducer outputs an analog value of about 100
  //Analog value reads up to 1023 at 5V
  //Our transducer measures up to 174PSI
  Serial.print(transana);
  Serial.print(" => ");
  Serial.println(pressure_output);
}

/*void bigButtonLED(){

  if(bigRedButton.read()){
        digitalWrite(BIG_RED_BUTTON_LED,HIGH);
      }
      else{
        digitalWrite(BIG_RED_BUTTON_LED,LOW);
      }

  }*/
  /*
void setAngle() {
  servo.attach(SERVOMD_PIN_SIG);
  servo.write(angleCounter);
  delay(100);
  servo.detach();
}
*/
void printLCD() {
  //update counters before we print to LCD screen
  //  pressureCounter = counterP(pressureCounter);
  // angleCounter = counterA(angleCounter);
  lcdI2C.clear();     // Clear LCD screen.
  lcdI2C.print("Pressure:");
  lcdI2C.print(pressureCounter);// Print print String to LCD on first line
  lcdI2C.selectLine(2);                    // Set cursor at the begining of line 2
  lcdI2C.print("Angle: H:");
  lcdI2C.print(hAngle/2);
  lcdI2C.print(" V:");
  lcdI2C.print(vAngle);

  // Print print String to LCD on second line
  delay(10);
}

/*int walkState(){
  if(state == 0){
    return 1;
  }
  else if(state == 1){
    return 2;
  }
  else{
    return 0;
  }
  }*/
void angleMATH() {
  if (!digitalRead(JOYSTICK_1_DOWN)) {

    if (angleCounter <= 1) {
      angleCounter = 0;
      Serial.println(angleCounter);

      printLCD();
    }
    else {
      angleCounter--;
      Serial.println(angleCounter);
      printLCD();

    }
  }
  else if (!digitalRead(JOYSTICK_1_UP)) {
    //Serial.println("JOY1 Up");
    if (angleCounter >= 89) {
      angleCounter = 90;
      Serial.println(angleCounter);

      printLCD();
    }
    angleCounter++;
    Serial.println(angleCounter);

    printLCD();
  }
}


void pressureMATH() {
  if (!digitalRead(JOYSTICK_2_DOWN)) {
    //Serial.println("JOY1 Down");
    if (pressureCounter <= 1) {
      pressureCounter = 0;
      Serial.println(pressureCounter);

      printLCD();
    }
    else {
      pressureCounter--;
      Serial.println(pressureCounter);

      printLCD();
    }


  }
  else if (!digitalRead(JOYSTICK_2_UP)) {
    //Serial.println("JOY1 Up");
    if (pressureCounter >= 19) {
      pressureCounter = 20;
      Serial.println(pressureCounter);

      printLCD();
    }
    else {
      pressureCounter++;
      Serial.println(pressureCounter);
      printLCD();
    }
  }
}
/*long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
  }*/
void Serial_Switch(){
  if(Serial.available() > 0){
    int selection = 0;
    selection = Serial.parseInt();
    if(selection == 1){
      if(EV.getState() == false){
        EV.turnOn();
        Serial.println("Entry Valve is now open.");
      }
      else{
        EV.turnOff();
        Serial.println("Entry Valve is now closed.");
      }
    }
    if(selection == 2){
      if(FV.getState() == false){
        FV.turnOn();
        Serial.println("Firing Valve is now open.");

      }
      else{
        FV.turnOff();
        Serial.println("Firing Valve is now closed.");
      }
    }
    if(selection == 3){
      if(BV.getState() == false){
        BV.turnOn();
        Serial.println("Bleed Valve is now closed.");
      }
      else{
        BV.turnOff();
        Serial.println("Bleed Valve is now open.");
      }
    }
    if(selection == 5){
      if(vAngle > 0){
        digitalWrite(6, LOW);
        delay(10);
        digitalWrite(6, HIGH);
        vAngle -= 10;
        double us = 2000 - (vAngle * 11.11);
        verti.writeMicroseconds(us);
        delay(50);
      }
    }
    if(selection == 8){
      if(vAngle < 70){
        digitalWrite(6, LOW);
        delay(10);
        digitalWrite(6, HIGH);
        vAngle += 10;
        double us = 2000 - (vAngle * 11.11);
        verti.writeMicroseconds(us);
        delay(50);
      }
    }
    if(selection == 4){
      digitalWrite(6, LOW);
      delay(10);
      digitalWrite(6, HIGH);
      int newAngle = hAngle - 10;
      horizon_Sweep(newAngle);
    }
    if(selection == 6){
      digitalWrite(6, LOW);
      delay(10);
      digitalWrite(6, HIGH);
      int newAngle = hAngle + 10;
      horizon_Sweep(newAngle);

    }
    if(selection == 7){
      digitalWrite(6, LOW);
      delay(10);
      digitalWrite(6, HIGH);
    }
  }
}
void reset_Angle(){
  verti.writeMicroseconds(2000);
  horizon_Sweep(180);
  horizon_Sweep(90);
  delay(1000);
}
void horizon_Sweep(int target){
  if(target < 0) target = 0;
  if(target > 180) target = 180;
  if(hAngle < target){
    for(hAngle; hAngle < target; hAngle ++){
      horizon.write(hAngle);
      delay(15);
    }
  }
  if(hAngle > target){
    for(hAngle; hAngle > target; hAngle --){
      horizon.write(hAngle);
      delay(15);
    }
  }



}
void LJS_Handler(){
  if(digitalRead(LJS_U) == 0){
    Serial.println("vAngle up");
    if(vAngle < 70){
        digitalWrite(ALARM_PIN, LOW);
        delay(10);
        digitalWrite(ALARM_PIN, HIGH);
        vAngle += 10;
        double us = 2000 - (vAngle * 11.11);
        verti.writeMicroseconds(us);
        printLCD();
        delay(250);
      }
  }
  if(digitalRead(LJS_D) == 0){
    Serial.println("vAngle down");
    if(vAngle > 0){
        digitalWrite(ALARM_PIN, LOW);
        delay(10);
        digitalWrite(ALARM_PIN, HIGH);
        vAngle -= 10;
        double us = 2000 - (vAngle * 11.11);
        verti.writeMicroseconds(us);
        printLCD();
        delay(250);
      }
  }
  if(digitalRead(LJS_L) == 0){
    Serial.println("hAngle left");
    digitalWrite(ALARM_PIN, LOW);
    delay(10);
    digitalWrite(ALARM_PIN, HIGH);
    int newAngle = hAngle - 5;
    horizon_Sweep(newAngle);
    printLCD();
  }
  if(digitalRead(LJS_R) == 0){
    Serial.println("hAngle left");
    digitalWrite(ALARM_PIN, LOW);
    delay(10);
    digitalWrite(ALARM_PIN, HIGH);
    int newAngle = hAngle + 5;
    horizon_Sweep(newAngle);
    printLCD();
  }

}
