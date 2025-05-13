#include "Alarm.h"
#include <Arduino.h>

Alarm::Alarm(int rPin, int gPin, int bPin, float* distancePtr)
  : _rPin(rPin), _gPin(gPin), _bPin(bPin), _distance(distancePtr) {
  pinMode(_rPin, OUTPUT);
  pinMode(_gPin, OUTPUT);
  pinMode(_bPin, OUTPUT);
}

void Alarm::update() {
  _currentTime = millis();

  if (_turnOnFlag) {
    _state = WATCHING;
    _turnOnFlag = false;
  }

  if (_turnOffFlag) {
    _state = OFF;
    _turnOffFlag = false;
  }

  switch (_state) {
    case OFF: _offState(); break;
    case WATCHING: _watchState(); break;
    case ON: _onState(); break;
    case TESTING: _testingState(); break;
  }
}

void Alarm::setColourA(int r, int g, int b) {
  _colA[0] = r; _colA[1] = g; _colA[2] = b;
}

void Alarm::setColourB(int r, int g, int b) {
  _colB[0] = r; _colB[1] = g; _colB[2] = b;
}

void Alarm::setVariationTiming(unsigned long ms) {
  _variationRate = ms;
}

void Alarm::setDistance(float d) {
  _distanceTrigger = d;
}

void Alarm::setTimeout(unsigned long ms) {
  _timeoutDelay = ms;
}

void Alarm::turnOff() {
  _turnOffFlag = true;
}

void Alarm::turnOn() {
  _turnOnFlag = true;
}

void Alarm::test() {
  _state = TESTING;
  _testStartTime = _currentTime;
}

AlarmState Alarm::getState() const {
  return _state;
}

void Alarm::_setRGB(int r, int g, int b) {
  analogWrite(_rPin, r);
  analogWrite(_gPin, g);
  analogWrite(_bPin, b);
}

void Alarm::_turnOff() {
  _setRGB(0, 0, 0);
}

void Alarm::_offState() {
  _turnOff();
}

void Alarm::_watchState() {
  if (*_distance <= _distanceTrigger) {
    _state = ON;
    _lastDetectedTime = _currentTime;
  }
}

void Alarm::_onState() {
  if (_currentTime - _lastUpdate >= _variationRate) {
    _currentColor = !_currentColor;
    _setRGB(
      _currentColor ? _colB[0] : _colA[0],
      _currentColor ? _colB[1] : _colA[1],
      _currentColor ? _colB[2] : _colA[2]
    );
    _lastUpdate = _currentTime;
  }

  if (*_distance > _distanceTrigger) {
    if (_currentTime - _lastDetectedTime >= _timeoutDelay) {
      _state = WATCHING;
      _turnOff();
    }
  } else {
    _lastDetectedTime = _currentTime;
  }
}

void Alarm::_testingState() {
  _setRGB(255, 255, 0);
  if (_currentTime - _testStartTime >= 3000) {
    _state = WATCHING;
    _turnOff();
  }
}
