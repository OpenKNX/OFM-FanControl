#pragma once

#include "IFanHardware.h"
#include "TachoReader.h"

/**
 * @brief IFanHardware fuer RP2040: ein PWM-Ausgang je Knoten plus optionaler Lastschalter.
 *
 * Der PWM-Ausgang traegt Drehzahl und Richtung gemeinsam (siehe IFanHardware).
 */
class RP2040FanHardware : public IFanHardware
{
  public:
    void init(int8_t pinDrive, int8_t pinDriveMirror, int8_t pinSwitch, int8_t pinTacho) override;

    bool hasTacho() const override { return _pinTacho >= 0; }
    void beginTacho() override;
    void updateTacho() override { if (_tacho.isEnabled()) _tacho.update(); }
    uint16_t rpm() const override { return _tacho.getRPM(); }
    uint32_t tachoPulses() const override { return _tacho.getPulseCount(); }
    void setMidpoint(uint8_t percent) override { _midpoint = percent > 100 ? 100 : percent; }
    void drive(Fan::Direction dir, uint8_t speedPercent) override;
    void stop() override;

    /**
     * @brief PWM-Aufloesung und -Frequenz einmalig setzen.
     *
     * Wirkt global fuer den Core, deshalb ist die Frequenz ein geraeteweiter Parameter und
     * nicht je Kanal einstellbar.
     */
    static void configurePwm(uint32_t freqHz);

  private:
    void writeDuty(uint8_t dutyPercent);

    int8_t _pinDrive = -1;
    int8_t _pinDriveMirror = -1;
    int8_t _pinSwitch = -1;
    int8_t _pinTacho = -1;
    uint8_t _midpoint = 50;
    TachoReader _tacho;

    static constexpr uint16_t PwmRange = 1000;
};
