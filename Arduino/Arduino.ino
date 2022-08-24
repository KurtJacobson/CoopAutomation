
// ************************************ Define Pins *************************************

// Motor control outputs
const int motorUpPin = 12;
const int motorDownPin = 11;
const int motorSpeedPin = 10;

// Limit switch inputs
const int topLimitPin = 9;
const int bottomLimitPin = 8;

// Photocell input
const int photocellPin = A0;


// ************************************* Variables *************************************

const long openTimeout = 1;                 // timeout for ramp to open in s
const long closeTimeout = 1;                // timeout for ramp to close in s

unsigned long debounceDelay = 50;           // limit switch debounce time in ms

const long photocellReadInterval = 1;      // time in s between light level readings
unsigned long lastPhotocellReadTime = 0;    // time of last light level reading
int lastPhotocellReadValue = 0;             // last light level reading value


bool rampUpError = false;
bool rampDownError = false;


void setup() {
  // put your setup code here, to run once:

  // Motor control outputs
  pinMode(motorUpPin, OUTPUT);
  pinMode(motorDownPin, OUTPUT);
  pinMode(motorSpeedPin, OUTPUT);

  // Limit switch inputs
  pinMode(topLimitPin, INPUT);
  pinMode(bottomLimitPin, INPUT);

  Serial.begin(9600); // set serial port for communication
}

void loop() {
  // put your main code here, to run repeatedly:

  readLightLevel();

}


// ********************************* Photocell Reading **********************************

void readLightLevel() {

  // return if we already read the light level in the past 10min
  if ( (millis() - lastPhotocellReadTime) < ( photocellReadInterval * 1000 ) ) return;

  lastPhotocellReadTime = millis();

  // Read light level
  lastPhotocellReadValue = analogRead(photocellPin);
  Serial.print("Light value: ");
  Serial.print(lastPhotocellReadValue);

  // Light threshholds, qualitatively determined
  if (lastPhotocellReadValue < 10) {
    Serial.println(" - Dark");
  } else if (lastPhotocellReadValue < 200) {
    Serial.println(" - Dim");
  } else if (lastPhotocellReadValue < 500) {
    Serial.println(" - Light");
  } else if (lastPhotocellReadValue < 800) {
    Serial.println(" - Bright");
  } else {
    Serial.println(" - Very bright");
  }
  


  if (lastPhotocellReadValue >= 500)
  {
    //motorUp(255);
    rampDown();
  }
  
  if (lastPhotocellReadValue <= 300)
  {
    //motorDown(255);
    rampUp();
  }

  //if (lastPhotocellReadValue < 600 && lastPhotocellReadValue > 300)
  //{
  //  motorOff();
  //}
  
}



// ********************************** Limit Switches ************************************


void rampUp() {
  if (topLimitIsOpen()) {
    if ( rampUpError ) return;
    Serial.print("Ramp is moving up...");
    unsigned long startTime = millis();
    while (topLimitIsOpen()) {
      motorUp(255);
      // error out if takes too long
      if ( (millis() - startTime) >= closeTimeout * 1000 ) {
        rampUpError = true;
        Serial.println("ERROR");
        motorOff();
        return;
      }
    }
    Serial.println("OK");
    motorOff();
  }
}

void rampDown() {
  if (bottomLimitIsOpen()) {
    if ( rampDownError ) return;
    Serial.print("Ramp is moving down...");
    unsigned long startTime = millis();
    while (bottomLimitIsOpen()) {
      motorDown(255);
      // error out if takes too long, or if top limit gets triggered (string wrap-around)
      if ( (millis() - startTime) >= ( openTimeout * 1000 ) or !topLimitIsOpen() ) {
        rampDownError = true;
        Serial.println("ERROR");
        motorOff();
        return;
      }
    }
    Serial.println("OK");
    motorOff();
  }
}

boolean topLimitIsOpen() {
  int reading1 = digitalRead(topLimitPin);
  delay(debounceDelay);
  int reading2 = digitalRead(topLimitPin);
  return (reading1 == reading2) && reading1 == 0;
}

boolean bottomLimitIsOpen() {
  int reading1 = digitalRead(bottomLimitPin);
  delay(debounceDelay);
  int reading2 = digitalRead(bottomLimitPin);
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
