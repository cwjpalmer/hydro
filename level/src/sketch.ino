/*
----- OPENING COMMENT BLOCK -----

Adapted from example code  
https://www.adafruit.com/products/464#Tutorials

This code works with the milonetech liquid level sensor.
It takes the analog input from the variable resistor on the level sensor, reads it, and prints it to the serial monitor.

*/





// the value of the 'other' resistor
#define SERIESRESISTOR 2250                 // [] it's 2250 ohms +/- 10%, so we should check it with a multimeter and put the correct value here    
 
// What pin to connect the sensor to
#define SENSORPIN 14 

void setup() 
{
  pinMode(ledPin, OUTPUT);                  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);                       // begin serial communications at BAUD=9600
}

void loop() 
{
  float reading;
 
  reading = analogRead(SENSORPIN);
 
  Serial.print("Analog reading "); 
  Serial.println(reading);
 
  // convert the value to resistance
  reading = (1023 / reading)  - 1;
  reading = SERIESRESISTOR / reading;
  Serial.print("Sensor resistance "); 
  Serial.println(reading);
 
  delay(1000);

  digitalWrite(ledPin, HIGH);               // turn the ledPin on
  delay(reading);                       // wait for an amount of time proportional to the sensor
  digitalWrite(ledPin, LOW);                // turn the ledPin off:       
  
  delay(1000);                              // wait 1 s
} 