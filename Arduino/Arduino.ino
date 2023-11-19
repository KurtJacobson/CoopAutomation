
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

const long openTime = 40;                 // time for ramp to open in s
const long closeTime = 40;                // time for ramp to close in s

const long debounceDelay = 50;              // limit switch debounce time in ms

const long photocellReadInterval = 6;       // time in s between light level readings
unsigned long lastPhotocellReadTime = 0;    // time of last light level reading
int photocellValue = 0;                     // last light level reading
float avgPhotocellValue = 0;                // rolling avg light level

int rampClosed = -1;                        // current ramp state


void setup() {
  // Motor control outputs
  pinMode(motorUpPin, OUTPUT);
  pinMode(motorDownPin, OUTPUT);
  pinMode(motorSpeedPin, OUTPUT);

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

    if (avgPhotocellValue >= 200)
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

  if ( rampClosed == 1) return;

  motorUp(255);
  Serial.print("Ramp is moving up..."); 
  delay(closeTime * 1000);

  motorOff();
  rampClosed = 1;
  
  Serial.println("OK");
}

void rampDown() {

  if ( rampClosed == 0) return;
  
  motorDown(255);
  Serial.print("Ramp is moving down..."); 
  delay(openTime * 1000);
  
  motorOff();  
  rampClosed = 0;
  
  Serial.println("OK");
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
