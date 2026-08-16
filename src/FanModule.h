#pragma once

#include "OpenKNX.h"
#include "FanChannel.h"
#include "RP2040FanHardware.h"
#include "TachoReader.h"
#include "hardware.h"
#include "knxprod.h"

/**
 * @brief OpenKNX-Modul der Luftersteuerung. Haelt die Knoten des Geraetes.
 *
 * Core 0 fuehrt die Kanallogik, Core 1 misst die Drehzahl. Die persistenten Werte
 * (Freigabe-Latch, Suspendierung, Betriebssekunden) liegen im Modul-Flashbereich.
 */
class FanModule : public OpenKNX::Module
{
  public:
    void setup(bool configured) override;
    void loop(bool configured) override;
    void processInputKo(GroupObject &ko) override;

#ifdef OPENKNX_DUALCORE
    void setup1(bool configured) override;
    void loop1(bool configured) override;
#endif

    void processAfterStartupDelay() override;
    void processBeforeRestart() override;

    uint16_t flashSize() override;
    void writeFlash() override;
    void readFlash(const uint8_t *data, const uint16_t size) override;

    const std::string name() override { return "FanControl"; }
    const std::string version() override { return MODULE_FanControl_Version; }

  private:
    static constexpr uint8_t FlashVersion = 1;

    // Dimensioniert nach den Ausgaengen des Boards, nicht nach FAN_ChannelCount: die ETS darf
    // mehr Luefter anbieten, als das Board treiben kann, und diese Felder sind physisch.
    RP2040FanHardware _hw[FAN_BOARD_CHANNELS];
    FanChannel *_channel[FAN_BOARD_CHANNELS] = {};
    TachoReader _tacho[FAN_BOARD_CHANNELS];

    uint32_t _lastTachoUpdate = 0;
    volatile bool _setupComplete = false;
    bool _startupDelayDone = false;
};

extern FanModule openknxFanModule;
