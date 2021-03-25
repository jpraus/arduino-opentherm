// Wemos D1 R1
#define THERMOSTAT_IN 16
#define THERMOSTAT_OUT 4
#define BOILER_IN 5
#define BOILER_OUT 14

// Wemos D1 R2
//#define THERMOSTAT_IN 16
//#define THERMOSTAT_OUT 4
//#define BOILER_IN 5
//#define BOILER_OUT 0

// Arduino UNO
//#define THERMOSTAT_IN 2
//#define THERMOSTAT_OUT 4
//#define BOILER_IN 3
//#define BOILER_OUT 5

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

  Serial.println(F("OpenTherm gateway self-test"));
}

/**
 * https://github.com/jpraus/arduino-opentherm#testing-out-the-hardware
 * 
 * Self test
 * - Connect 24V power supply to red terminal
 * - Interconnect BLUE THERM and GREEN BOILER terminals with each other with 2 wires. Polarity does not matter at all.
 */
void loop() {
/*
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);
  Serial.print(F("H => "));
  Serial.println(digitalRead(THERMOSTAT_IN));

  delay(1000);

  digitalWrite(BOILER_OUT, LOW);
  delay(10);
  Serial.print(F("L => "));
  Serial.println(digitalRead(THERMOSTAT_IN));
  

  delay(1000);
  return;
*/
  Serial.print(F("Boiler inbound, thermostat outbound .. "));
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // thermostat out low -> boiler in high
    digitalWrite(THERMOSTAT_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 1) { // ok
      Serial.println(F("OK"));
    }
    else {
      Serial.println(F("Failed"));
      Serial.println(F("Boiler is not registering signal or thermostat is not sending properly"));
    }
  }
  else {
    Serial.println(F("Failed"));
    Serial.println(F("Boiler is high even if no signal is being sent"));
  }
  
  Serial.print(F("Boiler outbound, thermostat inbound .. "));
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // boiler out low -> thermostat in high
    digitalWrite(BOILER_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 1 && digitalRead(BOILER_IN) == 0) { // ok
      Serial.println(F("OK"));
    }
    else {
      Serial.println(F("Failed"));
      Serial.println(F("Thermostat is not registering signal or boiler is not sending properly"));
    }
  }
  else {
    Serial.println(F("Failed"));
    Serial.println(F("Thermostat is high even if no signal is being sent"));
  }

  delay(1000);
}
