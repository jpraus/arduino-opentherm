// Wemos D1 R1
//#define THERMOSTAT_IN 16
//#define THERMOSTAT_OUT 4
//#define BOILER_IN 5
//#define BOILER_OUT 14

// Arduino UNO
#define THERMOSTAT_IN 2
#define THERMOSTAT_OUT 4
#define BOILER_IN 3
#define BOILER_OUT 5

void setup() {
  pinMode(THERMOSTAT_IN, INPUT);
  digitalWrite(THERMOSTAT_IN, HIGH); // pull up
  digitalWrite(THERMOSTAT_OUT, HIGH);
  pinMode(THERMOSTAT_OUT, OUTPUT); // low output = high current, high output = low current
  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_IN, HIGH); // pull up
  digitalWrite(BOILER_OUT, LOW);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage

  Serial.begin(115200);
  delay(1000);

  Serial.println("OpenTherm gateway self-test");
}

/**
 * https://github.com/jpraus/arduino-opentherm#testing-out-the-hardware
 * 
 * Self test
 * - Connect 24V power supply to red terminal
 * - Interconnect BLUE THERM and GREEN BOILER terminals with each other with 2 wires. Polarity does not matter at all.
 */
void loop() {
  Serial.println();

  Serial.print("Boiler inbound, thermostat outbound .. ");
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // thermostat out low -> boiler in high
    digitalWrite(THERMOSTAT_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 1) { // ok
      Serial.println("OK");
    }
    else {
      Serial.println("Failed");
      Serial.println("Boiler is not registering signal or thermostat is not sending properly");
    }
  }
  else {
    Serial.println("Failed");
    Serial.println("Boiler is high even if no signal is being sent");
  }
  
  Serial.print("Boiler outbound, thermostat inbound .. ");
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // boiler out low -> thermostat in high
    digitalWrite(BOILER_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 1 && digitalRead(BOILER_IN) == 0) { // ok
      Serial.println("OK");
    }
    else {
      Serial.println("Failed");
      Serial.println("Thermostat is not registering signal or boiler is not sending properly");
    }
  }
  else {
    Serial.println("Failed");
    Serial.println("Thermostat is high even if no signal is being sent");
  }

  delay(5000);
}
