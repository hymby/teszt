#include "LCTechRelayBoard.h"

const uint8_t LCTechRelayBoard::DEFAULT_INPUT_MAP_X8[8] = {
  0x10, // IN1
  0x08, // IN2
  0x20, // IN3
  0x04, // IN4
  0x40, // IN5
  0x02, // IN6
  0x80, // IN7
  0x01  // IN8
};

LCTechRelayBoard::LCTechRelayBoard(
  uint8_t latchPin,
  uint8_t clockPin,
  uint8_t dataOutPin,
  uint8_t dataInPin,
  uint8_t enablePin,
  uint8_t channels,
  bool relayActiveLow,
  const uint8_t* inputMap
)
: _latchPin(latchPin),
  _clockPin(clockPin),
  _dataOutPin(dataOutPin),
  _dataInPin(dataInPin),
  _enablePin(enablePin),
  _channels(channels),
  _relayActiveLow(relayActiveLow)
{
  if (_channels < 1) _channels = 1;
  if (_channels > 8) _channels = 8;

  // Default mapping if not provided: sequential bit mapping (1<<i)
  // Many boards are *not* sequential -> for your X8 board use DEFAULT_INPUT_MAP_X8.
  if (inputMap) {
    _inputMapLen = _channels;
    for (uint8_t i = 0; i < _channels; i++) _inputMap[i] = inputMap[i];
  } else {
    _inputMapLen = _channels;
    for (uint8_t i = 0; i < _channels; i++) _inputMap[i] = (uint8_t)(1 << i);
  }
}

void LCTechRelayBoard::setInputMapping(const uint8_t* mapPtr, uint8_t mapLen) {
  if (!mapPtr) return;
  if (mapLen < 1) return;
  if (mapLen > 8) mapLen = 8;
  _inputMapLen = mapLen;
  for (uint8_t i = 0; i < mapLen; i++) _inputMap[i] = mapPtr[i];
}

void LCTechRelayBoard::begin() {
  pinMode(_latchPin, OUTPUT);
  pinMode(_clockPin, OUTPUT);
  pinMode(_dataOutPin, OUTPUT);
  pinMode(_dataInPin, INPUT);
  pinMode(_enablePin, OUTPUT);

  // Your board required: HIGH then LOW to enable relays
  digitalWrite(_enablePin, HIGH);

  _relayMask = 0;
  _writeRelays();
  delay(50);

  digitalWrite(_enablePin, LOW);
}

void LCTechRelayBoard::_writeRelays() {
  digitalWrite(_latchPin, LOW);

  uint8_t out = _relayMask;
  if (_relayActiveLow) out = (uint8_t)~out;

  shiftOut(_dataOutPin, _clockPin, MSBFIRST, out);

  digitalWrite(_latchPin, HIGH);
}

void LCTechRelayBoard::setAllRelays(uint8_t mask) {
  _relayMask = mask;
  _writeRelays();
}

void LCTechRelayBoard::setRelay(uint8_t ch1_based, bool on) {
  if (ch1_based < 1 || ch1_based > _channels) return;
  uint8_t bit = (uint8_t)(1 << (ch1_based - 1));
  if (on) _relayMask |= bit;
  else    _relayMask &= (uint8_t)~bit;
  _writeRelays();
}

void LCTechRelayBoard::pulse(uint8_t ch1_based, uint16_t ms) {
  if (ms == 0) ms = _defaultPulseMs;
  setRelay(ch1_based, true);
  delay(ms);
  setRelay(ch1_based, false);
}

uint8_t LCTechRelayBoard::readInputsRaw() {
  // LOAD pulse (latch parallel inputs into 74HC165)
  digitalWrite(_latchPin, LOW);
  delayMicroseconds(5);
  digitalWrite(_latchPin, HIGH);

  uint8_t v = 0;
  for (int i = 0; i < 8; i++) {
    v <<= 1;
    if (digitalRead(_dataInPin)) v |= 1;

    digitalWrite(_clockPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(_clockPin, LOW);
    delayMicroseconds(2);
  }

  _inputsRaw = v;
  return _inputsRaw;
}

bool LCTechRelayBoard::inputActive(uint8_t ch1_based) {
  if (ch1_based < 1 || ch1_based > _inputMapLen) return false;
  uint8_t mask = _inputMap[ch1_based - 1];

  bool bitIs1 = (_inputsRaw & mask) != 0;
  if (_inputActiveLow) return !bitIs1;  // active when 0
  return bitIs1;                        // active when 1
}