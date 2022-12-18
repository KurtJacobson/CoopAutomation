#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);


// ************************************ Define Pins *************************************

// Motor control outputs
const int motorSpeedPin = 10;
const int motorDownPin = 11;
const int motorUpPin = 12;

// Limit switch inputs
const int switchPin = 7;

// Photocell input
const int photocellPin = A0;


// ************************************* Variables *************************************

const long openTimeout = 20;                 // timeout for ramp to open in s
const long closeTimeout = 20;                // timeout for ramp to close in s

const long debounceDelay = 50;              // limit switch debounce time in ms

const long photocellReadInterval = 1;       // time in s between light level readings
unsigned long lastPhotocellReadTime = 0;    // time of last light level reading
int photocellValue = 0;                     // last light level reading
float avgPhotocellValue = 0;                // rolling avg light level

bool rampUpError = false;
bool rampDownError = false;

int rampClosed = -1;                        // current ramp state


void setup() {

  // init LCD
  lcd.init();
  lcd.backlight();

  // Motor control outputs
  pinMode(motorUpPin, OUTPUT);
  pinMode(motorDownPin, OUTPUT);
  pinMode(motorSpeedPin, OUTPUT);

  // Limit switch inputs
  pinMode(switchPin, INPUT_PULLUP);

  Serial.begin(9600); // set serial port for communication
}

void loop() {
  readLightLevel();
  delay(photocellReadInterval * 1000);

  lcd.setCursor(0,2);
  lcd.print("SW State:");
  lcd.print(!digitalRead(switchPin));
     
}


// ********************************* Photocell Reading **********************************

void readLightLevel() {

  const int ncycles = 10;         // number of read cycles before moving ramp
  static int ccycle = 0;          // current cycle

  // Read light level
  photocellValue = analogRead(photocellPin);
  Serial.print("Light value: ");
  Serial.print(photocellValue);

  // Light threshholds, qualitatively determined
  if (photocellValue < 10) {
    Serial.println(" - Dark");
  } else if (photocellValue < 200) {
    Serial.println(" - Dim");
  } else if (photocellValue < 500) {
    Serial.println(" - Light");
  } else if (photocellValue < 800) {
    Serial.println(" - Bright");
  } else {
    Serial.println(" - Very bright");
  }

  avgPhotocellValue = movingAverage(photocellValue);
  Serial.print("Avg: ");
  Serial.println(avgPhotocellValue);

  lcd.setCursor(0,0);
  lcd.print("LUM:");
  lcd.setCursor(4,0);
  lcd.print("    ");
  lcd.setCursor(4,0);
  lcd.print(photocellValue);
  lcd.setCursor(10,0);
  lcd.print("AVG:");
  lcd.setCursor(14,0);
  lcd.print(avgPhotocellValue);

  if (++ccycle >= ncycles) {
    ccycle = 0;

    if (avgPhotocellValue >= 300)
    {
      rampDown();
    }
    
    if (avgPhotocellValue <= 25)
    {
      rampUp();
    }
  }
}


// ************************************ Ramp Logic **************************************

void rampUp() {

  if ( rampClosed == 1 or rampUpError == true) return;         // return if ramp already open

  motorUp(255);                          // turn on motor
  unsigned long startTime = millis();
  Serial.print("Ramp is moving up..."); 
  delay(500);                            // delay a couple ms to get off switch

  while ( !switchIsClosed() ) {
    // error out if takes too long
    if ( millis() - startTime >= closeTimeout * 1000 ) {
      rampUpError = true;
      Serial.println("ERROR");
      motorOff();

      lcd.setCursor(0,3);
      lcd.print("RAMP: CLOSE ERROR!!!");
        
      return;
    }
  }

  motorOff();
  rampClosed = 1;
  
  Serial.println("OK");
  lcd.setCursor(0,3);
  lcd.print("RAMP: CLOSED");
}

void rampDown() {

  if ( rampClosed == 0 or rampDownError == true ) return;            // return if previous ramp down error

  motorDown(255);                          // turn on motor
  unsigned long startTime = millis();
  Serial.print("Ramp is moving down..."); 
  delay(500);                             // delay a couple seconds to get off switch
  
  while ( !switchIsClosed() ) {
    // error out if takes too long, or if top limit gets triggered (string wrap-around)
    if ( millis() - startTime >= openTimeout * 1000 ) {
      rampDownError = true;
      Serial.println("ERROR");
      motorOff();

      lcd.setCursor(0,3);
      lcd.print("RAMP: OPEN ERROR!!!");
      
      return;
    }
  }
  
  motorOff();
  rampClosed = 0;
  
  Serial.println("OK");
  lcd.setCursor(0,3);
  lcd.print("RAMP: OPEN");
}


// ********************************** Limit Switches ************************************

boolean switchIsClosed() {
  int reading1 = digitalRead(switchPin);
  delay(debounceDelay);
  int reading2 = digitalRead(switchPin);
  return (reading1 == reading2) && reading1 == 0;
}


// *********************************** Motor Control ************************************

// Start motor in UP direction at 'speed'
void motorUp(int speed) {
  //Serial.println("Motor on UP");
  digitalWrite(motorUpPin, HIGH);
  digitalWrite(motorDownPin, LOW);
  analogWrite(motorSpeedPin, speed);
}

// Start motor in DOWN direction at 'speed'
void motorDown(int speed) {
  //Serial.println("Motor on DOWN");
  digitalWrite(motorUpPin, LOW);
  digitalWrite(motorDownPin, HIGH);
  analogWrite(motorSpeedPin, speed);
}

// Stop motor and set speed to zero
void motorOff() {
  //Serial.println("Motor OFF");
  digitalWrite(motorUpPin, LOW);
  digitalWrite(motorDownPin, LOW);
  analogWrite(motorSpeedPin, 0);
}


// ********************************* Utility Functions **********************************

float movingAverage(float value) {
  //https://stackoverflow.com/questions/67208086/how-can-i-calculate-a-moving-average-in-arduino 
  const byte nvalues = 10;            // Moving average window
  static byte current = 0;            // Index for current value
  static byte cvalues = 0;            // Count of values read (<= nvalues)
  static float sum = 0;               // Rolling sum
  static float values[nvalues];

  sum += value;

  // If the window is full, adjust the sum by deleting the oldest value
  if (cvalues == nvalues)
    sum -= values[current];

  values[current] = value;            // Replace the oldest with the latest

  if (++current >= nvalues)
    current = 0;

  if (cvalues < nvalues)
    cvalues += 1;

  return sum/cvalues;
}
