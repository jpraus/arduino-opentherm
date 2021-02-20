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
volatile int OPENTHERM::_timeoutCounter = -1;

#define STOP_BIT_POS 33

void OPENTHERM::listen(byte pin, int timeout, void (*callback)()) {
  _stop();
  _pin = pin;
  _timeoutCounter = timeout * 5; // timer ticks at 5 ticks/ms
  _callback = callback;

  _listen();
}

void OPENTHERM::_listen() {
  _stopTimer();
  _mode = MODE_LISTEN;
  _active = true;
  _data = 0;
  _bitPos = 0;

  _startReadTimer();
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
    _stopTimer();
    _active = false;
  }
}

void OPENTHERM::_read() {
  _data = 0;
  _bitPos = 0;
  _mode = MODE_READ;
  _capture = 1; // reset counter and add as if read start bit
  _clock = 1; // clock is high at the start of comm
  _startReadTimer(); // get us into 1/4 of manchester code
}

void OPENTHERM::_timerISR() {
  if (_mode == MODE_LISTEN) {
    if (_timeoutCounter == 0) {
      _mode = MODE_ERROR_TOUT;
      _stop();
      return;
    }
    byte value = digitalRead(_pin);
    if (value == 1) { // incoming data (rising signal)
      _read();
    }
    if (_timeoutCounter > 0) {
      _timeoutCounter --;
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

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) // Arduino Uno
ISR(TIMER2_COMPA_vect) { // Timer2 interrupt
  OPENTHERM::_timerISR();
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
#endif // END AVR arduino Uno

#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__) // Arduino Leonardo
ISR(TIMER3_COMPA_vect) { // Timer3 interrupt
  OPENTHERM::_timerISR();
}

// 5 kHz timer
void OPENTHERM::_startReadTimer() {
  cli();
  TCCR3A = 0; // set entire TCCR3A register to 0
  TCCR3B = 0; // same for TCCR3B
  TCNT3  = 0; //initialize counter value to 0
  // set compare match register for 4kHz increments (1/4 of bit period)
  OCR3A = 3199; // = (16*10^6) / (5000*1) - 1 (must be <65536)
  TCCR3B |= (1 << WGM32);  // turn on CTC mode
  TCCR3B |= (1 << CS30);   // No prescaling
  TIMSK3 |= (1 << OCIE3A); // enable timer compare interrupt
  sei();
}

// 2 kHz timer
void OPENTHERM::_startWriteTimer() {
  cli();
  TCCR3A = 0; // set entire TCCR3A register to 0
  TCCR3B = 0; // same for TCCR3B
  TCNT3  = 0; //initialize counter value to 0
  // set compare match register for 2080Hz increments (2kHz to do transition in the middle of the bit)
  OCR3A = 7691;// = (16*10^6) / (2080*1) - 1 (must be <65536)
  TCCR3B |= (1 << WGM32); // turn on CTC mode
  TCCR3B |= (1 << CS30);   // No prescaling
  TIMSK3 |= (1 << OCIE3A); // enable timer compare interrupt
  sei();
}

// 1 kHz timer
void OPENTHERM::_startTimeoutTimer() {
  cli();
  TCCR3A = 0; // set entire TCCR3A register to 0
  TCCR3B = 0; // same for TCCR3B
  TCNT3  = 0; //initialize counter value to 0
  // set compare match register for 1kHz increments
  OCR3A = 15999; // = (16*10^6) / (1000*1) - 1 (must be <65536)
  TCCR3B |= (1 << WGM32); // turn on CTC mode
  TCCR3B |= (1 << CS30);   // No prescaling
  TIMSK3 |= (1 << OCIE3A); // enable timer compare interrupt
  sei();
}

void OPENTHERM::_stopTimer() {
  cli();
  TIMSK3 = 0;
  sei();
}
#endif // END AVR arduino Leonardo

#if defined(__AVR_ATmega4809__) // Arduino Uno Wifi Rev2, Arduino Nano Every
// timer interrupt
ISR(TCB0_INT_vect) {
  OPENTHERM::_timerISR();
  TCB0.INTFLAGS = TCB_CAPT_bm; // clear interrupt flag
}

// 5 kHz timer
void OPENTHERM::_startReadTimer() {
  cli();
  TCB0.CTRLB = TCB_CNTMODE_INT_gc; // use timer compare mode
  TCB0.CCMP = 3199; // value to compare with (16*10^6) / 5000 - 1
  TCB0.INTCTRL = TCB_CAPT_bm; // enable the interrupt
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // use Timer A as clock, enable timer
  sei();
}

// 2 kHz timer
void OPENTHERM::_startWriteTimer() {
  cli();
  TCB0.CTRLB = TCB_CNTMODE_INT_gc; // use timer compare mode
  TCB0.CCMP = 7999; // value to compare with (16*10^6) / 2000 - 1
  TCB0.INTCTRL = TCB_CAPT_bm; // enable the interrupt
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // use Timer A as clock, enable timer
  sei();
}

// 1 kHz timer
void OPENTHERM::_startTimeoutTimer() {
  cli();
  TCB0.CTRLB = TCB_CNTMODE_INT_gc; // use timer compare mode
  TCB0.CCMP = 15999; // value to compare with (16*10^6) / 1000 - 1
  TCB0.INTCTRL = TCB_CAPT_bm; // enable the interrupt
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // use Timer A as clock, enable timer
  sei();
}

void OPENTHERM::_stopTimer() {
  cli();
  TCB0.CTRLA = 0;
  sei();
}
#endif // END ATMega4809 Arduino Uno Wifi Rev2, Arduino Nano Every

#ifdef ESP8266
// 5 kHz timer
void OPENTHERM::_startReadTimer() {
  noInterrupts();
  timer1_attachInterrupt(OPENTHERM::_timerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // 5MHz (5 ticks/us - 1677721.4 us max)
  timer1_write(1000); // 5kHz
  interrupts();
}

// 2 kHz timer
void OPENTHERM::_startWriteTimer() {
  noInterrupts();
  timer1_attachInterrupt(OPENTHERM::_timerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // 5MHz (5 ticks/us - 1677721.4 us max)
  timer1_write(2500); // 2kHz
  interrupts();
}

// 1 kHz timer
void OPENTHERM::_startTimeoutTimer() {
  noInterrupts();
  timer1_attachInterrupt(OPENTHERM::_timerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // 5MHz (5 ticks/us - 1677721.4 us max)
  timer1_write(5000); // 1kHz
  interrupts();
}

void OPENTHERM::_stopTimer() {
  noInterrupts();
  timer1_disable();
  timer1_detachInterrupt();
  interrupts();
}
#endif // END ESP8266

#ifdef SAM
void TC4_Handler() {
  // Check for overflow (OVF) interrupt
  if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF) {
    OPENTHERM::_timerISR();
    REG_TC4_INTFLAG = TC_INTFLAG_OVF; // Clear the OVF interrupt flag
  }
}

void OPENTHERM::_setTimer(uint16_t cc0) {
  noInterrupts();
  // Set up the generic clock (GCLK4) used to clock timers
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                    GCLK_GENDIV_ID(4);   // Select Generic Clock (GCLK) 4
  while (GCLK->STATUS.bit.SYNCBUSY)
    ; // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |         // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |       // Enable GCLK4
                     GCLK_GENCTRL_SRC_DFLL48M | // Set the 48MHz clock source
                     GCLK_GENCTRL_ID(4);        // Select GCLK4
  while (GCLK->STATUS.bit.SYNCBUSY)
    ; // Wait for synchronization

  // Feed GCLK4 to TC4 and TC5
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |     // Enable GCLK4 to TC4 and TC5
                     GCLK_CLKCTRL_GEN_GCLK4 | // Select GCLK4
                     GCLK_CLKCTRL_ID_TC4_TC5; // Feed the GCLK4 to TC4 and TC5
  while (GCLK->STATUS.bit.SYNCBUSY)
    ; // Wait for synchronization

  REG_TC4_COUNT16_CC0 = cc0; // Set the TC4 CC0 register as the TOP value in match frequency mode
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY)
    ; // Wait for synchronization

  NVIC_SetPriority(TC4_IRQn, 0); // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
  NVIC_EnableIRQ(TC4_IRQn);      // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

  REG_TC4_INTFLAG |= TC_INTFLAG_OVF;  // Clear the interrupt flags
  REG_TC4_INTENSET = TC_INTENSET_OVF; // Enable TC4 interrupts

  REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1 | // Set prescaler to 1024, 48MHz/1024 = 46.875kHz
                   TC_CTRLA_WAVEGEN_MFRQ |   // Put the timer TC4 into match frequency (MFRQ) mode
                   TC_CTRLA_ENABLE;          // Enable TC4
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY)
    ; // Wait for synchronization
  interrupts();
}

// 5 kHz timer
void OPENTHERM::_startReadTimer() {
  OPENTHERM::_setTimer(9559);
}

// 2 kHz timer
void OPENTHERM::_startWriteTimer() {
  OPENTHERM::_setTimer(23999);
}

// 1 kHz timer
void OPENTHERM::_startTimeoutTimer() {
  OPENTHERM::_setTimer(47999);
}

void OPENTHERM::_stopTimer() {
  REG_TC4_INTENCLR = TC_INTENCLR_OVF; // Disable TC4 interrupts
}
#endif

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
    OT_DEBUG_SERIAL.print("ReadData");
  }
  else if (data.type == OT_MSGTYPE_READ_ACK) {
    OT_DEBUG_SERIAL.print("ReadAck");
  }
  else if (data.type == OT_MSGTYPE_WRITE_DATA) {
    OT_DEBUG_SERIAL.print("WriteData");
  }
  else if (data.type == OT_MSGTYPE_WRITE_ACK) {
    OT_DEBUG_SERIAL.print("WriteAck");
  }
  else if (data.type == OT_MSGTYPE_INVALID_DATA) {
    OT_DEBUG_SERIAL.print("InvalidData");
  }
  else if (data.type == OT_MSGTYPE_DATA_INVALID) {
    OT_DEBUG_SERIAL.print("DataInvalid");
  }
  else if (data.type == OT_MSGTYPE_UNKNOWN_DATAID) {
    OT_DEBUG_SERIAL.print("UnknownId");
  }
  else {
    OT_DEBUG_SERIAL.print(data.type, BIN);
  }
  OT_DEBUG_SERIAL.print(" ");
  OT_DEBUG_SERIAL.print(data.id);
  OT_DEBUG_SERIAL.print(" ");
  OT_DEBUG_SERIAL.print(data.valueHB, HEX);
  OT_DEBUG_SERIAL.print(" ");
  OT_DEBUG_SERIAL.print(data.valueLB, HEX);
}

float OpenthermData::f88() {
  float value = (int8_t) valueHB;
  return value + (float)valueLB / 256.0;
}

void OpenthermData::f88(float value) {
  if (value >= 0) {
    valueHB = (byte) value;
    float fraction = (value - valueHB);
    valueLB = fraction * 256.0;
  }
  else {
    valueHB = (byte)(value - 1);
    float fraction = (value - valueHB - 1);
    valueLB = fraction * 256.0;
  }
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
