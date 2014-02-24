#include <Arduino.h>

void setup();
void loop();
float calibrateEmpty  (float sensorValue);
float calibrateFull (float sensorValue);
float linearFitSlope (float sensorValue, float calFullValue, float slope);
float liqLevelCalc (float sensorValue, float calFullValue, float slope);
#line 1 "src/sketch.ino"
/*
----- OPENING COMMENT BLOCK -----

Adapted from 'analog input' : 
 http://arduino.cc/en/Tutorial/AnalogInput
 
This code works with the milonetech liquid level sensor.
It takes the analog input from the variable resistor on the level sensor, reads it, and prints it to the serial monitor.


The end goal of this code in the hydroponics project is to determine the level in a liquid reservoir.
Steps to accomplish this:
[x] 1. take analog input from variable
[x] 2. understand what 'empty' and 'full' look like as an analog input 
[x] 3. produce a function for liquid level as f(variable resistance analog value, reference resistance analog value, reference inputs at 'empty' and 'full')
[x] 4. add calibration instructions to this file

NB: The liquid level sensor contains two resistors. One is variable, and one is for reference. The reference resistor can be used to compensate for the effect of temperature on the liquid level reading from the variable resistor. This is deemed non-crucial and omitted from this code. This functionality may be required for accurate readings in the field, and should be added if that is the case.

Application: 
- connect it to the data logger
- let it control a solenoid, or alert the operator to add more water to the system
 */


/*
----- OPERATOR INSTRUCTIONS -----
Instructions are sent to the arduino through the serial connection, as bytes.

1 -> empty calibration
2 -> full calibration
3 -> calculate slope

*/





float sensorValue = 0;                      // variable to store the value coming from the sensor // this was initially an int
float refValue = 0;                       // variable to store the value coming from the reference resistor // this was omitted as this code does not compensate for temperature
float calEmptyValue = 0;                    // variable to store the raw value yielded by empty calibration 
float calFullValue = 0;                     // variable to store the raw value yielded by full calibration 
float slope = 0;                            // variable to store the calculated value of the slope, for liq level calc
float liqLevelReading = 0;                  // variable to store liquid level reading, as a percentage
float liqLevel =0;
int incomingByte = 0;                       // variable to store an incoming byte from serial communications  
int sensorPin = A4;                         // select the input pin for the potentiometer that responds to liquid level
int refPin = A5;                            // signal pin for reference resistor
int ledPin = 13;                            // select the pin for the LED


void setup() 
{
  pinMode(ledPin, OUTPUT);                  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);                       // begin serial communications at BAUD=9600

}

void loop() 
{


  sensorValue = analogRead(sensorPin);      // read the value from the sensor
  refValue = analogRead(refPin);            // read the value from the reference resistor
 
  //  this block of code lights the LED, with duration & delay proportional sensor reading for visual feedback
  digitalWrite(ledPin, HIGH);               // turn the ledPin on
  Serial.println(sensorValue);              // stop the program for <sensorValue> milliseconds:
  delay(sensorValue);                       // wait for an amount of time proportional to the sensor
  digitalWrite(ledPin, LOW);                // turn the ledPin off:       
  delay(sensorValue);                       // stop the program for for <sensorValue> milliseconds:


  if (Serial.available() > 0) 
  {
    incomingByte = Serial.read(); 
     switch (incomingByte) 
     {
    case '1':    
      calibrateEmpty(sensorValue);
      break;
    case '2':    
      calibrateFull(sensorValue);
      break;
    case '3':    
      linearFitSlope(sensorValue, calFullValue, slope);
      break;
    default:
      Serial.println('Invalid input. Enter 1 for empty calibration, 2 for full calibration, or 3 to calculate slope'); 
  }

  liqLevel = liqLevelCalc(sensorValue, calFullValue, slope);                // run liqLevelCalc() on delay input
  delay(1000);                              // wait 1 s
 }

} 


// function to take a calibration value for the sensor when the liquid level is in air
float calibrateEmpty  (float sensorValue) 
{
  calEmptyValue = analogRead(sensorValue);
  Serial.print("Empty Calibration Value = ");
  Serial.println(calEmptyValue);
  return calEmptyValue;
}

// function to take a calibration value for the sensor when it is at 100%
float calibrateFull (float sensorValue) 
{
  calFullValue = analogRead(sensorValue);
  Serial.print("Full Calibration Value = ");
  Serial.println(calFullValue);
  return calFullValue;
}

// function to produce the slope needed for linear fit used to interpolate the liquid level
// must be run AFTER calibrateEmpty() and calibrateFull()
float linearFitSlope (float sensorValue, float calFullValue, float slope) 
{
  slope = 100/(calEmptyValue - calFullValue);
  Serial.print("Slope found = ");
  Serial.println(slope);
}

// function to determine the liquid level from a sensor value; sensor value is input
float liqLevelCalc (float sensorValue, float calFullValue, float slope) 
{
	float result = 0;
  result = calFullValue - slope * sensorValue;
  Serial.print("Liquid level = ");
  Serial.print(result);
  return result;
}

