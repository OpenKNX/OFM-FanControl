#pragma once
#include "Fan.h"
#include "IFanHardware.h"

class FawasAirSolitaire : public Fan {
public:
    FawasAirSolitaire(IFanHardware& hw, uint8_t pwm_pin, uint8_t sw_pin);

    void changeFanSpeedDelegate(int16_t fanSpeed) override;
    int16_t getFanSpeed() override;

    // Full control — native control model for this fan
    void setFullControlPower(bool on) override;
    void setFullControlSpeed(uint8_t percent) override;
    void setFullControlDirection(uint8_t dir) override;
    uint8_t getFullControlSpeed() override;

protected:
    void updateMode() override;

private:
    const uint8_t _pwmPin;
    const uint8_t _swPin;

    uint8_t _speedPercent = 0;
    int8_t _direction = 1; // 1=Zuluft(A), -1=Abluft(B)

    // Step-based speed for Manual/Automatic modes (maps to percent internally)
    int16_t _fanStep = 0;
    static constexpr std::array<int16_t, 6> _FanSteps = {0, 20, 40, 60, 80, 100};

    void applyPWM();
    static int16_t getPWMLevel(int16_t fraction, int16_t base = 24, int16_t resolution = 1024);
};
