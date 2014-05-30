
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
  #include <EEPROMex.h>                      //Extended Eeprom library
  #include <EEPROMVar.h>                     //EEPROM variable lib (comes with EEPROMex)
  #include "SPI.h"



  // Pin Definitions
  #define DHTPIN             34                // pin for DHT22 (interior)
  #define DHTPIN2 			 35				   // pin for DHT22 (exterior)
  #define redLEDpin          2                 // LEDs on SD card
//#define greenLEDpin        3                 // LEDs on SD Card                  // not in use
  #define DS18S20_Pin        28                // DS18S20 Signal pin on digital 2
  #define pHPin              A7                // pin for pH probe
  #define pHPlusPin          38                // pin for Base pump (relay)
  #define pHMinPin           40                // pin for Acide pump (relay)
  #define lightSensor        53                // pin for Photoresistor
  #define solenoidPin        49                // digital pin
  #define LED_SOLENOID_PIN   30                // LED
  #define LED_LIQ_PIN        31                // LED

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
  float pHSetpoint=4.8;                      //holds value for pH Setpoint


  //  Custom Definitions & Variables
  #define DHTTYPE DHT22                      //DHT 22  (AM2302)
  OneWire ds(DS18S20_Pin);                   //Temperature chip i/o
  DHT dht(DHTPIN, DHTTYPE);                  // create a DHT object called dht (interior)
  DHT dht2 (DHTPIN2, DHTTYPE);				 // create a DHT object called dht2 (exterior)
  float h;                                   //humidity (interior)
  float t;                                   //temperature (interior)
  float h2;									 //humidity (exterior)
  float t2;									 //temperature (exterior)
  char filename[] = "LOGGER00.CSV";          //filename for CSV file
  File logfile;                              //indicate CSV file exists
  RTC_DS1307 RTC;                            //Define RTC module
  EEPROMClassEx EEPROMex;                    //create an EPPROMClassEx object called EEPROMex


  //  Other Variable Initialization
//int x, y;                    // not in use
//int page = 0;                // not in use
  int tankProgState = 0;
  int manualRefilState = 0;
  float pH;                          //generates the value of pH
//int pmem = 0;                      //check which page you're on    // I think we should get rid of this variable... it doesn't seem to be doing anything. -CP
  float HysteresisMin;
  float HysteresisPlus;
  float SetHysteresis = 0.1;
  int lightADCReading;
  double currentLightInLux;
  double lightInputVoltage;
  double lightResistance;
  int EepromAddresspHSetpoint = 10;      //location of pHSetpoint in Eeprom
  int EepromAddressSetHysteresis = 20;   //location of SetHysteresis in Eeprom
  float liqTemperatureOutput;
//  byte bGlobalErr;              //for passing error code back.      // not in use (only appeaers here) => commented out   -CP
  bool fillingNow = false;
  bool overFilledError = false;

/*
  ***** EEPROM BLOCK *****
      *  goal: store setpoints to flash memory so that system recovers to same state upon reset
      *  read function to pull values (runs on startup)
      *  update function saves key (runs once per loop)
      todos
      [] create SD read & update functions
      [] only call EEPROM functions if no SD card is present
*/
  void EepromRead() {
      // reading from EEPROM doesn't degrade it, so we can leave this stuff active all the time
      pHSetpoint = EEPROM.readFloat(EepromAddresspHSetpoint);
      SetHysteresis = EEPROM.readFloat(EepromAddressSetHysteresis);
  }
  void EepromUpdate() {
    // write key values to EEPROM, only changing bytes for which the current value differs from the EEPROM value
    // this slightly degrades the EEPROM, but only when the setpoints change
    EEPROMex.updateFloat(EepromAddresspHSetpoint, pHSetpoint);
    EEPROMex.updateFloat(EepromAddressSetHysteresis, SetHysteresis);
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

/*
  ***** DATA LOGGING (SD & RTC) BLOCK *****
      *  SD setup, SD loop, RTC setup functions
      *  for data logging
      Tasks
      [] separate SD & RTC functions into two blocks
*/
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
    } else {
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

/*
  ***** CONTROL LOGIC BLOCK *****
      *  includes reading values & process control
      * DHT, liquid T (1wire), pH
      Tasks
      [] Separate out for different probes
        [] DHT
        [] liquid T
        [] pH
      [] include setup functions
*/
void logicLoop() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  h2 = dht2.readHumidity();
  t2 = dht2.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT (interior)");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" % RH\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" °C");
    }

  if (isnan(t2) || isnan(h2)) {
    Serial.println("Failed to read from DHT (exterior)");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" % RH\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" °C");
    }  

    float pHSensorValue = 0;
    pHSensorValue = analogRead(pHPin);
    pH = (0.0178 * pHSensorValue - 1.889);


    HysteresisMin = (pHSetpoint - SetHysteresis);
    HysteresisPlus = (pHSetpoint + SetHysteresis);


    // - SERIES OF IF STATEMENTS TO CHANGE CONTROL VARIABLES BASED ON SYSTEM STATE -
    if (pH >= HysteresisMin && pH <= HysteresisPlus) {
      // add nothing
      digitalWrite(pHMinPin,LOW);
      digitalWrite(pHPlusPin,LOW);
      Serial.println("pH in acceptable range");
    } else if (pH < HysteresisMin ) {
      // make sure pH down solution not being added
      digitalWrite(pHMinPin,LOW);
      // add a dose of pH plus soution
      digitalWrite(pHPlusPin,HIGH);
      delay(300);
      digitalWrite(pHPlusPin,LOW);
      Serial.println("pH too low. Adjusting ...");
    } else if (pH > HysteresisPlus) {
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

    Serial.print("pH setpoint = ");
    Serial.println(pHSetpoint);
    Serial.print("pH range is setpoint plus or minus: ");
    Serial.println(SetHysteresis);
    Serial.print("current pH = ");
    Serial.println(pH);

    Serial.print("Current liquid temperature = ");
    Serial.print(getTemp());
    Serial.println(" C");

}

/*
  ***** LIGHT BLOCK *****
      * only reads (no process control)
      * set up for one sensor
      Tasks
      [] adapt to scale for multiple sensors
      [] include setup
*/
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
    Serial.print(currentLightInLux);
    Serial.println(" lux");
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

/*
  ***** SEMANTIC SERIAL COMMAND BLOCK *****
      *  only one function
      *  takes string input from serial comms
      *  parses command
      *  modifies system variables based on command
      todos
      [] move 'help' to its own function
      [] move generic error message to its own function
*/
  void followSerialCommand() {
    // receive a command as a character array (not a String object)
    String command;

    if (Serial.available()) {
      command = Serial.readString();

      char commandForParsing[32];
      command.toCharArray(commandForParsing,32);

      // parse the command into pieces using strtok (read: string token)
      char *i;

      char* goal = strtok_r(commandForParsing," ",&i);
      char* variable = strtok_r(NULL," ",&i);
      char* value = strtok_r(NULL," ",&i);
      float floatValue = atof(value);


      // interpret the pieces of the command using strcmp (read: string compare)
        if (String(goal).equalsIgnoreCase("help")) {
          // display a walkthrough on giving commands

          Serial.println();
          Serial.println();
          Serial.println("*****************************************************");
          Serial.println("So, you need help, eh?");
          Serial.println("Here's some info on giving commands to the arduino over serial communications (that's what you're doing now).");
          Serial.println();
          Serial.println("Enter a goal, variable and a value, all separated by spaces. Then hit enter. Note: this interface is not case sensitive.");
          Serial.println();
            Serial.println("Possible goals are:");
              Serial.println("     help");
              Serial.println("     set");
              Serial.println("     inc");
              Serial.println("     increase");
              Serial.println("     dec");
              Serial.println("     decrease");
              Serial.println();
            Serial.println("Possible variables are:");
              Serial.println("     phSet     (the setpoint for pH)");
              Serial.println("     phHysteresis     (this is the 'plus or minus' value for the pH setpoint)");
              Serial.println();
            Serial.println("Values are decimal values, such as 4.81 or 5");
            Serial.println("Here are some example commands");
            Serial.println();
              Serial.println("set phset 5.61     (changes the pH set point to 5.61)");
              Serial.println("INC PHhysteresis 0.15     (increases the hysteresis on pH by 0.15)");
              Serial.println("dec phset     (this will decrease the ph setpoint by the default value, 0.1)");
              Serial.println();
          Serial.println();
          Serial.println("That's all the help I have built in. For more detailed info, check out the source code or get in touch with the coders.");
          Serial.println();
          Serial.println("*****************************************************");
        } else if (String(goal).equalsIgnoreCase("set")) {  // if the goal is set
          if (floatValue==0) {  // check to make sure that the value supplied is a number
            Serial.println("Value given was not a number. Check command & try again. Give command 'help' for more info.");
          } else {  // if value is a number:
              // set hysteresis value
              if (String(variable).equalsIgnoreCase("phHysteresis")) {
                SetHysteresis = floatValue;
              // set ph set value
              } else if (String(variable).equalsIgnoreCase("phSet")) {
                pHSetpoint = floatValue;
              } else {
                Serial.print("Command not recognized. Check command & try again. Give command 'help' for more info.");
              }
          }
        } else if (String(goal).equalsIgnoreCase("inc")) {  // if the goal is increase
            if (String(variable).equalsIgnoreCase("phHysteresis")) {
              phIncreaseHysteresis();
            } else if (String(variable).equalsIgnoreCase("phSet")) {
              phIncreaseSetpoint(floatValue);
            } else {
              Serial.print("Command not recognized. Check command & try again. Give command 'help' for more info.");
            }
        } else if (String(goal).equalsIgnoreCase("increase")) {  // if the goal is increase
            if (String(variable).equalsIgnoreCase("phHysteresis")) {
              phIncreaseHysteresis();
            } else if (String(variable).equalsIgnoreCase("phSet")) {
              phIncreaseSetpoint(floatValue);
            } else {
              Serial.print("Command not recognized. Check command & try again. Give command 'help' for more info.");
            }
        } else if (String(goal).equalsIgnoreCase("dec") || String(goal).equalsIgnoreCase("decrease")){  // if the goal is decrease
            if (String(variable).equalsIgnoreCase("phHysteresis")) {
              phDecreaseHysteresis();
            } else if (String(variable).equalsIgnoreCase("phSet")) {
              phDecreaseSetpoint();
            } else {
              Serial.print("Command not recognized. Check command & try again. Give command 'help' for more info.");
            }
        } else {
          Serial.print("Command not recognized. Check command & try again. Give command 'help' for more info.");
        }
    }
  }


/*
  ***** VARIABLE INCREMENT BLOCK  *****
      * functions to increase or decrease setpoints
      * effectively support for serial command functions
      Tasks
      [] adapt to be more generic
        [] eliminate repeated, nearly identical code
*/
  //  pH & Hysteresis Controls
  void phIncreaseSetpoint(float amount) {
    if (amount) {
      pHSetpoint = pHSetpoint + 0.10;
      if (pHSetpoint >= 9.00) {
        pHSetpoint = 9.00;
      }
    }
    else {
      pHSetpoint = pHSetpoint + amount;
      if (pHSetpoint >= 9.00) {
        pHSetpoint = 9.00;
      }
    }
  }
  void phDecreaseSetpoint() {
    pHSetpoint = pHSetpoint - 0.10;
    if (pHSetpoint <= 3.00) {
      pHSetpoint = 3.00;
    }
  }
  void phIncreaseHysteresis() {
    SetHysteresis = SetHysteresis + 0.01;
    if (SetHysteresis >= 9.00) {
      SetHysteresis = 9.00;
    }
  }
  void phDecreaseHysteresis() {
    SetHysteresis = SetHysteresis - 0.01;
    if (SetHysteresis <= 0.01) {
      SetHysteresis = 0.01;
    }
  }


  // *********************** Main Loops **************************
  void setup() {
    //EepromRead();             //  pull values for pHSetpoint, SetHysteresis, from eeprom
    dht.begin();              //
    logicSetup();             //  set some pinmodes and begin serial comms
    timeSetup();              //  start wire and RTC ... not sure what this means specifically, but it gets the clock tickin'
    SDSetup();                //  setup SD card, report if card is missing
    //TankShouldFillSetup();
  }

   void loop() {
     logicLoop();             //  change control variables based on system state, serial print process variables
     lightLoop();             //  calculate and serial print light level
     //TankLevelControlLoop();       //  f
     SDLoop();                //  f
     followSerialCommand();   // respond to serial input
     //EepromUpdate();
     Serial.println();
     delay(30000);
   }
