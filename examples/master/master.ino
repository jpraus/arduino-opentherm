#include <opentherm.h>

#define BOILER_IN 3
#define BOILER_OUT 5

OpenthermData message;

void setup() {
  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_OUT, HIGH);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage

  Serial.begin(19200);
}

/**
 * Loop will act as thermostat (master) connected to Opentherm boiler.
 * It will request slave configration from boiler every 100ms or so and waits for response from boiler.
 */
void loop() {
  if (OPENTHERM::isIdle()) {
    message.type = OT_MSGTYPE_READ_DATA;
    message.id = OT_MSGID_SLAVE_CONFIG;
    message.valueHB = 0;
    message.valueLB = 0;
    Serial.print("-> "); 
    OPENTHERM::printToSerial(message); 
    Serial.println();
    OPENTHERM::send(BOILER_OUT, message); // send message to boiler
  }
  else if (OPENTHERM::isSent()) {
    OPENTHERM::listen(BOILER_IN, 800); // wait for boiler to respond
  }
  else if (OPENTHERM::getMessage(message)) { // boiler responded
    OPENTHERM::stop();
    Serial.print("<- ");
    OPENTHERM::printToSerial(message);
    Serial.println();
    Serial.println();
    delay(100); // minimal delay before next communication
  }
  else if (OPENTHERM::isError()) {
    OPENTHERM::stop();
    Serial.println("<- Timeout");
    Serial.println();
  }
}
