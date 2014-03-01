  /* 
  THIS VERSION IS MODIFIED FROM THE ORIGINAL VERSION
  ALL CODE PERTAINING TO THE LCD SCREEN IS BEING COMMENTED OUT
  ALL OTHER CODE IS BEING ANNOTATED TO FACILITATE A STRONGER UNDERSTANDING OF THE FUNCTIONALITY OF THE CODE
  */

    /*
    **************Billie's Hydroponic Controller V1.0.1***************
    **************-------------------------------------***************
    ****Made by Tom De Bie for anyone who can find a use for it ;)****
    ******************************************************************
    ******************************************************************
  
    */

    #include <SD.h>                            //SD card library
    #include <Wire.h>                          //One Wire library
    #include "RTClib.h"                        //Real Time Clock library
    #include <EEPROMex.h>                      //Extended Eeprom library
    #include <OneWire.h>                       //OneWire library, for liquid temperature sensor
    #include "DHT.h"                           //DHT library for DHT22 sensor 
  //#include <DallasTemperature.h>             //Library for Dallas Temperature that may or may not be required for liquid temperature sensor 



    #define DHTPIN 44                          //pin for DHT22                      // [  ] 1 digital input, 10KOhm resistor, 5V
    #define redLEDpin 2                        //LEDs on SD card
    #define greenLEDpin 3                      //LEDs on SD Card 
    #define DHTTYPE DHT22                      // DHT 22  (AM2302)

    #define ECHO_TO_SERIAL   1                 // echo data to serial port
    #define WAIT_TO_START    0                 // Wait for serial input in setup()

    DHT dht(DHTPIN, DHTTYPE);

    int pHPin = A7;                            //pin for pH probe                   // [  ] analog Pin
    int pHPlusPin = 45;                        //pin for Base pump (relay)          // [  ] digital Pin
    int pHMinPin = 46;                         //pin for Acide pump (relay)         // [  ] digital Pin
    int ventilatorPin = 47;                    //pin for Fan (relay)                // [  ] digital Pin
    int floatLowPin = A1;                      //pin for lower float sensor         // [  ] -> level sensor: 2 analog inputs, 10KOhm resistor, 5V 
    int floatHighPin = A2;                     //pin for upper float sensor         // [  ] -> level sensor: 2 analog inputs, 10KOhm resistor, 5V
    int lightSensor = A8;                      //pin for Photoresistor              // [  ]   1 analog input, 10kOhm resistor, 5V
    int sdPin = 10;                            //pin for serial comms with SD card  // [  ] Adafruit uses 'echo data to serial'
    int solenoidPin = 49;                      // digital pin
    float h;
    float t;
  //int liquidTemperaturePin = 2;             // digital pin   LIQTfindMeTag


    File logfile;                               //indicate CSV file exists 
    RTC_DS1307 RTC;                            //Define RTC module

    //****************************************************************//
    //*********Declaring Variables************************************//
    //****************************************************************//
    int x, y;
    int page = 0;
    int tankProgState = 0;
    int manualRefilState = 0;
    float pH;                          //generates the value of pH

    int pmem = 0;                      //check which page you're on
    float Setpoint;                    //holds value for Setpoint
    float HysterisMin;                 
    float HysterisPlus;
    float SetHysteris;
    float FanTemp;
    float FanHumid;
    //float liquidTemperature;           // variable to hold temperature of liquid from waterproof thermometer LIQTfindMeTag

    int lightADCReading;
    double currentLightInLux;
    double lightInputVoltage;
    double lightResistance;

    int EepromSetpoint = 10;      //location of Setpoint in Eeprom
    int EepromSetHysteris = 20;   //location of SetHysteris in Eeprom
    int EepromFanTemp = 40;       //location of FanTemp in Eeprom
    int EepromFanHumid = 60;      //location of FanHumid in Eeprom


    /*LIQUID LEVEL VARIABLES*/
    float liqLevelsensorValue = 0;                      // variable to store the value coming from the sensor // this was initially an int
    float liqLevelrefValue = 0;                         // variable to store the value coming from the reference resistor // this was omitted as this code does not compensate for temperature
    float liqLevelcalEmptyValue = 0;                    // variable to store the raw value yielded by empty calibration 
    float liqLevelcalFullValue = 0;                     // variable to store the raw value yielded by full calibration 
    float liqLevelslope = 0;                            // variable to store the calculated value of the slope, for liq level calc
    float liqLevelReading = 0;                          // variable to store liquid level reading, as a percentage
    float liqLevel =0;                                  
    int liqLevelsensorPin = A4;                         // select the input pin for the potentiometer that responds to liquid level
    int liqLevelrefPin = A5;                            // signal pin for reference resistor
    int ledPin = 13;                                    // select the pin for the LED
    int tankLowSetPoint = 20;                           // variable to store lower limit of tank level   // as a %
    int tankHighSetPoint = 80;                          // variable to store upper limit of tank level   // as a %


    byte bGlobalErr;    //for passing error code back.


    /*
                VOID SETUP AND VOID LOOP MOVED TO END OF DOCUMENT
                functions must be defined before they are called,
                so they had to be at the end
    */
         
         // - DISABLE EEPROM WHEN NOT REQUIRED - specified life of 100k write/erase cycles
         void EepromRead() { // memory whose values are kept when board loses power  - specified life of 100k write/erase cycles
          Setpoint = EEPROM.readFloat(EepromSetpoint);
          SetHysteris = EEPROM.readFloat(EepromSetHysteris);
          FanTemp = EEPROM.read(EepromFanTemp);
          FanHumid = EEPROM.read(EepromFanHumid);
        }


        void logicSetup() {
        pinMode(pHPlusPin, OUTPUT);
        pinMode(pHMinPin, OUTPUT);
        pinMode(ventilatorPin, OUTPUT);
        pinMode(solenoidPin, OUTPUT);
        //OneWire ds(liquidTemperaturePin); //LIQTfindMeTag
         
        pmem==0;
         
        delay(300);
        Serial.println("System Booting!");
        Serial.println("__________________________________\n\n");
        delay(700);
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

        int getLiqLevel() {

          liqLevelsensorValue = analogRead(liqLevelsensorPin);      // read the value from the sensor
          liqLevelrefValue = analogRead(liqLevelrefPin);            // read the value from the reference resistor
          liqLevelReading = liqLevelCalc(liqLevelsensorValue, liqLevelcalFullValue, liqLevelslope);                // run liqLevelCalc() on delay input
          return liqLevelReading;
          }
         


        void logicLoop() { // 

        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
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
                           
          float sensorValue = 0;
          sensorValue = analogRead(pHPin);
          pH = (0.0178 * sensorValue - 1.889);

          HysterisMin = (Setpoint - SetHysteris);
          HysterisPlus = (Setpoint + SetHysteris);
           
          // - SERIES OF IF STATEMENTS TO CHANGE CONTROL VARIABLES BASED ON SYSTEM STATE -
          if (pH == Setpoint) {
            pmem == 0, // why is this a logical operator?
            digitalWrite (pHMinPin, LOW);
            digitalWrite (pHPlusPin, LOW);
          }

          if (pH >= HysterisMin && pH <= HysterisPlus && pmem == 0) {
            digitalWrite (pHMinPin, LOW);
            digitalWrite (pHPlusPin, LOW);
          }
         
          if (pH < HysterisMin && pmem == 0) {
            pmem == 1,
            digitalWrite (pHPlusPin, HIGH);
            digitalWrite (pHMinPin, LOW);
          }
         
          if (pH >= HysterisMin && pH < Setpoint && pmem == 1) {
            digitalWrite (pHPlusPin, HIGH);
            digitalWrite (pHMinPin, LOW);
          }
         
          if (pH > HysterisPlus && pmem == 0) {
            pmem ==2,
            digitalWrite (pHMinPin, HIGH);
            digitalWrite (pHPlusPin, LOW);
          }
         
          if (pH <= HysterisPlus && pH > Setpoint && pmem == 2) {
            digitalWrite (pHMinPin, HIGH);
            digitalWrite (pHPlusPin, LOW);
          }

          Serial.print("pmem = ");
          Serial.println(pmem);
          Serial.print("Setpoint = ");
          Serial.println(Setpoint);
          Serial.print("Hysteris = ");
          Serial.println(SetHysteris);       
          Serial.print("pH = ");
          Serial.println(pH);

                 
        }
                 
         



        void fotoLoop() {
          if (page == 0) {
          lightADCReading = analogRead(lightSensor);
          // Calculating the voltage of the Analog to Digital Converter ADC for light
          lightInputVoltage = 5.0 * ((double)lightADCReading / 1024.0);
          // Calculating the resistance of the photoresistor in the voltage divider
          lightResistance = (10.0 * 5.0) / lightInputVoltage - 10.0;
          // Calculating the intensity of light in lux       
          currentLightInLux = 255.84 * pow(lightResistance, -10/9);
          //myGLCD.printNumI(currentLightInLux, 91, 162, 4);
          Serial.print("Light level = ");
          Serial.println(currentLightInLux);        
          }
        }




        void phIncreaseSetpoint() {
          Setpoint = Setpoint + 0.01;
          if (Setpoint >= 9.00) {
            Setpoint = 9.00;
          }
          
        }

        void phDecreaseSetpoint() {
          Setpoint = Setpoint - 0.01;
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

        void FanIncreaseTemp() {
          FanTemp = FanTemp + 1;
          if (FanTemp >= 50) {
            FanTemp = 50;
          }
         
        }


        void FanDecreaseTemp() {
          FanTemp = FanTemp - 1;
          if (FanTemp <= 0) {
            FanTemp = 0;
          }
          
        }


        void FanIncreaseHumid() {
          FanHumid = FanHumid + 1;
          if (FanHumid >= 100) {
            FanHumid = 100;
          }
     
        }


        void FanDecreaseHumid() {
          FanHumid = FanHumid - 1;
          if (FanHumid <= 0) {
            FanHumid = 0;
          }
        
        }


        void FanControl() {                                            // control the fan based on DHT sensor T and Humididity
          if ((h >= FanHumid) && (t >= FanTemp)) {   // if himidity is too high and temp us too high, turn fan on - note not 100% sure the DHT[] values are T and H, but logicall they should be
            digitalWrite(ventilatorPin, HIGH);
          }
          else {
            digitalWrite(ventilatorPin, LOW);
          }
        }

                                                                       // [x] rewrite to use level sensor, instead of float switch as input
        void TankProgControl () {
          if (getLiqLevel() < tankLowSetPoint) {
            while (getLiqLevel() < tankHighSetPoint) {
              digitalWrite(solenoidPin, HIGH);  //open solenoid valve
            }
          }
          else {
            digitalWrite(solenoidPin, LOW);     //close solenoid valve
          }
        }

        /*
        int liquidTemperatureRead(){ // changed this function from void to int to allow us to return T reading LIQTfindMeTag

          int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
          byte i;
          byte present = 0;
          byte data[12];
          byte addr[8];

          if ( !ds.search(addr)) { 
              ds.reset_search();
              // return;            // COMMENTED OUT BECAUSE NOT RETURNING ANYTHING, AND WOULDN'T COMPILE WITH THIS LINE INCLUDED
          }

          ds.reset();
          ds.select(addr);
          ds.write(0x44,1);         // start conversion, with parasite power on at the end

          delay(1000);     // maybe 750ms is enough, maybe not
          // we might do a ds.depower() here, but the reset will take care of it.

          present = ds.reset();
          ds.select(addr);    
          ds.write(0xBE);         // Read Scratchpad

         
          Serial.print(" Liquid T = ");
          for ( i = 0; i < 9; i++) {           // we need 9 bytes
            data[i] = ds.read();
          }
          
          LowByte = data[0];
          HighByte = data[1];
          TReading = (HighByte << 8) + LowByte;
          SignBit = TReading & 0x8000;  // test most sig bit
          if (SignBit) // negative
          {
            TReading = (TReading ^ 0xffff) + 1; // 2's comp
          }
          Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

          Whole = Tc_100 / 100;  // separate off the whole and fractional portions
          Fract = Tc_100 % 100;


          if (SignBit) // If its negative
          {
             Serial.print("-");
          }
          Serial.print(Whole);
          Serial.print(".");
          if (Fract < 10)
          {
             Serial.print("0");
          }
          Serial.print(Fract);
          Serial.print("\n");

          return Tc_100;
        }
        */
        
        void ManualRefilProg()                                        // adds liquid to tank from LCD command
        {
          digitalWrite(solenoidPin, HIGH);

        }


        void error(char *str)                                        // error function assocaited with SD card 
        {
          Serial.print("error: ");
          Serial.println(str); 

          // red LED indicates error
          digitalWrite(redLEDpin, HIGH);

          while(1);
        }



        void SDSetup()                                                // set up SD card & RTC
        {

          // initialize the SD card
          Serial.print("Initializing SD card...");
          // make sure that the default chip select pin is set to
          // output, even if you don't use it:
          pinMode(10, OUTPUT);
          // use debugging LEDs
          pinMode(redLEDpin, OUTPUT);
          pinMode(greenLEDpin, OUTPUT);

          if (!SD.begin(10, 11, 12, 13)) {
              error("Card failed, or not present");
            }
            Serial.println("card initialized.");

            // create a new file
            char filename[] = "LOGGER00.CSV";
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

            //return filename;

        }

     

        void SDLoop()                                                // print data to SD card as a CSV
        {
          File dataFile = SD.open("LOGGER00.CSV", FILE_WRITE);
          
          DateTime now;
          now = RTC.now();
          
          if (dataFile) {
            now = RTC.now();
            dataFile.print(pH);
            dataFile.print(", ");
            //dataFile.print(t, DEC);
            //dataFile.print(", ");
            //dataFile.print(h, DEC);
            //dataFile.print(", ");
            dataFile.print(currentLightInLux);
          //dataFile.print(", ");                       //LIQTfindMeTag
          //dataFile.print(liquidTemperatureRead()); // [] FIND A WAY TO REMOVE THE REDUNDANCY: putting this function here means that we read liq T twice each loop: once from calling liqtread in void loop, and once from SDloop. LIQTfindMeTag
            dataFile.print(", ");
            dataFile.print(now.day(), DEC);   
            dataFile.print('/');
            dataFile.print(now.month(), DEC);
            dataFile.print('/');
            dataFile.print(now.year(), DEC);
            dataFile.print(' ');
            dataFile.print(now.hour(), DEC);
            dataFile.print(':');
            dataFile.print(now.minute(), DEC);
            dataFile.print(':');
            dataFile.print(now.second(), DEC);
            dataFile.println();
            dataFile.close();
          }
          else {
            Serial.println("error opening DATALOG.CSV");
          }
        }

        void timeSetup()                                            // start time
        {
            Wire.begin();  
            if (!RTC.begin()) {
            logfile.println("RTC failed");
            Serial.println("RTC failed");
            }    
        }

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
                case '3':    
                  phIncreaseHysteris();                         // pH inc hysteris
                  break;
                case '4':    
                  phDecreaseHysteris();                         // pH dec hysteris
                  break;
                case '5':    
                  FanIncreaseTemp();                            // fan inc temp
                  break;
                case '6':    
                  FanDecreaseTemp();                            // fan dec temp
                  break;
                case '7':    
                  FanIncreaseHumid();                           // fan inc humid
                  break;
                case '8':    
                  FanDecreaseHumid();                           // fan dec humid
                  break;
                case '9':
                  ManualRefilProg();                            // manual order to fill the tank
                  break;
                default:
                  Serial.println('Invalid input. Enter 1-9');
                  break;   // else return error
              }
          }
        }

        void setup() {
          Serial.begin(9600);
          Serial.println();
          delay(1000);
          Serial.println("System initializing");
          EepromRead();             // pull values for Setpoint, SetHysteris, FanTemp, FanHumid from eeprom
          delay(1000);
          dht.begin();
          delay(1000);
          logicSetup();             // set some pinmodes and begin serial comms
          delay(1000);
          timeSetup();              // start wire and RTC ... not sure what this means specifically, but it gets the clock tickin'
          delay(1000);
          SDSetup();                // setup SD card, report if card is missing
          delay(1000);
        }
         
         void loop() {
           logicLoop();             // change control variables based on system state, serial print process variables
           fotoLoop();              // calculate and serial print light level
           FanControl();            // control fan from T and Humid
           //TankProgControl();       // [] MUST REWRITE fill tank if below float level
           SDLoop();                // log {pH, T, Humid, light, date, time} to SD card   [] ADD LIQUID TEMPERATURE
           followSerialCommand();   // respond to serial input 
           delay(3000);
         }