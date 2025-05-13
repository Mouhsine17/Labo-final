#pragma once

enum AlarmState {
  OFF,
  WATCHING,
  ON,
  TESTING
};

class Alarm {
  public:
    Alarm(int rPin, int gPin, int bPin, float* distancePtr);

    void update();

    void setColourA(int r, int g, int b);
    void setColourB(int r, int g, int b);
    void setVariationTiming(unsigned long ms);
    void setDistance(float d);
    void setTimeout(unsigned long ms);

    void turnOff();
    void turnOn();
    void test();

    AlarmState getState() const;

  private:
    int _rPin, _gPin, _bPin;
    unsigned long _currentTime = 0;

    int _colA[3];
    int _colB[3];
    bool _currentColor = false;

    float* _distance;
    float _distanceTrigger = 10.0;

    unsigned long _variationRate = 500;
    unsigned long _timeoutDelay = 3000;
    unsigned long _lastUpdate = 0;
    unsigned long _lastDetectedTime = 0;
    unsigned long _testStartTime = 0;

    AlarmState _state = OFF;

    bool _turnOnFlag = false;
    bool _turnOffFlag = false;

    void _setRGB(int r, int g, int b);
    void _turnOff();

    void _offState();
    void _watchState();
    void _onState();
    void _testingState();
};
