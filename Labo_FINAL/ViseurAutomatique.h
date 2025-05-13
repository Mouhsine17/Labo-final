#pragma once
#include <AccelStepper.h>

class ViseurAutomatique {
public:
  ViseurAutomatique(int in1, int in2, int in3, int in4, float& distanceRef)
    : _stepper(AccelStepper::FULL4WIRE, in1, in3, in2, in4), _distance(distanceRef) {
    _active = false;
    _angle = 90;
  }

  void setPasParTour(int steps) {
    _stepsPerRev = steps;
  }

  void setAngleMin(int min) { _angleMin = min; }
  void setAngleMax(int max) { _angleMax = max; }
  void setDistanceMinSuivi(int dmin) { _distMin = dmin; }
  void setDistanceMaxSuivi(int dmax) { _distMax = dmax; }

  void activer() { _active = true; }
  void desactiver() { _active = false; }

  void update() {
    if (!_active) return;
    if (_distance < _distMin || _distance > _distMax) return;

    float ratio = (_distance - _distMin) / (_distMax - _distMin);
    _angle = _angleMin + (_angleMax - _angleMin) * ratio;
    long target = (_stepsPerRev * _angle) / 360;

    _stepper.moveTo(target);
    _stepper.setMaxSpeed(1000);
    _stepper.setAcceleration(500);
    _stepper.run();
  }

  float getAngle() const { return _angle; }

private:
  AccelStepper _stepper;
  float& _distance;
  int _stepsPerRev = 2048;
  int _angleMin = 10;
  int _angleMax = 170;
  int _distMin = 30;
  int _distMax = 60;
  float _angle = 90;
  bool _active = true;
};
