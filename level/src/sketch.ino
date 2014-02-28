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

10 -> empty calibration
11 -> full calibration
12 -> calculate slope

*/





float liqLevelsensorValue = 0;                      // variable to store the value coming from the sensor // this was initially an int
float liqLevelrefValue = 0;                         // variable to store the value coming from the reference resistor // this was omitted as this code does not compensate for temperature
float liqLevelcalEmptyValue = 0;                    // variable to store the raw value yielded by empty calibration 
float liqLevelcalFullValue = 0;                     // variable to store the raw value yielded by full calibration 
float liqLevelslope = 0;                            // variable to store the calculated value of the slope, for liq level calc
float liqLevelReading = 0;                          // variable to store liquid level reading, as a percentage
float liqLevel =0;                                  
int liqLevelincomingByte = 0;                       // variable to store an incoming byte from serial communications  
int liqLevelsensorPin = A4;                         // select the input pin for the potentiometer that responds to liquid level
int liqLevelrefPin = A5;                            // signal pin for reference resistor
int ledPin = 13;                                    // select the pin for the LED

int incomingByte = 10;

void setup() 
{
  pinMode(ledPin, OUTPUT);                  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);                       // begin serial communications at BAUD=9600

}

void loop() 
{


  liqLevelsensorValue = analogRead(liqLevelsensorPin);      // read the value from the sensor
  liqLevelrefValue = analogRead(liqLevelrefPin);            // read the value from the reference resistor
 
  //  this block of code lights the LED, with duration & delay proportional sensor reading for visual feedback
  // omit for use in control system
  digitalWrite(ledPin, HIGH);               // turn the ledPin on
  Serial.println(liqLevelsensorValue);              // stop the program for <sensorValue> milliseconds:
  delay(liqLevelsensorValue);                       // wait for an amount of time proportional to the sensor
  digitalWrite(ledPin, LOW);                // turn the ledPin off:       
  delay(liqLevelsensorValue);                       // stop the program for for <sensorValue> milliseconds:


  if (Serial.available() > 0) 
  {
    incomingByte = Serial.read(); 
     switch (incomingByte) 
     {
    case '10':    
      liqLevelcalibrateEmpty(liqLevelsensorValue);
      break;
    case '11':    
      liqLevelcalibrateFull(liqLevelsensorValue);
      break;
    case '12':    
      liqLevellinearFitSlope(liqLevelsensorValue, liqLevelcalFullValue);
      break;
    default:
      Serial.println('Invalid input. Enter 1 for empty calibration, 2 for full calibration, or 3 to calculate slope'); 
  }

  liqLevelReading = liqLevelCalc(liqLevelsensorValue, liqLevelcalFullValue, liqLevelslope);                // run liqLevelCalc() on delay input
  delay(1000);                              // wait 1 s
 }

} 


// function to take a calibration value for the sensor when the liquid level is in air
float liqLevelcalibrateEmpty  (float liqLevelsensorValue) 
{
  liqLevelcalEmptyValue = analogRead(liqLevelsensorValue);
  Serial.print("Empty Calibration Value = ");
  Serial.println(liqLevelcalEmptyValue);
  return liqLevelcalEmptyValue;
}

// function to take a calibration value for the sensor when it is at 100%
float liqLevelcalibrateFull (float liqLevelsensorValue) 
{
  liqLevelcalFullValue = analogRead(liqLevelsensorValue);
  Serial.print("Full Calibration Value = ");
  Serial.println(liqLevelcalFullValue);
  return liqLevelcalFullValue;
}

// function to produce the slope needed for linear fit used to interpolate the liquid level
// must be run AFTER liqLevelcalibrateEmpty() and liqLevelcalibrateFull()
float liqLevellinearFitSlope (float liqLevelsensorValue, float liqLevelcalFullValue) 
{
  float liqLevelslope = 100/(liqLevelcalEmptyValue - liqLevelcalFullValue);
  Serial.print("Slope found = ");
  Serial.println(liqLevelslope);
  return liqLevelslope;
}

// function to determine the liquid level from a sensor value; sensor value is input
float liqLevelCalc (float liqLevelsensorValue, float liqLevelcalFullValue, float liqLevelslope) 
{
	float result = 0;
  result = liqLevelcalFullValue - liqLevelslope * liqLevelsensorValue;
  Serial.print("Liquid level = ");
  Serial.print(result);
  return result;
}

