
int incomingByte = 0;


void setup() {
	Serial.begin(9600);

}

void loop() {
	 if (Serial.available() > 0) {
     	incomingByte = Serial.read();
     }
     Serial.print('IncomingByte is ');
     Serial.println(incomingByte);
     incomingByte=0;
     delay(1000);
   
}

/*
		void followSerialCommand() {
          if (Serial.available() > 0) {
            int incomingByte = Serial.read();
              switch (incomingByte) {
                case 'i':    
                  phIncreaseSetpoint();                         // pH inc set point
                  break;
                case 'down':    
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
        */