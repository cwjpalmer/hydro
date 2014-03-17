String commandIn;

char command[20];
char* c1;
char* c2;
char* c3;

float ph=4.0;
int tankLowSetPoint = 20;
int tankHighSetPoint = 80;

void setup() {
	Serial.begin(9600);
}

void loop () {
	Serial.print("pH set at: ");
	Serial.println(ph);

	Serial.print("Awaiting command...");
	delay(1000);

	// if serial comms are receiving
	if (Serial.available() > 0) {
		// read string
		commandIn = Serial.readString();
		commandIn.toCharArray(command, 20);
		// print string 
		Serial.print("Your command was: ");
		Serial.println(command);

		c1 = strtok(command," ");
		c2 = strtok(NULL," ");
		c3 = strtok(NULL," ");
		Serial.print("c1 is: ");
		Serial.println(c1);
		Serial.print("c2 is: ");
		Serial.println(c2);
		Serial.print("c3 is: ");
		Serial.println(c3);

		if (strcmp(c1,"ph") == 0) {
			Serial.println("command ph recognized");
			
			if (strcmp(c2,"set") == 0) {
				Serial.println("command ph set recognized");
				ph = atof(c3);
			}
			else if (strcmp(c2,"increase") == 0) {
				Serial.println("command ph increase recognized");
				if (c3) {
					//Serial.println("c3 is present");
					ph = ph + atof(c3);
				}
				else {
					ph = ph + 0.1;
				}
			}
			else if (strcmp(c2,"decrease") == 0) {
				Serial.println("command ph decrease recognized");
			}
			else {
				Serial.println("pH comand not recognized. try again.");
			}
		}
		// elseif() {

		// }
		else {
			Serial.println("comand not recognized. try again.");
		}
	
	}
	else {
		Serial.println("No command registered");
	}
	
}
