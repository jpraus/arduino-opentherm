#include "opentherm.h"
#include "Arduino.h"

#define MODE_IDLE 0     // no operation

#define MODE_LISTEN 1   // waiting for transmission to start
#define MODE_READ 2     // reading 32-bit data frame
#define MODE_RECEIVED 3 // data frame received with valid start and stop bit

#define MODE_WRITE 4    // writing data with timer
#define MODE_SENT 5     // all data written to output

#define MODE_ERROR_MANCH 8  // manchester protocol data transfer error
#define MODE_ERROR_TOUT 9   // read timeout

byte OPENTHERM::_pin = 0;
void (*OPENTHERM::_callback)() = NULL;

volatile byte OPENTHERM::_mode = MODE_IDLE;
volatile unsigned int OPENTHERM::_capture = 0;
volatile byte OPENTHERM::_clock = 0;
volatile byte OPENTHERM::_bitPos = 0;
volatile unsigned long OPENTHERM::_data = 0;
volatile bool OPENTHERM::_active = false;
volatile int OPENTHERM::_timeoutMs = -1;

#define STOP_BIT_POS 33

void OPENTHERM::listen(byte pin, int timeout, void (*callback)()) {
  _stop();
  _pin = pin;
  _timeoutMs = timeout;
  _callback = callback;

  _listen();
}

void OPENTHERM::_listen() {
  _stopTimer();
  _mode = MODE_LISTEN;
  _active = true;
  _data = 0;
  _bitPos = 0;
  noInterrupts();
  attachInterrupt(digitalPinToInterrupt(_pin), OPENTHERM::_risingSignalISR, RISING);
  interrupts();
  if (_timeoutMs > 0) {
    _startTimeoutTimer(); // tick every 1ms
  }
}

void OPENTHERM::send(byte pin, OpenthermData &data, void (*callback)()) {
  _stop();
  _pin = pin;
  _callback = callback;

  _data = data.type;
  _data = (_data << 12) | data.id;
  _data = (_data << 8) | data.valueHB;
  _data = (_data << 8) | data.valueLB;
  if (!_checkParity(_data)) {
    _data = _data | 0x80000000;
  }

  _clock = 1; // clock starts at HIGH
  _bitPos = 33; // count down (33 == start bit, 32-1 data, 0 == stop bit)
  _mode = MODE_WRITE;

  _active = true;
  _startWriteTimer();
}

bool OPENTHERM::getMessage(OpenthermData &data) {
  if (_mode == MODE_RECEIVED) {
    data.type = (_data >> 28) & 0x7;
    data.id = (_data >> 16) & 0xFF;
    data.valueHB = (_data >> 8) & 0xFF;
    data.valueLB = _data & 0xFF;
    return true;
  }
  return false;
}

void OPENTHERM::stop() {
  _stop();
  _mode = MODE_IDLE;
}

void OPENTHERM::_stop() {
  if (_active) {
    detachInterrupt(digitalPinToInterrupt(_pin)); 
    _stopTimer();
    _active = false;
  }
}

void OPENTHERM::_risingSignalISR() {
  detachInterrupt(digitalPinToInterrupt(_pin));
  _data = 0;
  _bitPos = 0;
  _mode = MODE_READ;
  _capture = 1; // reset counter and add as if read start bit
  _clock = 1; // clock is high at the start of comm
  _startReadTimer(); // get us into 1/4 of manchester code
}

void OPENTHERM::_timerISR() {
  if (_mode == MODE_LISTEN) {
    if (_timeoutMs > 0) {
      _timeoutMs --;
    }
    if (_timeoutMs == 0) {
      _mode = MODE_ERROR_TOUT;
      _stop();
    }
  }
  else if (_mode == MODE_READ) {
    byte value = digitalRead(_pin);
    byte last = (_capture & 1);
    if (value != last) {
      // transition of signal from last sampling
      if (_clock == 1 && _capture > 0xF) {
        // no transition in the middle of the bit
        _listen();
      }
      else if (_clock == 1 || _capture > 0xF) {
        // transition in the middle of the bit OR no transition between two bit, both are valid data points
        if (_bitPos == STOP_BIT_POS) {
          // expecting stop bit
          if (_verifyStopBit(last)) {
            _mode = MODE_RECEIVED;
            _stop();
            _callCallback();
          }
          else {
            // end of data not verified, invalid data
            _listen();
          }
        }
        else {
          // normal data point at clock high
          _bitRead(last);
          _clock = 0;
        }
      }
      else {
        // clock low, not a data point, switch clock
        _clock = 1;
      }
      _capture = 1; // reset counter
    }
    else if (_capture > 0xFF) {
      // no change for too long, invalid mancheter encoding
      _listen();
    }
    _capture = (_capture << 1) | value;
  }
  else if (_mode == MODE_WRITE) {
    // write data to pin
    if (_bitPos == 33 || _bitPos == 0)  { // start bit
      _writeBit(1, _clock);
    }
    else { // data bits
      _writeBit(bitRead(_data, _bitPos - 1), _clock);
    }
    if (_clock == 0) {
      if (_bitPos <= 0) { // check termination
        _mode = MODE_SENT; // all data written
        _stop();
        _callCallback();
      }
      _bitPos--;
      _clock = 1;
    }
    else {
      _clock = 0;
    }
  }
}

ISR(TIMER2_COMPA_vect) { // Timer2 interrupt
  OPENTHERM::_timerISR();
}

void OPENTHERM::_bitRead(byte value) {
  _data = (_data << 1) | value;
  _bitPos ++;
}

bool OPENTHERM::_verifyStopBit(byte value) {
  if (value == HIGH) { // stop bit detected
    if (_checkParity(_data)) { // parity check, success
      return true;
    }
    else { // parity check failed, error
      return false;
    }
  }
  else { // no stop bit detected, error
    return false;
  }
}

void OPENTHERM::_writeBit(byte high, byte clock) {
  if (clock == 1) { // left part of manchester encoding
    digitalWrite(_pin, !high); // low means logical 1 to protocol
  }
  else { // right part of manchester encoding
    digitalWrite(_pin, high); // high means logical 0 to protocol
  }
}

bool OPENTHERM::hasMessage() {
  return _mode == MODE_RECEIVED;
}

bool OPENTHERM::isSent() {
  return _mode == MODE_SENT;
}

bool OPENTHERM::isIdle() {
  return _mode == MODE_IDLE;
}

bool OPENTHERM::isError() {
  return _mode == MODE_ERROR_TOUT;
}

void OPENTHERM::_callCallback() {
  if (_callback != NULL) {
    _callback();
    _callback = NULL;
  }
}

// 5 kHz timer
void OPENTHERM::_startReadTimer() {
  cli();
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; //initialize counter value to 0
  // set compare match register for 4kHz increments (1/4 of bit period)
  OCR2A = 99; // = (16*10^6) / (5000*32) - 1 (must be <256)
  TCCR2A |= (1 << WGM21); // turn on CTC mode
  TCCR2B |= (1 << CS21) | (1 << CS20); // Set CS21 & CS20 bit for 32 prescaler
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt
  sei();
}

// 2 kHz timer
void OPENTHERM::_startWriteTimer() {
  cli();
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; //initialize counter value to 0
  // set compare match register for 2080Hz increments (2kHz to do transition in the middle of the bit)
  OCR2A = 252;// = (16*10^6) / (2080*32) - 1 (must be <256)
  TCCR2A |= (1 << WGM21); // turn on CTC mode
  TCCR2B |= (1 << CS21) | (1 << CS20); // Set CS21 & CS20 bit for 32 prescaler
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt
  sei();
}

// 1 kHz timer
void OPENTHERM::_startTimeoutTimer() {
  cli();
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; //initialize counter value to 0
  // set compare match register for 1kHz increments
  OCR2A = 249; // = (16*10^6) / (1000*64) - 1 (must be <256)
  TCCR2A |= (1 << WGM21); // turn on CTC mode
  TCCR2B |= (1 << CS22); // Set CS22 bit for 64 prescaler
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt
  sei();
}

void OPENTHERM::_stopTimer() {
  cli();
  TIMSK2 = 0;
  sei();
}

// https://stackoverflow.com/questions/21617970/how-to-check-if-value-has-even-parity-of-bits-or-odd
bool OPENTHERM::_checkParity(unsigned long val) {
  val ^= val >> 16;
  val ^= val >> 8;
  val ^= val >> 4;
  val ^= val >> 2;
  val ^= val >> 1;
  return (~val) & 1;
}

void OPENTHERM::printToSerial(OpenthermData &data) {
  if (data.type == OT_MSGTYPE_READ_DATA) {
    Serial.print("ReadData");
  }
  else if (data.type == OT_MSGTYPE_READ_ACK) {
    Serial.print("ReadAck");
  }
  else if (data.type == OT_MSGTYPE_WRITE_DATA) {
    Serial.print("WriteData");
  }
  else if (data.type == OT_MSGTYPE_WRITE_ACK) {
    Serial.print("WriteAck");
  }
  else if (data.type == OT_MSGTYPE_INVALID_DATA) {
    Serial.print("InvalidData");
  }
  else if (data.type == OT_MSGTYPE_DATA_INVALID) {
    Serial.print("DataInvalid");
  }
  else if (data.type == OT_MSGTYPE_UNKNOWN_DATAID) {
    Serial.print("UnknownId");
  }
  else {
    Serial.print(data.type, BIN);
  }
  Serial.print(" ");
  Serial.print(data.id);
  Serial.print(" ");
  Serial.print(data.valueHB, HEX);
  Serial.print(" ");
  Serial.print(data.valueLB, HEX);
}

float OpenthermData::f88() {
  float value = (int8_t) valueHB;
  return value + (float)valueLB / 256.0;
}

void OpenthermData::f88(float value) {
  valueHB = (byte) value;
  float fraction = (value - valueHB);
  valueLB = fraction * 256.0;
}

uint16_t OpenthermData::u16() {
  uint16_t value = valueHB;
  return (value << 8) | valueLB;
}

void OpenthermData::u16(uint16_t value) {
  valueLB = value & 0xFF;
  valueHB = (value >> 8) & 0xFF;
}

int16_t OpenthermData::s16() {
  int16_t value = valueHB;
  return (value << 8) | valueLB;
}

void OpenthermData::s16(int16_t value) {
  valueLB = value & 0xFF;
  valueHB = (value >> 8) & 0xFF;
}
