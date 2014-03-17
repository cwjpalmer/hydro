String commandIn;

char command[20];
char *c1;
char *c2;
char *c3;


int ph = 4;


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

	}
	else {
		Serial.println("No command registered");
	}
	
}
