

#include <Wire.h>
#include <Servo.h>
#include <rgb_lcd.h>

rgb_lcd lcd;

boolean start;
boolean started = false;

// Pin constants *****************************
// NOTE: Don't have any on pin 13 as this pin goes high when reset

// high and low bobbers
const int InHigh =  3;
const int InLow  =  2;

// ultrasonic sensor pins
const int triggerPin = 6;
const int echoPin = 7;

// reference voltage pin
const int InRef  = 8;

// LEDs to show on or off
const int HighLED = 9;
const int LowLED  = 10;

//end of pin constants ******************************************


// Use "unsigned long" for variables that hold time
// since the value will quickly become too large for an int to store
unsigned long startTime = 0;                        // store the tide half-cycle starting time
unsigned long previousTime = 0;                     // store the time since the pump was either turned on or off.

// 6.5 hours is the half-period of a tide (in msec)
const unsigned long TideInterval = 6.5 * 60 * 60 * 1000;

// Pump is ON for 5 minutes
const unsigned long pumpOnTime = 5.0 * 60 * 1000;

// Pump is OFF for 6.1 minutes
const unsigned long pumpOffTime = 6.1 * 60 * 1000;

// Tide state variables
boolean HighTide = false; // Default lowering tide when you first turn on the system
boolean LowTide = false;

//bools for pump state
boolean filling = false;
boolean draining = false;
boolean PumpOn = true;

//Limiters
boolean overFull = false;
boolean tooLow = false;

// Sensor varialbles - Initially set all the switches LOW (not tripped)
int sensorHigh = LOW;
int sensorLow = LOW;
int sensorRef = LOW;

// Limiter sensors set to LOW (not tripped)
int highLimitSensor = LOW;
int lowLimitSensor = LOW;

// Set the last sensor states
int lastSensorHigh = sensorHigh;
int lastSensorLow = sensorLow;
int lastSensorRef = sensorRef;

// Sensor reading variabls
int readingSensorHigh;
int readingSensorLow;
int readingSensorRef;

unsigned long lastTimeHigh = 0;
unsigned long lastTimeLow = 0;
unsigned long lastTimeRef = 0;
unsigned long lastTimePulseHigh = 0;
unsigned long lastTimeLCDUpdated = 0;

const unsigned long debounceDelay = 100;
const unsigned long pulseDelay = 1000;       // delay a second before triggering the ultrasonic sensor
const unsigned long lcdDelay = 500;           // delay a half of a second between LCD updates

// Indicator light variables
unsigned long timeLED = 0;
int stateLED = LOW;             // ledState used to set the LED
long blinkLED = 500;           // interval at which to blink (milliseconds)
const long blinkFast = 150;
const long blinkSlow = 1000;

// ultrasonic sensor variables
unsigned long lastTimeUpdated = 0;
short delayUpdate = 3000;               // update only every 3000 milliseconds (3 seconds)


// Strings to hold the current state and sensor information
const int lenString = 25;
char StateString[lenString] = " ";
char SensorString[lenString] = " ";
char prevStateString[lenString] = " ";
char prevSensorString[lenString] = " ";

//veriable to hold time counter
unsigned long currentTime;


//create servo objects to control servos
// servos use pins 5 and 11 as well as power supply
Servo tideServo;
Servo rayServo;

//counter for log
unsigned long logCount = 0;

//Functions--------------------------------------

void dumpState(long currentTime, String msg) {
  /* Prints state to serial monitor */
  Serial.print(F("Time "));
  Serial.println(currentTime);
  // ensure these are correct
  Serial.print(F("TideInterval "));
  Serial.println(TideInterval);
  Serial.print(F("PumpOnTime "));
  Serial.println(pumpOnTime);
  Serial.print(F("PumpOffTime "));
  Serial.println(pumpOffTime);

  Serial.println(msg);
  Serial.print(F("The variable PumpOn is "));
  Serial.println(PumpOn);
  Serial.print(F("The variable filling is "));
  Serial.println( filling );
  Serial.print(F("   FYI: High Water Sensor = "));
  Serial.println(sensorHigh);
  Serial.print("   FYI: Low Water Sensor = ");
  Serial.println(sensorLow);
  Serial.print(F("   FYI: Reference Sensor = "));
  Serial.println(sensorRef);
  Serial.println();

}


//Turns on Neutral for 5 seconds followed by 
//Tide in for 5 seconds then tideOut for 5 seconds.
void test(){
  tideIdle();
  delay(5000);
  tideIn();
  delay(5000);
  tideOut();
  delay(5000);
  
}

//tideIdle water goes into both tanks
void tideIdle(){
  tideServo.write(0);
  rayServo.write(90);
  
  lcd.print("Tide Idle");
  delay(2000);
  lcd.clear();
}


//move water from ray tank to tidepool
void tideIn(){
  //tideIdle();
  tideServo.write(0);
  rayServo.write(0);
  
  //display to lcd
  lcd.print("Tide Pool Filling");
  delay(2000);
  lcd.clear();
  //strncpy(StateString, "TidePool ", lenString);  
}

//move water out of the tidepool into the ray tank
void tideOut(){
  //tideIdle();Filling
  tideServo.write(90);
  rayServo.write(90);
  
  // output to lcd
  lcd.print("Tide Pool Filling");
  delay(2000);
  lcd.clear();
  //strncpy(StateString, "TidePool Draining", lenString);
}

//gets reading from tide pool ultrasonic sensor.
String getTWaterDepth(){
  String depth = "22";
  return depth;
}

//gets ray tank ultrasonic sensor reading.
String getRWaterDepth(){
  String depth = "9";
  return depth;
}

//calculates the total water volume.
String getTotalVolume(){
  String volume = "600";
  return volume;
}

//gets the tide state.
String getTide(){
  String tide = "outgoing";
  
  if(filling == true){
    tide = "Incoming Tide";
  }
  else{
    tide = "Outgoing Tide";
  }
  return tide;
}

//gets the temperature reading.
String getAvTemp(){
  String tmp = "21";
  return tmp;
}

//create and send log files to PI
void sendLog(){
  String logStr = "";

  logStr = "LOG," + getTWaterDepth();
  logStr += "," + getRWaterDepth();
  logStr += "," + getTotalVolume();
  logStr += "," + getTide();
  logStr += "," + getAvTemp();

  Serial.println(logStr);  
  }

//logTimer()
void logTimer(){
  //check logCount
  if(logCount >= 60000){
    sendLog();
    logCount = 0;
  }
  else{
    logCount++;
  }
}

void initVariables(){
  if ( currentTime - startTime >= TideInterval || startTime > currentTime) {
    //Initially set the flow to neutral
    tideIdle();
    
    // start variable makes sure this loop is executed at the beginning when t=0
    start = false;
    //check for high tide
    if ( HighTide ) {
      HighTide = false;
      filling = false;
    }
    else if ( LowTide ) {
      LowTide = false;
      filling = true;
    }
    else {
      filling = !filling;      // possibly the high and low tide sensor switches never get triggered
      // in the tide interval (testing-situation)
    }
    
    // Reset the start of the tide cycle
    startTime = currentTime;
    previousTime = currentTime;
    
    PumpOn = false;            // Turn the pump off. In five minutes, the pump will be on
    blinkLED = blinkFast;
    
    // update LCD Screen
    if (filling) {
      strncpy(StateString, "TidePool Drained", lenString);
      //Serial.println(F("In tide change, updating tidepool drained"));
      //dumpState(currentTime, "TidePool Drained");
    }
    else {
      strncpy(StateString, "TidePool Full", lenString);
      //Serial.print(currentTime);
      //Serial.println(StateString);
      //dumpState(currentTime, "Tidepool Full");
    }
  }
}
//end initVariables

// checkSwitchState
// checks the state of the float sensors
void checkSwitchState(){
  
  if ( readingSensorHigh != lastSensorHigh ) {
    // Set the time and then we will will wait to see if the state change is stable
    lastTimeHigh = currentTime;
  }
  // If enough time has passed then the sensor is not fluctuating
  if ( (currentTime - lastTimeHigh) > debounceDelay ) {
    if ( readingSensorHigh != sensorHigh )
      sensorHigh = readingSensorHigh;
  }
  lastSensorHigh = readingSensorHigh;

  if ( readingSensorLow != lastSensorLow ) {
    // Set the time and then we will will wait to see if the state change is stable
    lastTimeLow = currentTime;
  }
  // If enough time has passed then the sensor is not fluctuating
  if ( (currentTime - lastTimeLow) > debounceDelay ) {
    if ( readingSensorLow != sensorLow )
      sensorLow = readingSensorLow;
  }
  lastSensorLow = readingSensorLow;

  if ( readingSensorRef != lastSensorRef ) {
    // Set the time and then we will will wait to see if the state change is stable
    lastTimeRef = currentTime;
  }
  // If enough time has passed then the sensor is not fluctuating
  if ( (currentTime - lastTimeRef) > debounceDelay ) {
    if ( readingSensorRef != sensorRef ) {
      sensorRef = readingSensorRef;
      // If Sensor reading switched than print out an update
      Serial.print(F("Reference Sensor = "));
      Serial.println(sensorRef);
    }
  }
  lastSensorRef = readingSensorRef;
  
}

// setHighLowVals
void setHighLowVals(){
  
  if ( sensorHigh == HIGH ) {
    HighTide = true;
    digitalWrite( HighLED, HIGH);
    
    strncpy(SensorString, "HIGH WATER!", lenString);
  } 
  else {
    HighTide = false;
    digitalWrite( HighLED, LOW);
    Serial.println(F("Setting High and Low Tide, Made LED LOW"));
  }

  strncpy(SensorString, "Check sensor Low == HIGH", lenString);
  
  if (sensorLow == HIGH) {
    LowTide = true;
    digitalWrite( LowLED, HIGH);
    
    strncpy(SensorString, "LOW WATER!", lenString);
  } 
  else {
    LowTide = false;
    digitalWrite( LowLED, LOW);
  }

  if (sensorLow == LOW and sensorHigh == LOW) {
    
    strncpy(SensorString, "Finish Update", lenString);

  }

  // if both the low sensor and high sensor are tripped, we have an error
  if (sensorLow == HIGH and sensorHigh == HIGH) {
    strncpy(SensorString, "SENSOR ERROR!", lenString);
  }

  // Blink the indicator LED if it is not high or low tide
  if ( !HighTide && !LowTide ) {
    if ( (currentTime - timeLED) >= blinkLED ) {
      // if the LED is off turn it on and vice-versa:
      if ( stateLED == LOW) {
        stateLED = HIGH;
      } else {
        stateLED = LOW;
      }
      timeLED = currentTime;
    }
    if ( filling ) {
      digitalWrite( HighLED, stateLED);
    } else {
      digitalWrite( LowLED, stateLED);
    }
  }

}

void runCycle(){
  //Serial.println(F("Checking filling & !HighTide"));
  if ( filling ){
    // if in high tide turn pump off
    // if not high tide, switch between turning the pump on and off
    if ( HighTide ){
      // only turn off if the pump has been on longer than pumpOnTime
      if ((currentTime - previousTime) >= pumpOnTime) {
        PumpOn = false;
        previousTime = currentTime;
        strncpy(StateString, "Stop fill High Tide", lenString);
        tideIdle();
        }
    } 
    else if ( PumpOn && (currentTime - previousTime) >= pumpOnTime ){
      PumpOn = false;
      blinkLED = blinkSlow;
      previousTime = currentTime;
      
      strncpy(StateString, "Pause Filling", lenString);
    }
    else if ( !PumpOn && (currentTime - previousTime) >= pumpOffTime ){
      PumpOn = true;
      blinkLED = blinkFast;
      previousTime = currentTime;
      
      // print if is filling
      strncpy(StateString, "Filling Tidepool", lenString);
    }
    if ( PumpOn && !HighTide) {
      // Pump stays on for the pumpOnTime
      //digitalWrite( PumpFill, writePumpOn);
      tideIn();
      }
      else{
        // Pump stays off for the pumpOffTime
        //digitalWrite( PumpFill, writePumpOff);
        tideIdle();
      }
  }
  
  //Serial.println(F("Not filling"));
  if ( !filling ) {
    if ( LowTide ) {
      if ((currentTime - previousTime) >= pumpOnTime) {
        PumpOn = false;
        previousTime = currentTime;
        
        strncpy(StateString, "Stop drain low tide", lenString);
      }
    }
    else if ( PumpOn && (currentTime - previousTime) >= pumpOnTime ) {
      PumpOn = false;
      blinkLED = blinkSlow;
      previousTime = currentTime;
      
      // print if paused draining
      strncpy(StateString, "Pause Draining Tidepool", lenString);
    }
    else if ( !PumpOn && (currentTime - previousTime) >= pumpOffTime ) {
      PumpOn = true;
      blinkLED = blinkFast;
      previousTime = currentTime;
      
      // print if is draining
      strncpy(StateString, "Draining Tidepool", lenString);
    }
    //Serial.println(F("in Draining, checking if pumpon"));
    if ( PumpOn && !LowTide) {
      // Pump stays on for the pumpOnTime
      tideOut();
    }
    else {
      // Pump stays off for the pumpOffTime
      tideIdle();
    }
  }
}

void setStateSring(){
  //Serial.println(F("Checking state string"));

  /* Update LCD Screen if the state or sensor strings are different */
  
  // state goes on first row of LCD screen
  Serial.println(StateString);
  Serial.println(prevStateString);
  if (strcmp(StateString, prevStateString) != 0 && (currentTime - lastTimeLCDUpdated) > lcdDelay) {
    strncpy(prevStateString, StateString, lenString);  // update previous stateString for comparison later
    lastTimeLCDUpdated = currentTime;

    Serial.println(prevStateString);
    Serial.println(F("Setting cursor"));
    lcd.setCursor(0,0);
    // pad remaining with spaces as to overwrite old message
    Serial.println(F("Printing String"));
    if (strlen(StateString) > 16) StateString[16] = '\0';           // shortens string to 16 characters
    lcd.print(StateString);
    Serial.println(F("padding string"));
    for(int i = strlen(StateString); i < 16; i++) { 
      Serial.print(" For i");
      Serial.print(i);
      lcd.print(' ');                       // add spaces to overwrite previous state
    }
  } // end if StateString different

  if (strcmp(SensorString, prevSensorString) != 0  && (currentTime - lastTimeLCDUpdated) > lcdDelay) {
    strncpy(prevSensorString, SensorString, lenString);  // update previous stateString for comparison later
    lastTimeLCDUpdated = currentTime;
    
    Serial.println(F("inside sensor update"));
    lcd.setCursor(0,1);                   // sensor info goes on second row of LCD
    // pad remaining with spaces as to overwrite old message
    Serial.println(F("Printing String"));
    if (strlen(SensorString) > 16)  SensorString[16] = '\0';           // shortens string to 16 characters
    lcd.print(SensorString);
    Serial.println(F("padding string"));
    for(int i = strlen(SensorString); i < 16; i++) { 
      Serial.print(" For i");
      Serial.print(i);
      lcd.print(' ');                       // add spaces to overwrite previous state
    }
    
    Serial.println(F("Updating colors"));
    // update color to show errors
    if (SensorString[0] == ' ') {
      lcd.setRGB(0, 255, 0);               // no message, so no sensor triggered therefore green
    }
    else if (strcmp(SensorString, "SENSOR ERROR!") == 0) {
        lcd.setRGB(255, 0, 0);           // has error, turn background red
    } else {
        lcd.setRGB(0, 0, 255);          // high or low sensor triggered, turn background blue
    }
  } // end sensor string update

  // update online log with water level
  if((currentTime - lastTimeUpdated) > delayUpdate) {
    Serial.println(F("Updating online water level"));
    float duration, distance;
    
    // send ultrasonic pulse
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    
    // see how long pulse takes to return to echo pin
    duration = pulseIn(echoPin, HIGH, 50000);
    distance = (duration / 2) / 29.1;    // Change time to get pulse back to centimeters
    Serial.print("duration->");
    Serial.println(duration);
    
    // save count to the 'distance' feed on Adafruit IO
    Serial.print("level sending -> ");
    Serial.println(distance);
    //ultrasonicSensorFeed->save(distance);

    // update last time updated
    lastTimeUpdated = currentTime;
  }
}

//End of functions----------------

// Run initial setup
void setup() {
  
  // Start serial connection so that we can print to the LCD screen for testing
  Serial.begin(9600);
  
  lcd.begin(16, 2);     // set up the LCD's number of columns and rows
  lcd.setRGB(0, 255, 0); // make lcd green initially
  
  // Set the various digital input pins
  pinMode( InHigh, INPUT );
  pinMode( InLow, INPUT );
  pinMode( InRef, INPUT );
  pinMode( echoPin, INPUT );
  
  // Set the various digital output pins
  pinMode( HighLED, OUTPUT );
  pinMode( LowLED, OUTPUT );
  pinMode( triggerPin, OUTPUT );
  
  //set started to false
  started = false;
  
  // these variables will be used later to change whether the tide pool if filling or emptying
  LowTide = digitalRead(InLow);
  HighTide = digitalRead(InHigh);
  
  //dumpState(0, "");              // print current state
  
  //attaches tideServo to pin 5
  tideServo.attach(5);
  
  //attaches rayServo to pin 11
  rayServo.attach(11); 

  //set tide to Idle
  tideIdle();
}

// Main program
void loop() {
  
  logTimer();
  // Read the current time... everything is testing time intervals.
  currentTime = millis();
  
  // If you are at High or Low tide you need to wait until the tide interval is complete.  The time has to reach
  // the end of the TideInterval.
  
  // initializes variables at the start of a filling or falling tide
  initVariables();
  // Now we need to check our sensors.... but make sure to debounce the readings
  
  readingSensorHigh = digitalRead(InHigh);
  readingSensorLow = digitalRead(InLow);
  readingSensorRef = digitalRead(InRef);
  
  // Check to see if the switches changed state due to either noise or water level
  checkSwitchState();
  
  /*
      Add a user programming feature here where you could action based on both triggers set.
      Could enter program mode with input set by toggling switches.
      Not implemented yet...
  */

  // Set the HighTide or LowTide boolean values and then turn off the appropriate valve
  // Indicator light: solid LED if a sensor is set, blink the HighLED if you are filling
  // and blink the LowLED if you are not filling
  
  // sets the filling and draining
  setHighLowVals();

  /*
      Need to wait until the High or Low tide time interval has been met. Then start a pump.
      We want to cycle the correct pump on and off with the cycle time.
      PumpFill fills the pool
      PumpDrain empties the pool
  */

  //call runCycle
  runCycle();
  
  // set stateString
  //setStateSring();--------------------
}
// End of loop()
