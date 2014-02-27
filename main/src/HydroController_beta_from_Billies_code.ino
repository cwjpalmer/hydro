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
  //#include <DallasTemperature.h>             //Library for Dallas Temperature that may or may not be required for liquid temperature sensor ... get it via $git clone https://github.com/milesburton/Arduino-Temperature-Control-Library.git


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

         /* COMMENTED OUT BECAUSE TOUCH SCREEN
         void graphSetup()
         {
        // Initial setup
          myGLCD.InitLCD(LANDSCAPE);
          myGLCD.clrScr();
          myTouch.InitTouch(LANDSCAPE); //LANDSCAPE
          myTouch.setPrecision(PREC_HI);
          mainscr();
          page = 0;
        }
        */

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
          calFullValue = analogRead(liqLevelsensorValue);
          Serial.print("Full Calibration Value = ");
          Serial.println(liqLevelcalFullValue);
          return liqLevelcalFullValue;
        }

        // function to produce the slope needed for linear fit used to interpolate the liquid level
        // must be run AFTER liqLevelcalibrateEmpty() and liqLevelcalibrateFull()
        float liqLevellinearFitSlope (float liqLevelsensorValue, float liqLevelcalFullValue, float liqLevelslope) 
        {
          slope = 100/(liqLevelcalEmptyValue - liqLevelcalFullValue);
          Serial.print("Slope found = ");
          Serial.println(liqLevelslope);
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

          if (Serial.available() > 0){
            incomingByte = Serial.read(); 
             switch (incomingByte) {
              case '10':    
                liqLevelcalibrateEmpty(liqLevelsensorValue);
                break;
              case '11':    
                liqLevelcalibrateFull(liqLevelsensorValue);
                break;
              case '12':    
                liqLevellinearFitSlope(liqLevelsensorValue, liqLevelcalFullValue, liqLevelslope);
                break;
              default:
                Serial.println('Invalid input. Enter 1 for empty calibration, 2 for full calibration, or 3 to calculate slope'); 
          }

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

        /*
        if (page == 0)
        {
          myGLCD.printNumF(pH, 2, 91, 23);
          myGLCD.printNumI(dht_dat[0], 91, 115, 3);
          myGLCD.printNumI(dht_dat[2], 91, 69, 3);
        }
        delay(250);  
        */             
        }
        

         /* I THINK THIS IS BASICALLY THE TOUCH SIDE OF THE UI
         void graphLoop()
         {
           if (true)
           {
             if (myTouch.dataAvailable())
             
             {
               myTouch.read();
               x=myTouch.getX();
               y=myTouch.getY();
               
               if (page == 0)
               {
                 if ((x>=255) && (x<=312))
                 {
                   if ((y>=17) && (y<=47))
                   {
                     //action for 'SET' button pH
                     page = 2;
                     myGLCD.clrScr();
                     PhSetting();
                   }
                   if ((y>=86) && (y<=114))
                   {
                     //action for 'SET' button fans.
                     page = 1;
                     myGLCD.clrScr();
                     FanSetting();
                   }
                   if ((y>=201) && (y<=229))
                   {
                     //action for 'SET' button Tank.
                     page = 3;
                     myGLCD.clrScr();
                     TankControl();
                   }
                 }
               }
               
               if (page == 1)
               {
                 if ((x>=155) && (x<=182))
                 {
                   if ((y>=47) && (y<=75))
                   {
                     //action for plus sign temp.
                     FanIncreaseTemp();
                   }
                   if ((y>=96) && (y<=122))
                    {
                     //action for minus sign temp.
                     FanDecreaseTemp();
                    }
                   if ((y>=131) && (y<=159))
                    {
                     //action for plus sign humidity.
                     FanIncreaseHumid();
                    }
                   if ((y>=180) && (y<=206))
                    {
                     //action for minus sign humidity.
                     FanDecreaseHumid();
                    }           
                 }
                 if ((x>=235) && (x<=307))
                 {
                   if ((y>=42) && (y<=71))
                   {
                     //action for 'save' button
                     page = 0;
                     myGLCD.clrScr();
                     mainscr();
                     EEPROM.write(EepromFanTemp, FanTemp);
                     EEPROM.write(EepromFanHumid, FanHumid);
                   }
                 }
                 if ((x>=207) && (x<=307))
                 {
                   if ((y>=155) && (y<=184))
                   {
                     //action for 'cancel' button
                     page = 0;
                     myGLCD.clrScr();
                     mainscr();
                     FanTemp = EEPROM.read(EepromFanTemp);
                     FanHumid = EEPROM.read(EepromFanHumid);
                   }
                 }
               }
               if (page == 2)
               {
                 if ((x>=155) && (x<=182))
                 {
                   if ((y>=47) && (y<=75))
                   {
                     //action for plus sign pHmin.
                     phIncreaseSetpoint();
                   }
                   if ((y>=96) && (y<=122))
                    {
                     //action for minus sign pHmin.
                     phDecreaseSetpoint();
                    }
                   if ((y>=131) && (y<=159))
                    {
                     //action for plus sign pHplus.
                     phIncreaseHysteris();
                    }
                   if ((y>=180) && (y<=206))
                    {
                     //action for minus sign pHplus.
                     phDecreaseHysteris();
                    }           
                 }
                 if ((x>=235) && (x<=307))
                 {
                   if ((y>=42) && (y<=71))
                   {
                     //action for 'save' button
                     page = 0;
                     myGLCD.clrScr();
                     mainscr();
                     EEPROM.writeFloat(EepromSetpoint, Setpoint);
                     EEPROM.writeFloat(EepromSetHysteris, SetHysteris);
                   }
                 }
                 if ((x>=207) && (x<=307))
                 {
                   if ((y>=155) && (y<=184))
                   {
                     //action for 'cancel' button
                     page = 0;
                     myGLCD.clrScr();
                     mainscr();
                     Setpoint = EEPROM.readFloat(EepromSetpoint);
                     SetHysteris = EEPROM.readFloat(EepromSetHysteris);
                   }
                 }
               }
               if (page == 3)
               {
                 if ((x>=45) && (x<=167))
                 {
                   if ((y>=64) && (y<=113))
                   {
                   //action for 'En/Disable control' button
                   }
                 }
                 if ((x>=25) && (x<=225))
                 {
                   if ((y>=164) && (y<=194))
                   {
                     //action for 'Manual Refil' button.
                     ManualRefilProg();
                   }
                 }
                 if ((x>=225) && (x<=297))
                 {
                   if ((y>=72) && (y<=101))
                   {
                     //action for 'Home' button.
                     page = 0;
                     myGLCD.clrScr();
                     mainscr();
                   }
                 }
               }
             }
           }
         } 
         */

        /*
        //Mainmenu
        void mainscr()
         {
          myGLCD.fillScr(0, 0, 0);
          myGLCD.setBackColor (0, 0, 0);
           
          myGLCD.setFont(SmallFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("pH", 25, 23);
          myGLCD.print("Temp", 25, 69);
          myGLCD.print("Humid", 25, 115);
          myGLCD.print("Light", 25, 161);
          myGLCD.print("Tank", 25, 207);
          myGLCD.print("Fans", 200, 92);
          myGLCD.print("C", 150, 71); //degree celcius
          myGLCD.print("%", 150, 117); //Percent
          myGLCD.print("Lux.", 165, 163); //Lux
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawLine(160, 79, 196, 97);
          myGLCD.drawLine(160, 120, 196, 97);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor(0, 0, 255);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (255, 17, 312, 47);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("SET", 260, 23); //set pH
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (255, 86, 312, 114);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("SET", 260, 92); //set Fans
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (255, 201, 312, 229);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("SET", 260, 207); //set Tank
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(0, 0, 255);
          myGLCD.setBackColor(255, 255, 255);
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (86, 17, 173, 47);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (86, 17, 173, 47);
          myGLCD.setColor(0, 0, 255);
          //myGLCD.print("5.0", 91, 23); //location value pH
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (86, 64, 143, 93);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (86, 64, 143, 93);
          myGLCD.setColor(0, 0, 255);
          //myGLCD.print("25", 91, 69); //location value Temp
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (86, 110, 143, 139);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (86, 110, 143, 139);
          myGLCD.setColor(0, 0, 255);
          //myGLCD.print("100", 91, 115); //location value Humid
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (86, 156, 158, 185);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (86, 156, 158, 185);
          myGLCD.setColor(0, 0, 255);
          //myGLCD.print("2200", 91, 161); //location value Light
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (86, 202, 222, 231);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (86, 202, 222, 231);
          myGLCD.setColor(0, 0, 255);
          //myGLCD.print("disabled", 91, 207); //location value Tank
         } */
         
         /*
         //Fan Settings - page 1
         void FanSetting()
         {
          myGLCD.fillScr(0, 0, 0);
          myGLCD.setBackColor (0, 0, 0);
         
          myGLCD.setFont(SmallFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("Temp", 25, 79);
          myGLCD.print("Humid", 25, 165);
          myGLCD.print("C", 140, 79);
          myGLCD.print("%", 140, 165);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("Fan Settings", CENTER, 0);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (255, 255, 255);
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (71, 72, 128, 101);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (71, 72, 128, 101);
          myGLCD.setColor(0, 0, 255);
          myGLCD.printNumI(FanTemp, 76, 79); //value Temp
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (71, 155, 128, 184);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (71, 155, 128, 184);
          myGLCD.setColor(0, 0, 255);
          myGLCD.printNumI(FanHumid, 76, 162); //value Humid
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor(0, 0, 255);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 47, 182, 75);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("+", 160, 53);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 96, 182, 122);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("-", 160, 102);
         
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 131, 182, 159);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("+", 160, 137);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 180, 182, 206);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("-", 160, 186);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawLine(167, 78, 167, 95);
          myGLCD.drawLine(168, 78, 168, 95);
          myGLCD.drawLine(167, 162, 167, 179);
          myGLCD.drawLine(168, 162, 168, 179);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (235, 42, 307, 71);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("Save", 240, 49);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (207, 155, 307, 184);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("Cancel", 210, 162);
         }
         */
         
         /*
         //pH Settings - page 2
         void PhSetting()
         {
          myGLCD.fillScr(0, 0, 0);
          myGLCD.setBackColor (0, 0, 0);
         
          myGLCD.setFont(SmallFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("Setp.", 25, 79);
          myGLCD.print("His.", 25, 165);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("pH Settings", CENTER, 0);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (255, 255, 255);
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (71, 72, 143, 101);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (71, 72, 143, 101);
          myGLCD.setColor(0, 0, 255);
          myGLCD.printNumF(Setpoint, 2, 76, 79); //value Min. pH
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (71, 155, 143, 184);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (71, 155, 143, 184);
          myGLCD.setColor(0, 0, 255);
          myGLCD.printNumF(SetHysteris, 2, 76, 162); //value Max. pH
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor(0, 0, 255);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 47, 182, 75);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("+", 160, 53);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 96, 182, 122);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("-", 160, 102);
         
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 131, 182, 159);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("+", 160, 137);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (155, 180, 182, 206);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("-", 160, 186);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawLine(167, 78, 167, 95);
          myGLCD.drawLine(168, 78, 168, 95);
          myGLCD.drawLine(167, 162, 167, 179);
          myGLCD.drawLine(168, 162, 168, 179);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (235, 42, 307, 71);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("Save", 240, 49);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (207, 155, 307, 184);
          myGLCD.setColor(255, 255, 255);
          myGLCD.print("Cancel", 210, 162);
         }
         */
         
         /*
         //Manual Tank Control - page 3
         void TankControl()
         {
          myGLCD.fillScr(0, 0, 0);
          myGLCD.setBackColor (0, 0, 0);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 0);
          myGLCD.print("Manual Tank Control", CENTER, 0);
         
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (225, 72, 297, 101);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 255);
          myGLCD.print("Home", 230, 79);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (45, 64, 167, 113);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 255);
          myGLCD.print("Enable", 50, 71);
          myGLCD.print("Control", 50, 91);
          myGLCD.setColor(0, 0, 255);
          myGLCD.fillRoundRect (25, 164, 225, 194);
          myGLCD.setColor(255, 255, 255);
          myGLCD.setBackColor (0, 0, 255);
          myGLCD.print("Manual Refil", 30, 171);
         
          myGLCD.setFont(BigFont);
          myGLCD.setColor(0, 0, 255);
          myGLCD.setBackColor(255, 255, 255);
          myGLCD.setColor(255, 255, 255);
          myGLCD.fillRoundRect (257, 164, 310, 194);
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawRoundRect (257, 164, 310, 194);
          myGLCD.setColor(0, 0, 255);
          myGLCD.print("OFF", 260, 171); //status refil
          myGLCD.setColor(255, 255, 0);
          myGLCD.drawLine(229, 179, 253, 179);
          }
         */
         
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
          /*
          if (page == 2) {                                           this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumF(Setpoint, 2, 76, 79);
          }
          */
        }

        void phDecreaseSetpoint() {
          Setpoint = Setpoint - 0.01;
          if (Setpoint <= 3.00) {
            Setpoint = 3.00;
          }
          /*
          if (page == 2) {                                          this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumF(Setpoint, 2, 76, 79);
          }
          */
        }

        void phIncreaseHysteris() {
          SetHysteris = SetHysteris + 0.01;
          if (SetHysteris >= 9.00) {
            SetHysteris = 9.00;
          }
          /*
          if (page == 2) {                                           this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumF(SetHysteris, 2, 76, 162);
          }
          */
        }

        void phDecreaseHysteris() {
          SetHysteris = SetHysteris - 0.01;
          if (SetHysteris <= 0.01) {
            SetHysteris = 0.01;
          }
          /*
          if (page == 2) {                                         this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumF(SetHysteris, 2, 76, 162);
          }
          */
        }

        void FanIncreaseTemp() {
          FanTemp = FanTemp + 1;
          if (FanTemp >= 50) {
            FanTemp = 50;
          }
          /*
          if (page == 1) {                                         this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumI(FanTemp, 76, 79);
          */
        }


        void FanDecreaseTemp() {
          FanTemp = FanTemp - 1;
          if (FanTemp <= 0) {
            FanTemp = 0;
          }
          /*
          if (page == 1) {                                          this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumI(FanTemp, 76, 79);
          */
        }


        void FanIncreaseHumid() {
          FanHumid = FanHumid + 1;
          if (FanHumid >= 100) {
            FanHumid = 100;
          }
          /*
          if (page == 1) {                                           this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumI(FanHumid, 76, 162);
          */
        }


        void FanDecreaseHumid() {
          FanHumid = FanHumid - 1;
          if (FanHumid <= 0) {
            FanHumid = 0;
          }
          /*
          if (page == 1) {                                          this is LCD input stuff - gut it
            myGLCD.setColor(0, 0, 255);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.printNumI(FanHumid, 76, 162);
          */
        }


        void FanControl() {                                            // control the fan based on DHT sensor T and Humididity
          if ((dht_dat[0] >= FanHumid) && (dht_dat[2] >= FanTemp)) {   // if himidity is too high and temp us too high, turn fan on - note not 100% sure the DHT[] values are T and H, but logicall they should be
            digitalWrite(ventilatorPin, HIGH);
          }
          else {
            digitalWrite(ventilatorPin, LOW);
          }
        }

                                                                       // [] rewrite to use level sensor, instead of float switch as input
        void TankProgControl() {                                        // this controls a solenoid to refill the tank as necessry based on the float switch
          int levelHigh = LOW;
          int levelLow = LOW;
         
          levelHigh = digitalRead(floatHighPin);
          levelLow = digitalRead(floatLowPin);
         
          if (levelHigh == LOW) {
            /*
            if (page == 0) {
              myGLCD.setColor(0, 0, 255);
              myGLCD.print("HalfFull", 91, 207);
            }
            */
            if (levelLow == LOW) {
              /*
              if (page == 0) {
                myGLCD.setColor(0, 0, 255);
                myGLCD.print("Filling ", 91, 207);
              }
              */
              digitalWrite(solenoidPin, HIGH); //solenoid valve open.
            }
          }
          else
          {
            /*
            if (page == 0) {
              myGLCD.setColor(0, 0, 255);
              myGLCD.print("Full    ", 91, 207);
            }
            */
            if (levelLow == HIGH) {
              digitalWrite(solenoidPin, LOW); //solenoid valve closed.
              /*
              if (page == 3) {
                myGLCD.setColor(0, 0, 255);
                myGLCD.print("OFF", 260, 171);
              }
              */
            }
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
                  Serial.println('Invalid input. Enter 1-9');   // else return error
              }
          }
        }

        void setup() {
          EepromRead();             // pull values for Setpoint, SetHysteris, FanTemp, FanHumid from eeprom
          logicSetup();             // set some pinmodes and begin serial comms
        //graphSetup();             // *GONE* LCD initalization
          timeSetup();              // start wire and RTC ... not sure what this means specifically, but it gets the clock tickin'
          SDSetup();                // setup SD card, report if card is missing
        }
         
         void loop() {
         //graphLoop();             // *GONE* LCD display
           logicLoop();             // change control variables based on system state, serial print process variables
           fotoLoop();              // calculate and serial print light level
         //liquidTemperatureRead(); // read liquid temperature, serial print  LIQTfindMeTag
           FanControl();            // control fan from T and Humid
           TankProgControl();       // [] MUST REWRITE fill tank if below float level
           SDLoop();                // log {pH, T, Humid, light, date, time} to SD card   [] ADD LIQUID TEMPERATURE
           followSerialCommand();   // respond to serial input 
         }