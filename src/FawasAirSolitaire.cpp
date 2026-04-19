#include "FawasAirSolitaire.h"

constexpr std::array<int16_t, 6> FawasAirSolitaire::_FanSteps;

FawasAirSolitaire::FawasAirSolitaire(IFanHardware& hw, uint8_t pwm_pin, uint8_t sw_pin)
    : Fan(hw), _pwmPin(pwm_pin), _swPin(sw_pin) {
    // Pass 0xFF for unused S2 pin — RP2040 silently ignores invalid pins
    _hw.init(_pwmPin, 0xFF, _swPin);
    applyPWM();
}

void FawasAirSolitaire::changeFanSpeedDelegate(int16_t fanSpeed) {
    // Clamp speed (0-5)
    if (fanSpeed < 0) fanSpeed = 0;
    if (fanSpeed > 5) fanSpeed = 5;

    _fanStep = _FanSteps[fanSpeed];
    _speedPercent = _fanStep; // _FanSteps values are already in percent
    updateMode();
}

int16_t FawasAirSolitaire::getFanSpeed() {
    // Reverse lookup from step to speed index
    int count = 0;
    for (auto& fs : _FanSteps) {
        if (fs == _fanStep)
            return count;
        count++;
    }
    return 0;
}

void FawasAirSolitaire::updateMode() {
    // Full control bypasses normal step-based logic
    if (_operatingMode == OperatingMode::FullControl) {
        return;
    }

    if (_operatingMode == OperatingMode::Off) {
        _speedPercent = 0;
        _hw.setDigital(_swPin, false);
    } else {
        _hw.setDigital(_swPin, true);
    }

    applyPWM();
}

// Full control methods — native control model for this fan
void FawasAirSolitaire::setFullControlPower(bool on) {
    _hw.setDigital(_swPin, on);
}

void FawasAirSolitaire::setFullControlSpeed(uint8_t percent) {
    if (percent > 100) percent = 100;
    _speedPercent = percent;
    applyPWM();
}

void FawasAirSolitaire::setFullControlDirection(uint8_t dir) {
    _direction = (dir == 0) ? 1 : -1;
    applyPWM();
}

uint8_t FawasAirSolitaire::getFullControlSpeed() {
    return _speedPercent;
}

// PWM mapping (matches FanActor_TestFirmware):
//   Center (50% duty) = stopped
//   Direction A (+): duty = (100 + speed) / 200  -> fraction = 12 + offset
//   Direction B (-): duty = (100 - speed) / 200  -> fraction = 12 - offset
//   MOSFET inverts: GPIO high = fan PWM low
void FawasAirSolitaire::applyPWM() {
    int16_t offset = (int16_t)_speedPercent * 12 / 100;
    _hw.setPWM(_pwmPin, getPWMLevel(12 + _direction * offset));
}

int16_t FawasAirSolitaire::getPWMLevel(int16_t fraction, int16_t base, int16_t resolution) {
    return (fraction * resolution) / base;
}
