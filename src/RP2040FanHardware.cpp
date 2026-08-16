#include "RP2040FanHardware.h"
#include <Arduino.h>
#include "hardware.h"

void RP2040FanHardware::configurePwm(uint32_t freqHz)
{
    // Global fuer den Core, deshalb nur einmal aus FanModule::setup() aufrufen.
    if (freqHz < 500) freqHz = 500;
    if (freqHz > 20000) freqHz = 20000;

    analogWriteFreq(freqHz);
    analogWriteRange(PwmRange);
}

void RP2040FanHardware::init(int8_t pinDrive, int8_t pinDriveMirror, int8_t pinSwitch, int8_t pinTacho)
{
    _pinDrive = pinDrive;
    _pinDriveMirror = pinDriveMirror;
    _pinSwitch = pinSwitch;
    _pinTacho = pinTacho;

    if (_pinDrive >= 0) pinMode(_pinDrive, OUTPUT);
    if (_pinDriveMirror >= 0) pinMode(_pinDriveMirror, OUTPUT);
    if (_pinSwitch >= 0) pinMode(_pinSwitch, OUTPUT);

    stop();
}

void RP2040FanHardware::beginTacho()
{
    // Der Interrupt landet auf dem Core, der ihn registriert - deshalb nicht in init().
    if (_pinTacho >= 0) _tacho.begin((uint8_t)_pinTacho);
}

void RP2040FanHardware::writeDuty(uint8_t dutyPercent)
{
    if (dutyPercent > 100) dutyPercent = 100;

    // Bewusst kein Sonderfall fuer 0: bei dieser Ansteuerung ist 0 % volle Leistung
    // Richtung A und damit ein gueltiger Betriebspunkt, kein "aus".
    uint32_t duty = (uint32_t)dutyPercent * PwmRange / 100;

#ifdef FAN_PWM_ACTIVE_LOW
    // Invertierende Endstufe (Level-Shifter auf NMOS mit Pullup): das Tastverhaeltnis am Pin
    // ist das Gegenstueck zu dem, was der Luefter sehen soll. Die Umkehr passiert bewusst
    // erst hier - alles darueber rechnet in Werten, die am Luefter ankommen.
    duty = PwmRange - duty;
#endif

    // Der Spiegel-Ausgang bekommt exakt dasselbe Signal (zweiter Luefter desselben Geraetes).
    if (_pinDrive >= 0) analogWrite(_pinDrive, duty);
    if (_pinDriveMirror >= 0) analogWrite(_pinDriveMirror, duty);
}

void RP2040FanHardware::drive(Fan::Direction dir, uint8_t speedPercent)
{
    if (speedPercent == 0)
    {
        stop();
        return;
    }

    if (speedPercent > 100) speedPercent = 100;

    uint8_t duty;
    if (dir == Fan::Direction::A && _midpoint > 0)
    {
        // Untere Haelfte: von der Mittelstellung Richtung 0.
        duty = (uint8_t)(_midpoint - (uint32_t)speedPercent * _midpoint / 100);
    }
    else
    {
        // Obere Haelfte: von der Mittelstellung Richtung 100.
        // Auch der Fall Mittelstellung 0 (gewoehnlicher Luefter) laeuft hier durch.
        duty = (uint8_t)(_midpoint + (uint32_t)speedPercent * (100 - _midpoint) / 100);
    }

    writeDuty(duty);
    if (_pinSwitch >= 0) digitalWrite(_pinSwitch, HIGH);
}

void RP2040FanHardware::stop()
{
    // Sicherer Zustand ist die Mittelstellung, nicht 0 %.
    writeDuty(_midpoint);
    if (_pinSwitch >= 0) digitalWrite(_pinSwitch, LOW);
}
