#include <opentherm.h>

#define THERMOSTAT_IN 2
#define THERMOSTAT_OUT 4
#define BOILER_IN 3
#define BOILER_OUT 5

OpenthermData message;

void setup() {
  pinMode(THERMOSTAT_IN, INPUT);
  digitalWrite(THERMOSTAT_OUT, HIGH);
  pinMode(THERMOSTAT_OUT, OUTPUT); // low output = high current, high output = low current
  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_OUT, HIGH);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage

  Serial.begin(19200);
}

#define MODE_LISTEN_MASTER 0
#define MODE_LISTEN_SLAVE 1

int mode = 0;

/**
 * Loop will act as man in the middle connected between Opentherm boiler and Opentherm thermostat.
 * It will listen for requests from thermostat, forward them to boiler and then wait for response from boiler and forward it to thermostat.
 * Requests and response are logged to Serial on the way through the gateway.
 */
void loop() {
  if (mode == MODE_LISTEN_MASTER) {
    if (OPENTHERM::isSent() || OPENTHERM::isIdle() || OPENTHERM::isError()) {
      OPENTHERM::listen(THERMOSTAT_IN);
    }
    else if (OPENTHERM::getMessage(message)) {
      Serial.print("-> ");
      OPENTHERM::printToSerial(message);
      Serial.println();
      OPENTHERM::send(BOILER_OUT, message); // forward message to boiler
      mode = MODE_LISTEN_SLAVE;
    }
  }
  else if (mode == MODE_LISTEN_SLAVE) {
    if (OPENTHERM::isSent()) {
      OPENTHERM::listen(BOILER_IN, 800); // response need to be send back by boiler within 800ms
    }
    else if (OPENTHERM::getMessage(message)) {
      Serial.print("<- ");
      OPENTHERM::printToSerial(message);
      Serial.println();
      Serial.println();
      OPENTHERM::send(THERMOSTAT_OUT, message); // send message back to thermostat
      mode = MODE_LISTEN_MASTER;
    }
    else if (OPENTHERM::isError()) {
      mode = MODE_LISTEN_MASTER;
      Serial.println("<- Timeout");
      Serial.println();
    }
  }
}
