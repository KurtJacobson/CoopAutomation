
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

const long openTimeout = 5;                 // timeout for ramp to open in s
const long closeTimeout = 5;                // timeout for ramp to close in s

const long debounceDelay = 50;              // limit switch debounce time in ms

const long photocellReadInterval = 1;       // time in s between light level readings
unsigned long lastPhotocellReadTime = 0;    // time of last light level reading
int photocellValue = 0;                     // last light level reading
float avgPhotocellValue = 0;                  // rolling avg light level

bool rampUpError = false;
bool rampDownError = false;


void setup() {

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
  readLightLevel();
  delay(photocellReadInterval * 1000);
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

  if (++ccycle >= ncycles) {
    ccycle = 0;

    if (avgPhotocellValue >= 500)
    {
      rampDown();
    }
    
    if (avgPhotocellValue <= 300)
    {
      rampUp();
    }
  }
}


// ************************************ Ramp Logic **************************************

void rampUp() {
  if ( !topLimitIsOpen( )) return;       // return if ramp already up
  if ( rampUpError ) return;             // return if previous ramp up error

  motorUp(255);                          // turn motor on
  Serial.print("Ramp is moving up...");
  
  unsigned long startTime = millis();
  while (topLimitIsOpen()) {
    // error out if takes too long
    if ( millis() - startTime >= closeTimeout * 1000 ) {
      rampUpError = true;
      Serial.println("ERROR");
      motorOff();
      return;
    }
  }
  Serial.println("OK");
  motorOff();
}

void rampDown() {
  if ( !bottomLimitIsOpen() ) return;      // return if ramp already down
  if ( rampDownError ) return;             // return if previous ramp down error

  motorDown(255);                          // turn on motor
  Serial.print("Ramp is moving down...");
  
  unsigned long startTime = millis();
  while (bottomLimitIsOpen()) {
    // error out if takes too long, or if top limit gets triggered (string wrap-around)
    if ( millis() - startTime >= openTimeout * 1000 or !topLimitIsOpen() ) {
      rampDownError = true;
      Serial.println("ERROR");
      motorOff();
      return;
    }
  }
  Serial.println("OK");
  motorOff();
}


// ********************************** Limit Switches ************************************

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
