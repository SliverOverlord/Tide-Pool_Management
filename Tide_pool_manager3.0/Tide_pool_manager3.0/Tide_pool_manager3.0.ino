/*
  Author: Joshua DeNio 
  Contributors: Steve Lindaas, Paige Meyer
  Date:6/12/2019
  Description:
    This program runs on an Arduino and is used to control water movement between two marine tanks replicating a natural tide pool.
    There are two valves controlled using servos to direct water to the desired destination. The pumps always run to prevent 
    pump failure. The Arduino reads two float sensors to control the range of water depth. The code will control the cycle time
    to match a natural tide cycle as closely as possible.
 */

#include <Wire.h>
#include <Servo.h>
#include <rgb_lcd.h>
#include <DallasTemperature.h>
#include <OneWire.h>

rgb_lcd lcd;

//boolean start;
boolean started;

// Pin constants *****************************
// NOTE: Don't have any on pin 13 as this pin goes high when reset

// high and low bobbers
const int InHigh =  3;
const int InLow  =  2;

// ultrasonic sensor pins
const int tideTriggerPin = 6;
const int tideEchoPin = 7;

// reference voltage pin
const int InRef  = 8;

// LEDs to show on or off
const int HighLED = 9;
const int LowLED  = 10;

//temperature sensor pin
const int tempIn = 4;

//pins for ray tank sensor
// ultrasonic sensor pins
const int rayTriggerPin = 12;
const int rayEchoPin = 13;

//Pins 5 and 11 are saved for servos

//end of pin constants ******************************************


// Use "unsigned long" for variables that hold time
// since the value will quickly become too large for an int to store
unsigned long startTime = 0;                        // store the tide half-cycle starting time
unsigned long previousTime = 0;                     // store the time since the pump was either turned on or off.

// 6.5 hours is the half-period of a tide (in msec)
const unsigned long TideInterval = 6.2438 * 60 * 60 * 1000;

// Pump is ON for 5 minutes
const unsigned long pumpOnTime = 5.0 * 60 * 1000;

// Pump is OFF for 6.1 minutes
const unsigned long pumpOffTime = 6.1 * 60 * 1000;

// Tide state variables
boolean HighTide = false; // Default lowering tide when you first turn on the system
boolean LowTide = false;

//bools for pump state
boolean filling;
boolean draining;
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

//ints for ultrasonic sensor height
float raySensorHeight = 81.5;
float tideSensorHeight = 93;

float tankWidth = 122.5;
float tankLength = 367.00;

int printDelay = 3000;

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

//wire buss for temp sensor
OneWire oneWire(tempIn);
DallasTemperature sensors(&oneWire);

//Functions--------------------------------------

//prints one string to the lcd
void printToLcd1(String str1){
  //clear LCD
  lcd.clear();
  
  // print to the lcd
  lcd.print(str1);
  delay(printDelay);
}

//prints two strings to the lcd
void printToLcd2(String str1, String str2){
  //clear LCD
  lcd.clear();
  
  // print to the lcd
  lcd.print(str1);
  lcd.setCursor(0,1);
  lcd.print(str2);
  delay(printDelay);
}


//updates lcd with current state
void dumpState(String tideStr, String tideDepth, String rayDepth, String temp) {
 
  printToLcd1("Sending Log");

  //print tide state to the lcd
  printToLcd1(tideStr);

  lcd.setCursor(0,1);
  // output to lcd
  lcd.print(tideDepth);
  lcd.print(" cm");
  delay(printDelay);


  //clear LCD
  lcd.clear();
  // output to lcd
  lcd.print("Ray Tank");
  lcd.setCursor(0,1);
  // output to lcd
  lcd.print("Depth: ");
  lcd.print(rayDepth);
  lcd.print(" cm");
  delay(printDelay);

  lcd.clear();
  lcd.print("Temperature:");
  lcd.setCursor(0,1);
  lcd.print(temp+" C");
  delay(printDelay);
  /* Prints state to serial monitor */
//  Serial.print(F("Time "));
//  Serial.println(currentTime);
//  // ensure these are correct
//  Serial.print(F("TideInterval "));
//  Serial.println(TideInterval);
//  Serial.print(F("PumpOnTime "));
//  Serial.println(pumpOnTime);
//  Serial.print(F("PumpOffTime "));
//  Serial.println(pumpOffTime);
//
//  Serial.println(msg);
//  Serial.print(F("The variable PumpOn is "));
//  Serial.println(PumpOn);
//  Serial.print(F("The variable filling is "));
//  Serial.println( filling );
//  Serial.print(F("   FYI: High Water Sensor = "));
//  Serial.println(sensorHigh);
//  Serial.print("   FYI: Low Water Sensor = ");
//  Serial.println(sensorLow);
//  Serial.print(F("   FYI: Reference Sensor = "));
//  Serial.println(sensorRef);
//  Serial.println();

}


//Turns on Neutral for 5 seconds followed by 
//Tide in for 5 seconds then tideOut for 5 seconds.
void test(){
  tideIdle();
  delay(7000);
  tideIn();
  delay(7000);
  tideOut();
  delay(7000);

  //Set back to idle
  tideIdle();

  sendLog();
}

//Sets the tidepool to lowTide
void setLowTide(){
  lcd.clear();
  lcd.print("Setting System");
  lcd.setCursor(0,1);
  lcd.print("To Low Tide");
  delay(5000);
  
  //add second sensor to loop later

  //while not low tide pump out some water and check level
  while(LowTide == false){
    tideOut();
    delay(60000);
    tideIdle();
    LowTide = digitalRead(InLow);
    sendLog();
  }
  
  //seting state variables
  filling = false;
  draining = true;
  
  lcd.clear();
  lcd.print("Tide Set");
  lcd.setCursor(0,1);
  lcd.print("To Low");
  delay(5000);
}

//Sets the tidepool to HighTide
void setHighTide(){
  lcd.clear();
  lcd.print("Setting System");
  lcd.setCursor(0,1);
  lcd.print("To High Tide");
  delay(5000);
  
  //add second sensor to loop later
  while(HighTide == false){
    tideIn();
    delay(60000);
    tideIdle();
    sendLog();
    HighTide = digitalRead(InHigh);
  }
  
  //seting state variables
  filling = true;
  draining = false;
  
  lcd.clear();
  lcd.print("Tide Set");
  lcd.setCursor(0,1);
  lcd.print("To High");
  delay(5000);
}


//tideIdle water goes into both tanks
void tideIdle(){
  tideServo.write(0);
  rayServo.write(90);
  
  //clear LCD
  lcd.clear();
  
  // output to lcd
  lcd.print("Tide Pool Is");
  lcd.setCursor(0,1);
  lcd.print("Idle");
  delay(printDelay);
}


//move water from ray tank to tidepool
void tideIn(){
  //tideIdle();
  tideServo.write(0);
  rayServo.write(0);
  
  //clear LCD
  lcd.clear();
  
  // output to lcd
  lcd.print("Tide Pool Is");
  lcd.setCursor(0,1);
  lcd.print("Filling");
  delay(printDelay);
}

//move water out of the tidepool into the ray tank
void tideOut(){
  //tideIdle();Filling
  tideServo.write(90);
  rayServo.write(90);
  
  //clear LCD
  lcd.clear();
  
  // output to lcd
  lcd.print("Tide Pool Is");
  lcd.setCursor(0,1);
  lcd.print("Draining");
  delay(printDelay);
}

float getWaterDepth(int triggerPin, int echoPin, float sensorHeight){
  float duration;
  float distance;
  float waterDepth;
    
    // send ultrasonic pulse
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    
    // see how long pulse takes to return to echo pin
    duration = pulseIn(echoPin, HIGH, 50000);
  
  // Change time to get pulse back to centimeters
    distance = (duration / 2) / 29.1;
    waterDepth = sensorHeight - distance;
    
  return waterDepth;
  }

float averageWaterDepth(int triggerPin, int echoPin, float sensorHeight){
  float distance1 = getWaterDepth(triggerPin, echoPin, sensorHeight);
  float distance2 = getWaterDepth(triggerPin, echoPin, sensorHeight);
  float distance3 = getWaterDepth(triggerPin, echoPin, sensorHeight);
  float lowestMeasure;

  if(distance1 <= distance2){
    lowestMeasure = distance1;
  }
  else{
    lowestMeasure = distance2;
  }
  if(distance3 < lowestMeasure){
    lowestMeasure = distance3;
  }
  return lowestMeasure;
}

//calculates the total water volume in liters.
String getTotalVolume(float tDepth, float rDepth){

  //calculate the total water volume
  float volume = tankLength*tankWidth*(tDepth+rDepth);

  //convert to Liters
  volume = volume/1000;
  
  return String(volume);
}

//gets the tide state.
String getTide(){
  String tide = "";
  
  if(filling == true){
    tide = "Incoming Tide";
  }
  else if(HighTide == true){
    tide = "High Tide";
  }
  else if(LowTide == true){
    tide = "Low Tide";
  }
  else{
    tide = "Outgoing Tide";
  }
  return tide;
}

//gets the temperature reading.
String getAvTemp(){
  String tmp = "21";

  sensors.requestTemperatures();
  tmp =String(sensors.getTempCByIndex(0));

  return tmp;
}

//create and send log files to PI
void sendLog(){
  //String logStr1 = "LOG,22,9,500,outgoing,26";
  String logStr = "";
  String tideStr = getTide();
  float tideDepth = averageWaterDepth(tideTriggerPin,tideEchoPin, tideSensorHeight);
  float rayDepth = averageWaterDepth(rayTriggerPin,rayEchoPin, raySensorHeight);
  String temp = getAvTemp();

  logStr = "LOG," + String(tideDepth);
  logStr += "," + String(rayDepth);
  logStr += "," + getTotalVolume(tideDepth, rayDepth);
  logStr += "," + tideStr;
  logStr += "," + temp;

  Serial.println(logStr);
  dumpState(tideStr, String(tideDepth), String(rayDepth), temp);
  }

//logTimer()
void logTimer(){
  //check logCount
  if(logCount >= 2){
    sendLog();
    logCount = 0;
  }
  else{
    logCount++;
  }
}


//intiualize the High low variables
void initVariables(){
  if ( (currentTime - startTime >= TideInterval) || (startTime > currentTime) || (started == false)) {
    //Initially set the flow to neutral
    tideIdle();
    
    // started variable makes sure this loop is executed at the beginning after initial setup
    started = true;
    
    
    //check for high tide
    if ( HighTide ) {
      HighTide = false;
      filling = false;
      draining = true;

      lcd.setRGB(0,255,0);
    }
    else if ( LowTide ) {
      LowTide = false;
      filling = true;
      draining = false;
      lcd.setRGB(0,0,230);
    }
    else {
      // possibly the high and low tide sensor switches never get triggered
      filling = false;
      draining = true;
      
      lcd.setRGB(0,255,0);
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
  lcd.clear();
  //check for high tide
  if ( sensorHigh == HIGH ) {
    HighTide = true;
    digitalWrite( HighLED, HIGH);
    
    
    lcd.print("High Water");
  } 
  else {
    HighTide = false;
    digitalWrite( HighLED, LOW);
    Serial.println(F("Setting High and Low Tide, Made LED LOW"));
  }


  //check for low tide
  if (sensorLow == HIGH) {
    LowTide = true;
    digitalWrite( LowLED, HIGH);
    lcd.print("Low Tide");
  } 
  else {
    LowTide = false;
    digitalWrite( LowLED, LOW);
  }
  
  //check for error
  if (sensorLow == LOW and sensorHigh == LOW) {
    lcd.print("Updating");
  }


  // if both the low sensor and high sensor are tripped, we have an error
  if (sensorLow == HIGH and sensorHigh == HIGH) {
    lcd.setRGB(255,0,0);
    lcd.print("Sensor Error");
  }


  // Blink the indicator LED if it is not high or low tide
  if ( !HighTide && !LowTide ) {
    if ( (currentTime - timeLED) >= blinkLED ) {
    // if the LED is off turn it on and vice-versa:
      if ( stateLED == LOW) {
        stateLED = HIGH;
      } 
      else {
        stateLED = LOW;
      }
      timeLED = currentTime;
    }
    if ( filling ) {
      digitalWrite( HighLED, stateLED);
    }
  else {
      digitalWrite( LowLED, stateLED);
    }
  }

}

void showTideState(){
  lcd.clear();
  if(filling == true){
    lcd.setRGB(0,0,230);
    lcd.print("Tide");
    lcd.setCursor(0,1);
    lcd.print("Incoming");
  }
  else{
    lcd.setRGB(0,255,0);
    lcd.print("Tide");
    lcd.setCursor(0,1);
    lcd.print("Outgoing");
  }
  
  delay(1000);
}

void showMovingWater(){
  lcd.clear();
  if(PumpOn == true){
    lcd.print("Moving water");
  }
  else{
    lcd.print("Not Moving Water");
  }
  delay(1000);
}

void runCycle(){
  //Serial.println(F("Checking filling & !HighTide"));
  
  showTideState();
  
  showMovingWater();
  
  if ( filling == true ){
    // if in high tide turn pump off
    // if not high tide, switch between turning the pump on and off
    if ( HighTide ){
      // only turn off if the pump has been on longer than pumpOnTime
      if ((currentTime - previousTime) >= pumpOnTime) {
        PumpOn = false;
        previousTime = currentTime;
        //strncpy(StateString, "Stop fill High Tide", lenString);
        printToLcd2("Stop Fill","High Tide");
        tideIdle();
        }
    } 
    else if ( PumpOn && (currentTime - previousTime) >= pumpOnTime ){
      PumpOn = false;
      blinkLED = blinkSlow;
      previousTime = currentTime;
      
      // Pump stays on for the pumpOnTime
      //strncpy(StateString, "Pause Filling", lenString);
      //print to lcd
      printToLcd1("Pause Filling");
      
      //seting tide to idle
      tideIdle();
    }
    else if ( (PumpOn == false) && (currentTime - previousTime) >= pumpOffTime ){
      PumpOn = true;
      blinkLED = blinkFast;
      previousTime = currentTime;
      
      // Pump stays off for the pumpOffTime
      
      //print to lcd
      printToLcd1("Filling Tidepool");

      //set to fill tidepool
      tideIn();
    }
    
  }
  
  //Serial.println(F("Not filling"));
  if ( draining == true) {
    if ( LowTide ) {
      if ((currentTime - previousTime) >= pumpOnTime) {
        PumpOn = false;
        previousTime = currentTime;

        //print to LCD
        printToLcd2("Stop Drain","Low Tide");

        //set to idle
        tideIdle();
      }
    }
    else if ( (PumpOn == true) && (currentTime - previousTime) >= pumpOnTime ) {
      PumpOn = false;
      blinkLED = blinkSlow;
      previousTime = currentTime;
      
      // print if paused draining
      //print to LCD
      printToLcd2("Pause Draining","Tide Pool");
      
      //set to idle
      // Pump stays off for the pumpOffTime
      tideIdle();
    }
    else if ( (PumpOn == false) && (currentTime - previousTime) >= pumpOffTime ) {
      PumpOn = true;
      blinkLED = blinkFast;
      previousTime = currentTime;
      
      // print if is draining

      //print to LCD
      printToLcd2("Draining","Tide Pool");
      
      // Pump stays on for the pumpOnTime
      tideOut();
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
        lcd.setRGB(0, 0, 230);          // high or low sensor triggered, turn background blue
    }
  } // end sensor string update

  // update online log with water level
  if((currentTime - lastTimeUpdated) > delayUpdate) {
    Serial.println(F("Updating online water level"));
//    float duration, distance;
//    
//    // send ultrasonic pulse
//    digitalWrite(triggerPin, LOW);
//    delayMicroseconds(2);
//    digitalWrite(triggerPin, HIGH);
//    delayMicroseconds(10);
//    digitalWrite(triggerPin, LOW);
//    
//    // see how long pulse takes to return to echo pin
//    duration = pulseIn(echoPin, HIGH, 50000);
//    distance = (duration / 2) / 29.1;    // Change time to get pulse back to centimeters
//    Serial.print("duration->");
//    Serial.println(duration);
//    
//    // save count to the 'distance' feed on Adafruit IO
//    Serial.print("level sending -> ");
//    Serial.println(distance);
    //ultrasonicSensorFeed->save(distance);

    // update last time updated
    lastTimeUpdated = currentTime;
  }
}

//End of functions----------------

// Run initial setup
void setup() {
  
  // Start serial connection so that we can print to the LCD screen for testing
  Serial.begin(115200);
  
  lcd.begin(16, 2);     // set up the LCD's number of columns and rows
  lcd.setRGB(0, 255, 0); // make lcd green initially
  
  // Set the various digital input pins
  pinMode( InHigh, INPUT );
  pinMode( InLow, INPUT );
  pinMode( InRef, INPUT );
  pinMode( tideEchoPin, INPUT );
  pinMode( rayEchoPin, INPUT );
  
  // Set the various digital output pins
  pinMode( HighLED, OUTPUT );
  pinMode( LowLED, OUTPUT );
  pinMode( tideTriggerPin, OUTPUT );
  pinMode( rayTriggerPin, OUTPUT );

  sensors.begin();
  
  //set started to false for first loop
  started = false;
  
  // these variables will be used later to change whether the tide pool if filling or emptying
  LowTide = digitalRead(InLow);
  HighTide = digitalRead(InHigh);
  
  //attaches tideServo to pin 5
  tideServo.attach(5);
  
  //attaches rayServo to pin 11
  rayServo.attach(11); 
  
  //run test
  test();
  
  //set to low tide
  setLowTide();
  
  //Can also be set to High at start using setHighTide(); default is low
  //setHighTide();

  //set tide to Idle
  //tideIdle();
}

// Main program
void loop() {
  
  //logTimer();
  sendLog();
  
  // Read the current time... everything is testing time intervals.
  currentTime = millis();
  
  lcd.clear();
  lcd.print("mills set");
  delay(1000);
  
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
  
  lcd.clear();
  lcd.print("checkedState");
  delay(1000);
  
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
  
  lcd.clear();
  lcd.print("setVals");
  delay(1000);

  /*
      Need to wait until the High or Low tide time interval has been met. Then start a pump.
      We want to cycle the correct pump on and off with the cycle time.
      PumpFill fills the pool
      PumpDrain empties the pool
  */

  //call runCycle
  runCycle();
  lcd.clear();
  lcd.print("ranCycle");
  delay(1000);
  
  // set stateString
  //setStateSring();--------------------
}
// End of loop()
