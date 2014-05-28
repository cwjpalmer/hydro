
  /*
  **************Billie's Hydroponic Controller V1.0.1***************
  **************-------------------------------------***************
  ****Made by Tom De Bie for anyone who can find a use for it ;)****
  ******************************************************************
  ******************************************************************

  */

  // ************************ Definitions **************************
  // Libraries
  #include <SD.h>                            //SD card library
  #include <Wire.h>                          //One Wire library
  #include "RTClib.h"                        //Real Time Clock library
  #include <OneWire.h>                       //OneWire library, for liquid temperature sensor
  #include "DHT.h"                           //DHT library for DHT22 sensor
  // not in use: commented out, and it seems to run fine
    //#include "SPI.h"
    //#include <EEPROMex.h>                      //Extended Eeprom library


  // Pin Definitions
  #define DHTPIN             34                // pin for DHT22
  #define redLEDpin          2                 // LEDs on SD card
//#define greenLEDpin        3                 // LEDs on SD Card                  // not in use
  #define DS18S20_Pin        28                // DS18S20 Signal pin on digital 2
  #define pHPin              A7                // pin for pH probe
  #define pHPlusPin          45                // pin for Base pump (relay)
  #define pHMinPin           46                // pin for Acide pump (relay)
  #define lightSensor        A8                // pin for Photoresistor
  #define solenoidPin        49                // digital pin
  #define LED_SOLENOID_PIN   38                // LED
  #define LED_LIQ_PIN        40                // LED

/*for AS: ideas on simulating float switches
  delete if not useful / needed :-)

  float switches: you can use a simple breadboard switch or button
  or fake a switch:
    wire breadboard as if you're setting up a switch (don't forget the pull down resistor)
    connect a jumper cable in place of the switch (fake switch on)
    disconnect jumper cable (fake switch off)

  -CP
*/
  #define LowestFloatPin     32
  #define MiddleFloatPin     33
  #define HighestFloatPin    34

  //  Setpoints
  float Setpoint=4.8;                    //holds value for Setpoint


  //  Custom Definitions & Variables
  #define DHTTYPE DHT22                      //DHT 22  (AM2302)
  OneWire ds(DS18S20_Pin);  //Temperature chip i/o
  DHT dht(DHTPIN, DHTTYPE);                  // create a DHT type object called dht
  float h;                                   //humidity
  float t;                                   //temperature
  char filename[] = "LOGGER00.CSV";          //filename for CSV file
  File logfile;                               //indicate CSV file exists
  RTC_DS1307 RTC;                            //Define RTC module


  //  Other Variable Initialization
//int x, y;                    // not in use
//int page = 0;                // not in use
  int tankProgState = 0;
  int manualRefilState = 0;
  float pH;                          //generates the value of pH
//int pmem = 0;                      //check which page you're on    // I think we should get rid of this variable... it doesn't seem to be doing anything. -CP
  float HysterisMin;
  float HysterisPlus;
  float SetHysteris = 0.1;
  int lightADCReading;
  double currentLightInLux;
  double lightInputVoltage;
  double lightResistance;
  int EepromSetpoint = 10;      //location of Setpoint in Eeprom
  int EepromSetHysteris = 20;   //location of SetHysteris in Eeprom
  float liqTemperatureOutput;
//  byte bGlobalErr;              //for passing error code back.      // not in use (only appeaers here) => commented out   -CP
  bool fillingNow = false;
  bool overFilledError = false;


  //  Looped Functions

  // ********************* Setup Functions ***********************

  //  Non-Volatile Arduino Memory
  void EepromRead() {
      //Setpoint = EEPROM.readFloat(EepromSetpoint);
      //SetHysteris = EEPROM.readFloat(EepromSetHysteris);
    }

  //  Configuration of pins
  void logicSetup() {

    pinMode(pHPlusPin, OUTPUT);
    pinMode(pHMinPin, OUTPUT);
    pinMode(solenoidPin, OUTPUT);
    pinMode(LED_SOLENOID_PIN, OUTPUT);
    pinMode (LED_LIQ_PIN, OUTPUT);
    pinMode(10, OUTPUT);                    // I think this is the SD pin -CP
    pinMode(redLEDpin, OUTPUT);
  //pinMode(greenLEDpin, OUTPUT);           // not in use

    /*pmem==0;*/

    delay(300);
    Serial.begin(9600);
    Serial.println();
    Serial.println("System Booting!");
    Serial.println("__________________________________\n\n");
    delay(700);

  }

/*                      // commented out for testing without SD card reader
  void SDSetup() {
    // initialize the SD card
    Serial.print("Initializing SD card...");

    if (!SD.begin(10, 11, 12, 13)) {
        error("Card failed, or not present");
      }
      Serial.println("card initialized.");

      // create a new file
      for (uint8_t i = 0; i < 100; i++) {
        filename[6] = i/10 + '0';
        filename[7] = i%10 + '0';
        if (! SD.exists(filename)) {
          // only open a new file if it doesn't exist
          logfile = SD.open(filename, FILE_WRITE);
          break;  // leave the loop!
        }
      }

      if (! logfile) {
        error("couldnt create file");
      }

      Serial.print("Logging to: ");
      Serial.println(filename);

     File dataFile = SD.open(filename, FILE_WRITE);
     dataFile.print("pH,temp,humidity,light,date");
  }
*/

/*
  ***** LIQUID LEVEL FUNCTION BLOCK *****
      *  contains setup & loop functions
      *  uses float switches
      *  loop function checks if liquid level is too low
      *  returns bool; true = too low, false = not too low = acceptable level
*/
  void TankShouldFillSetup() {
    pinMode(LowestFloatPin,INPUT);
    pinMode(MiddleFloatPin,INPUT);
    pinMode(HighestFloatPin,INPUT);
  }
  //  Liquid Level Sensor Function
  bool TankShouldFill() {
    bool result;

    bool lowestFloatSubmerged = false;
    bool middleFloatSubmerged = false;
    bool highestFloatSubmerged = false;

    lowestFloatSubmerged = digitalRead(LowestFloatPin);
    middleFloatSubmerged = digitalRead(MiddleFloatPin);
    highestFloatSubmerged = digitalRead(HighestFloatPin);

    if (! lowestFloatSubmerged) {    // this else-if statement is overkill, but I included redundant statements for clarity of behaviour -CP
      // fill
      result = true;
    } else if (fillingNow && ! middleFloatSubmerged) {
      // fill
      result = true;
    } else if (middleFloatSubmerged) {
      // stop filling
      result = false;
    } else {
      // nothing
    }

    if (highestFloatSubmerged) {
      overFilledError = true;
      result = false;
    } else {
      overFilledError = false;
    }

    return result;
  }


  void logicLoop() {


  h = dht.readHumidity();
  t = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" % RH\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" °C");
    }
    /*                        [ ] CP CHANGE BACK
    float pHSensorValue = 0;
    pHSensorValue = analogRead(pHPin);
    pH = (0.0178 * pHSensorValue - 1.889);
    */

    pH = 6;  // CP KILL THIS - its BS BS BS

    HysterisMin = (Setpoint - SetHysteris);
    HysterisPlus = (Setpoint + SetHysteris);

    // - SERIES OF IF STATEMENTS TO CHANGE CONTROL VARIABLES BASED ON SYSTEM STATE -
    if (pH >= HysterisMin && pH <= HysterisPlus) {
      // pH is above lower limit and below upper limit
      // add nothing
      digitalWrite(pHMinPin,LOW);
      digitalWrite(pHPlusPin,LOW);
      Serial.println("pH in acceptable range");
    } else if (pH < HysterisMin ) {
      // make sure pH down solution not being added
      digitalWrite(pHMinPin,LOW);
      // add a dose of pH plus soution
      digitalWrite(pHPlusPin,HIGH);
      delay(300);
      digitalWrite(pHPlusPin,LOW);
      Serial.println("pH too low. Adjusting ...");
    } else if (pH > HysterisPlus) {
      // make sure pH up solution not being added
      digitalWrite(pHPlusPin,LOW);
      // add a dose of pH down solution
      digitalWrite(pHMinPin,HIGH);
      delay(300);
      digitalWrite(pHMinPin,LOW);
      Serial.println("pH too high. Adjusting ...");
    } else {
      // feedback saying pH is fucked
      digitalWrite(pHMinPin,LOW);
      digitalWrite(pHPlusPin,LOW);
      Serial.println("Unexpected pH reading. Check pH sensor, verify reading. If this message persists, monitor & control pH manually using handheld sensor & pH up & pH down solutions.");
    }

    Serial.print("pH Setpoint = ");
    Serial.println(Setpoint);
    Serial.print("pH Hysteris = ");
    Serial.println(SetHysteris);
    Serial.print("pH = ");
    Serial.println(pH);

    Serial.print("Liquid Temperature: ");
    Serial.print(getTemp());
    Serial.println(" °C");

  }




  //  Determine amount of light, in lux
  void lightLoop() {
    lightADCReading = analogRead(lightSensor);
    // Calculating the voltage of the Analog to Digital Converter ADC for light
    lightInputVoltage = 5.0 * ((double)lightADCReading / 1024.0);
    // Calculating the resistance of the photoresistor in the voltage divider
    lightResistance = (10.0 * 5.0) / lightInputVoltage - 10.0;
    // Calculating the intensity of light in lux
    currentLightInLux = 255.84 * pow(lightResistance, -10/9);
    Serial.print("Light level = ");
    Serial.println(currentLightInLux);
  }



  //  pH & Hysteris Controls
  void phIncreaseSetpoint() {
    Setpoint = Setpoint + 0.10;
    if (Setpoint >= 9.00) {
      Setpoint = 9.00;
    }
  }
  void phDecreaseSetpoint() {
    Setpoint = Setpoint - 0.10;
    if (Setpoint <= 3.00) {
      Setpoint = 3.00;
    }
  }
  void phIncreaseHysteris() {
    SetHysteris = SetHysteris + 0.01;
    if (SetHysteris >= 9.00) {
      SetHysteris = 9.00;
    }
  }
  void phDecreaseHysteris() {
    SetHysteris = SetHysteris - 0.01;
    if (SetHysteris <= 0.01) {
      SetHysteris = 0.01;
    }
  }


  void TankLevelControlLoop () {
    if (TankShouldFill()) {
        //digitalWrite(solenoidPin, HIGH);  //open solenoid valve
      digitalWrite(LED_SOLENOID_PIN, HIGH);    // LED representation
      fillingNow = true;
    } else {
      //digitalWrite(solenoidPin, LOW);     //close solenoid valve
      digitalWrite(LED_SOLENOID_PIN, LOW);    // LED representation
      fillingNow = false;
    }

    if (overFilledError) {
      Serial.println("Nutrient tank is overfilled and may overflow. Check tank & related hardware, esp. fill solenoid & feeding pump.");
    } else if (fillingNow) {
      Serial.println("Nutrient tank is filling.");
    } else {
      Serial.println("Nutrient tank level is within acceptable range.");
    }
  }


  //  Liquid Water Temperature
  float getTemp(){

    byte data[12];
    byte addr[8];

    if ( !ds.search(addr)) {
        //no more sensors on chain, reset search
        ds.reset_search();
        return -1000;
    }

    if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.println("CRC is not valid!");
        return -1000;
    }

    if ( addr[0] != 0x10 && addr[0] != 0x28) {
        Serial.print("Device is not recognized");
        return -1000;
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44,1); // start conversion, with parasite power on at the end

    byte present = ds.reset();
    ds.select(addr);
    ds.write(0xBE); // Read Scratchpad


    for (int i = 0; i < 9; i++) { // we need 9 bytes
      data[i] = ds.read();
    }

    ds.reset_search();

    byte MSB = data[1];
    byte LSB = data[0];

    float tempRead = ((MSB << 8) | LSB); //using two's compliment
    float TemperatureSum = tempRead / 16;
    if (TemperatureSum > 35) {
      digitalWrite (LED_LIQ_PIN, HIGH);
    }
    else {
      digitalWrite (LED_LIQ_PIN, LOW);
    }
    liqTemperatureOutput = TemperatureSum;
    return TemperatureSum;

  }


  //  Manual Refill via Open Solenoid Valve
  void ManualRefilProg() {
    digitalWrite(solenoidPin, HIGH);
  }


  //  Error Function Associated with SD Card
  void error(char *str)
  {
    Serial.print("error: ");
    Serial.println(str);

    // red LED indicates error
    digitalWrite(redLEDpin, HIGH);
    while(1);
  }


  //  Print Data to SD Card
  void SDLoop()
  {
    File dataFile = SD.open(filename, FILE_WRITE);

    DateTime now;
    now = RTC.now();

    if (dataFile) {
      now = RTC.now();
      dataFile.print(pH);
      dataFile.print(", ");
      dataFile.print(t, DEC);
      dataFile.print(", ");
      dataFile.print(h, DEC);
      dataFile.print(", ");
      dataFile.print(currentLightInLux);
      dataFile.print(", ");
      dataFile.print(liqTemperatureOutput);
      dataFile.println();
      dataFile.close();
    }
    else {
      Serial.println("error copying data to CSV");
    }
  }


  void timeSetup()
  {
      Wire.begin();
      if (!RTC.begin()) {
      logfile.println("RTC failed");
      Serial.println("RTC failed");
      }
  }


  // Serial Commands; to be Replaced
  void followSerialCommand() {
    if (Serial.available() > 0) {
      int incomingByte = Serial.read();
        switch (incomingByte) {
          case '1':
            phIncreaseSetpoint();                         // pH inc set point
            break;
          case '2':
            phDecreaseSetpoint();                         // pH dec set point
            break;
          default:
            Serial.println('Invalid input. Enter 1-9');
            break;
        }
    }
  }

  // *********************** Main Loops **************************
  void setup() {
    EepromRead();             //  pull values for Setpoint, SetHysteris, from eeprom
    dht.begin();              //
    logicSetup();             //  set some pinmodes and begin serial comms
    timeSetup();              //  start wire and RTC ... not sure what this means specifically, but it gets the clock tickin'
    //SDSetup();                //  setup SD card, report if card is missing
    TankShouldFillSetup();
  }

   void loop() {
     logicLoop();             //  change control variables based on system state, serial print process variables
     lightLoop();             //  calculate and serial print light level
     TankLevelControlLoop();       //  f
     //SDLoop();                //  f
     followSerialCommand();   // respond to serial input
     Serial.println();
     delay(4000);
   }
