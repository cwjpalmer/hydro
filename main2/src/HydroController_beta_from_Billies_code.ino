//#include <EEPROMex.h> //WTF is there 'xx' here?
//#include <EEPROMVar.h>

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
    Software Requirements:
    ----------------------
    -Arduino IDE 1.0
    -UTFT library
    -ITDB02_Touch library
    -SD library
    -Wire library
    -RTClib library
    -EEPROM library
    -EEPROMex library

    Hardware Requirements:
    ----------------------
    -Arduino Mega 2560
    -3.2 inch TFT Touchscreen
    -LC Studio SD Cardreader
    -DS1307 Real Time Clock module
    -Phidgets 1130 pH/ORP module
    -4 channel Relay 220V/10Amp
    -DHT11 or DHT22 sensor
    -pH electrode probe with BNC connector
    -Photoresistor
    -Solenoid valve 12V
    -2x Peristaltic pump
    -2x Duckbill side waterlevel Float sensor
    -resistors (220ohm - 1Kohm)
    -Cermet Potentiometer 110Kohm
    -Protoboard
    -9V DC powersupply
    */

  //#include <UTFT.h>                          //16bit TFT screen library            // commented out because not using touch screen
  //#include <ITDB02_Touch.h>                  //Touchscreen library                 // commented out because not using touch screen
    #include <SD.h>                            //SD card library
    #include <Wire.h>                          //One Wire library
    #include "RTClib.h"                        //Real Time Clock library
    #include <EEPROMex.h>                      //Extended Eeprom library
    #include <OneWire.h>                       //OneWire library, for liquid temperature sensor
  //#include <DallasTemperature.h>             //Library for Dallas Temperature that may or may not be required for liquid temperature sensor 


  //UTFT myGLCD(ITDB32S,38,39,40,41);          //pins used for TFT                  // commented out because not using touch screen
  //ITDB02_Touch  myTouch(6,5,4,3,2);          //pins used for Touch                // commented out because not using touch screen

    #define dht_dpin 44                        //pin for DHT11                      // [  ] 1 digital input, 10KOhm resistor, 5V
    int pHPin = A0;                            //pin for pH probe                   // [  ] analog Pin
    int pHPlusPin = 45;                        //pin for Base pump (relay)          // [  ] digital Pin
    int pHMinPin = 46;                         //pin for Acide pump (relay)         // [  ] digital Pin
    int ventilatorPin = 47;                    //pin for Fan (relay)                // [  ] digital Pin
    int floatLowPin = A1;                      //pin for lower float sensor         // [  ] -> level sensor: 2 analog inputs, 10KOhm resistor, 5V 
    int floatHighPin = A2;                     //pin for upper float sensor         // [  ] -> level sensor: 2 analog inputs, 10KOhm resistor, 5V
    int lightSensor = A3;                      //pin for Photoresistor              // [  ]   1 analog input, 10kOhm resistor, 5V
    int sdPin = 10;                            //pin for serial comms with SD card  // [  ] Adafruit uses 'echo data to serial'
    int solenoidPin = 49;                      // digital pin
  //int liquidTemperaturePin = 2;             // digital pin   LIQTfindMeTag

    /*      removed because used for LCD display
    extern uint8_t BigFont[];                  //Which fonts to use...
    extern uint8_t SmallFont[];
    extern uint8_t SevenSegNumFont[];
    */

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

    DateTime now;                 //call current Date and Time

    #define DHTTYPE DHT11
    byte bGlobalErr;    //for passing error code back.
    byte dht_dat[4];    //Array to hold the bytes sent from sensor.


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
         
        InitDHT();
        Serial.begin(9600);
        delay(300);
        Serial.println("Luchtvochtigheid, temperatuur & pH");
        Serial.println("__________________________________\n\n");
        delay(700);
        }

        /*
        ----- LIQUID LEVEL SENSOR FUNCTIONS -----
        */


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
        } 


        void logicLoop() { // loop that prints humidity lvl ("Luchtvochtigheid") <- that is the angriest humidity I have ever seen 
 
          ReadDHT();

          switch (bGlobalErr) {
            case 0:
           Serial.print("Luchtvochtigheid = ");
           Serial.print(dht_dat[0], DEC);
           Serial.println("%  ");
           Serial.print("Temperatuur = ");
           Serial.print(dht_dat[2], DEC);
           Serial.println(" *C  ");

                break;
             case 1:
                Serial.println("Error 1: DHT start condition 1 not met.");
                break;
             case 2:
                Serial.println("Error 2: DHT start condition 2 not met.");
                break;
             case 3:
                Serial.println("Error 3: DHT checksum error.");
                break;
             default:
                Serial.println("Error: Unrecognized code encountered.");
                break;
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
                 
         
        void InitDHT() { // setup DHT sensor
            pinMode(dht_dpin,OUTPUT);
            digitalWrite(dht_dpin,HIGH);
        }

        void ReadDHT() {
          bGlobalErr=0;
          byte dht_in;
          byte i;
          digitalWrite(dht_dpin,LOW);
          delay(23);
          digitalWrite(dht_dpin,HIGH);
          delayMicroseconds(40);
          pinMode(dht_dpin,INPUT);
          dht_in=digitalRead(dht_dpin); // intresting trick - send a pulse of power to the pin and then read it (last 5 lines)

          if(dht_in) {
             bGlobalErr=1; //dht start condition 1 not met
             return;
          }
         
          delayMicroseconds(80);
          dht_in=digitalRead(dht_dpin);

          if(!dht_in) {
             bGlobalErr=2;//dht start condition 2 not met
             return;
          }

          delayMicroseconds(80);
          for (i=0; i<5; i++)
          dht_dat[i] = read_dht_dat();

          pinMode(dht_dpin,OUTPUT);

          digitalWrite(dht_dpin,HIGH);

          byte dht_check_sum =
               dht_dat[0]+dht_dat[1]+dht_dat[2]+dht_dat[3]; // what's this check sum checking?

          if(dht_dat[4]!= dht_check_sum)
           {bGlobalErr=3;}
        } // removed semicolon because I don't think it belonged


        byte read_dht_dat() { // what does this do?
          byte i = 0;
          byte result=0;
          for(i=0; i< 8; i++)
          {
              while(digitalRead(dht_dpin)==LOW);
              delayMicroseconds(45);

              if (digitalRead(dht_dpin)==HIGH)
               result |=(1<<(7-i));
               
                while (digitalRead(dht_dpin)==HIGH);
            }
          return result;
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
          if ((dht_dat[0] >= FanHumid) && (dht_dat[2] >= FanTemp)) {   // if himidity is too high and temp us too high, turn fan on - note not 100% sure the DHT[] values are T and H, but logicall they should be
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
          /*
          if (page == 3) {                                             // LCD screen stuff - []remove
            myGLCD.setColor(255, 255, 0);
            myGLCD.print("ON ", 260, 171);
          }
          */
        }

        void SDSetup()                                                // set up SD card
        {
          Serial.print("Initializing SD card...");
          pinMode(sdPin, OUTPUT);

          if (!SD.begin(sdPin)) {                                // chipselect is the 
            Serial.println("Card Failed, or not present");
            return;
          }
          Serial.println("card initialized.");
        }



        void SDLoop()                                                // print data to SD card as a CSV
        {
          File dataFile = SD.open("datalog.csv", FILE_WRITE);
         
          if (dataFile) {
            now = RTC.now();
            dataFile.print(pH);
            dataFile.print(", ");
            dataFile.print(dht_dat[2], DEC);
            dataFile.print(", ");
            dataFile.print(dht_dat[0], DEC);
            dataFile.print(", ");
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
            Serial.println("error opening datalog.csv");
          }
        }

        void timeSetup()                                            // start time
        {
          Wire.begin();
          RTC.begin();
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
                  Serial.println('Invalid input. Enter 1-12');
                  break;   // else return error
              }
          }
        }

        void setup() {
          EepromRead();             // pull values for Setpoint, SetHysteris, FanTemp, FanHumid from eeprom
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
           TankProgControl();       // [] MUST REWRITE fill tank if below float level
           SDLoop();                // log {pH, T, Humid, light, date, time} to SD card   [] ADD LIQUID TEMPERATURE
           followSerialCommand();   // respond to serial input 
         }