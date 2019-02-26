/***********************************************************
File name:   Tide_pool_management
Description:  
Author: Joshua DeNio
Date: 2/25/2019
***********************************************************/

//high and low float sensors
const int inHigh1 = 1;
const int inLow1 = 2;
const int inHigh2 = 3;
const int inLow2 = 4;

//ultrasonic sensor pins
const int triggerPin = 5;
const int echoPin = 6;

//reference voltage pin
const int InRef  = 7;

//LED to show power on
//const int HighLEDGreen = 8;

//pins to send data to the Raspberry PI
const int out1 = 9;
const int out2 = 10;
const int out3 = 11;

//pins to control solenoids
const int solenoidTrigger1 = 12;
const int selenoidTrigger2 = 13;

const int selenoidTrigger3 = 12;
const int selenoidTrigger4 = 13;

const int selenoidTrigger5 = 8;








//int ledPin=8; //definition digital 8 pins as pin to control the LED
void setup()
{
    pinMode(ledPin,OUTPUT);    //Set the digital 8 port mode, OUTPUT: Output mode
}
void loop()
{  
    //digitalWrite(ledPin,HIGH); //HIGH is set to about 5V PIN8
    //delay(1000);               //Set the delay time, 1000 = 1S
    //digitalWrite(ledPin,LOW);  //LOW is set to about 5V PIN8
    //delay(1000);               //Set the delay time, 1000 = 1S
} 
