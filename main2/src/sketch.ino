
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
    #include "SPI.h"
  



    #define DHTPIN 34                          //pin for DHT22                     
    #define redLEDpin 2                        //LEDs on SD card
    #define greenLEDpin 3                      //LEDs on SD Card 
    #define DHTTYPE DHT22                      // DHT 22  (AM2302)

    #define ECHO_TO_SERIAL   1                 // echo data to serial port
    #define WAIT_TO_START    0                 // Wait for serial input in setup()
    int DS18S20_Pin = 28; //DS18S20 Signal pin on digital 2


    //Temperature chip i/o
    OneWire ds(DS18S20_Pin);  // on digital pin 2

    // liquid level
    #define liqLevelRefResistor 2250           // [] it's 2250 ohms +/- 10%, so we should check it with a multimeter and put the correct value here    
    #define liqLevelSensorPin A10 

    // LED Pins to represent control systems
    #define LED_SOLENOID_PIN 38
    #define LED_LIQ_PIN 40



    DHT dht(DHTPIN, DHTTYPE);

    int pHPin = A7;                            //pin for pH probe                  
    int pHPlusPin = 45;                        //pin for Base pump (relay)         
    int pHMinPin = 46;                         //pin for Acide pump (relay)        
    int lightSensor = A8;                      //pin for Photoresistor             
    int solenoidPin = 49;                      //digital pin
    float h;                                   //humidity 
    float t;                                   //temperature
  //int liquidTemperaturePin = 2;              //digital pin   LIQTfindMeTag
    char filename[] = "LOGGER00.CSV";          //filename for CSV file

    int tankLowSetPoint = 850;                           // variable to store lower limit of tank level   // as a %
    int tankHighSetPoint = 80;                          // variable to store upper limit of tank level   // as a %


    File logfile;                               //indicate CSV file exists 
    RTC_DS1307 RTC;                            //Define RTC module

  
    int x, y;
    int page = 0;
    int tankProgState = 0;
    int manualRefilState = 0;
    float pH;                          //generates the value of pH

    int pmem = 0;                      //check which page you're on
    float Setpoint=4.8;                    //holds value for Setpoint
    float HysterisMin;                 
    float HysterisPlus;
    float SetHysteris;

    int lightADCReading;
    double currentLightInLux;
    double lightInputVoltage;
    double lightResistance;

    int EepromSetpoint = 10;      //location of Setpoint in Eeprom
    int EepromSetHysteris = 20;   //location of SetHysteris in Eeprom
    float liqTemperatureOutput;


    byte bGlobalErr;    //for passing error code back.

         
         // - DISABLE EEPROM WHEN NOT REQUIRED - specified life of 100k write/erase cycles
         void EepromRead() { // memory whose values are kept when board loses power  - specified life of 100k write/erase cycles
          //Setpoint = EEPROM.readFloat(EepromSetpoint);
          //SetHysteris = EEPROM.readFloat(EepromSetHysteris);
        }


        void logicSetup() {
        pinMode(pHPlusPin, OUTPUT);
        pinMode(pHMinPin, OUTPUT);
        pinMode(ventilatorPin, OUTPUT);
        pinMode(solenoidPin, OUTPUT);
        pinMode(LED_SOLENOID_PIN, OUTPUT);
        pinMode (LED_LIQ_PIN, OUTPUT);
         
        pmem==0;
         
        delay(300);
        Serial.println("System Booting!");
        Serial.println("__________________________________\n\n");
        delay(700);
        }



        // function to determine the liquid level from a sensor value; sensor value is input
        int getLiqLevel() {
          float reading;
 
          reading = analogRead(liqLevelSensorPin);
         
          // convert the value to resistance
          reading = (1023 / reading)  - 1;
          reading = liqLevelRefResistor / reading;
          Serial.print("Sensor resistance "); 
          Serial.println(reading);

          return reading;
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
            delay(300);                          // LED
            digitalWrite (pHPlusPin, LOW);       // LED
            digitalWrite (pHMinPin, LOW);
          }
         
          if (pH >= HysterisMin && pH < Setpoint && pmem == 1) {
            digitalWrite (pHPlusPin, HIGH);
            delay(300);                          // LED
            digitalWrite (pHPlusPin, LOW);       // LED            
            digitalWrite (pHMinPin, LOW);
          }
         
          if (pH > HysterisPlus && pmem == 0) {
            pmem ==2,
            digitalWrite (pHMinPin, HIGH);
            delay(300);                          // LED
            digitalWrite (pHMinPin, LOW);        // LED            
            digitalWrite (pHPlusPin, LOW);
          }
         
          if (pH <= HysterisPlus && pH > Setpoint && pmem == 2) {
            digitalWrite (pHMinPin, HIGH);
            delay(300);                          // LED
            digitalWrite (pHMinPin, LOW);        // LED            
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

          float liqTemperature = getTemp();
          Serial.print("Liquid Temperature: ");
          Serial.print(liqTemperature);
          Serial.println(" °C");
                 
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


        void TankProgControl () {
          if (getLiqLevel() < tankLowSetPoint) {
              //digitalWrite(solenoidPin, HIGH);  //open solenoid valve
            digitalWrite(LED_SOLENOID_PIN, LOW);    // LED representation 
          }
          else {
            //digitalWrite(solenoidPin, LOW);     //close solenoid valve
            digitalWrite(LED_SOLENOID_PIN, HIGH);    // LED representation             
          }
        }

        float getTemp(){
          //returns the temperature from one DS18S20 in DEG Celsius

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

     

        void SDLoop()                                                // print data to SD card as a CSV
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
            Serial.println("error opening CSV file");
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
                default:
                  Serial.println('Invalid input. Enter 1-9');
                  break;   // else return error
              }
          }
        }

        void setup() {
          Serial.begin(9600);
          Serial.println();
          EepromRead();             // pull values for Setpoint, SetHysteris, from eeprom
          dht.begin();
          logicSetup();             // set some pinmodes and begin serial comms
          timeSetup();              // start wire and RTC ... not sure what this means specifically, but it gets the clock tickin'
          SDSetup();                // setup SD card, report if card is missing
        }
         
         void loop() {
           logicLoop();             // change control variables based on system state, serial print process variables
           fotoLoop();              // calculate and serial print light level
           TankProgControl();       // [] MUST REWRITE fill tank if below float level
           SDLoop();                // log {pH, T, Humid, light, date, time} to SD card   [] ADD LIQUID TEMPERATURE
           followSerialCommand();   // respond to serial input 
           delay(1000);
         }