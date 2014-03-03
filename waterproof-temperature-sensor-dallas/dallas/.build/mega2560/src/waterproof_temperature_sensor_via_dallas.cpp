#include <Arduino.h>
#include <OneWire.h>                       //OneWire library, for liquid temperature sensor
#include <DallasTemperature.h>             //Library for Dallas Temperature that may or may not be required for liquid temperature sensor 
void setup(void);
void loop ();
#line 1 "src/waterproof_temperature_sensor_via_dallas.ino"
// this is not an arduino sketch, but rather code that can be included to add a function to a sketch
// the function liquidTemperatureRead determines the temperature using a one-wire temperature sensor

//#include <OneWire.h>                       //OneWire library, for liquid temperature sensor
//#include <DallasTemperature.h>             //Library for Dallas Temperature that may or may not be required for liquid temperature sensor 


#define ONE_WIRE_BUS 28      // pin for temperature sensor

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// put this somewhere in: void setup(){}
float temp;


void setup(void)
{
pinMode (ONE_WIRE_BUS,OUTPUT);
// start serial port
Serial.begin(9600);
Serial.println("Dallas Temperature IC Control Library Demo");
 
// Start up the library
sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
}



//add this function and all it when you need a temperature reading

void loop () {
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(1000);
  Serial.println("DONE");
  Serial.println(sensors);
  Serial.print("Liquid temperature is: ");
  temp = sensors.getTempCByIndex(0);
  Serial.println(temp);
  
}

