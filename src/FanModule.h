#pragma once

#include "MaicoPPB30.h"
#include "FawasAirSolitaire.h"
#include "FanChannel.h"
#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include "RP2040FanHardware.h"
#include "TachoReader.h"

class FanModule : public OpenKNX::Module {
public:
  void loop() override;
  void setup(bool configured) override;

#ifdef OPENKNX_DUALCORE
  void setup1() override;
  void loop1() override;
#endif

  void processAfterStartupDelay() override;
  void processInputKo(GroupObject &ko) override;
  bool sendReadRequest(GroupObject &ko);

  const std::string name() override;
  const std::string version() override;

private:
  RP2040FanHardware _fan1Hw;
  RP2040FanHardware _fan2Hw;
  Fan* _fan1 = nullptr;
  Fan* _fan2 = nullptr;

  FanChannel *_channel[FAN_ChannelCount];
  uint32_t readRequestDelay = 0;

  TachoReader _tacho[FAN_ChannelCount];
  uint32_t _lastRpmUpdate = 0;
  volatile bool _setupComplete = false;
};

// Wir benutzen das, um in main besser auf das Modul zugreifen zu können
extern FanModule openknxFanModule;