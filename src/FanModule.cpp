#include "FanModule.h"
#include "IFanHardware.h"
#include "hardware.h"


const std::string FanModule::name() { return "FanModule"; }

const std::string FanModule::version() {
  return std::to_string(FAN_ModuleVersion);
}

void FanModule::setup(bool configured) {
  if(ParamFAN_StatusLED == 1) {
    _fan1Hw.setDigital(STATUS_LED_PIN, true);
  } else {
    _fan1Hw.setDigital(STATUS_LED_PIN, false);
  }

  // Read global hardware config: 0=1x Maico PPB30, 1=2x Fawas AirSolitaire
  uint8_t hwConfig = ParamFAN_HardwareConfig;
  openknx.logger.logWithPrefixAndValues("FanMod", "HardwareConfig=%d (0=Maico, 1=Fawas)", hwConfig);

  if (hwConfig == 1) {
    // 2x Fawas AirSolitaire — one fan per HW channel
    _fan1 = new FawasAirSolitaire(_fan1Hw, FAN1_S1_PWM_PIN, FAN1_SW_PIN);
    _fan2 = new FawasAirSolitaire(_fan2Hw, FAN2_S1_PWM_PIN, FAN2_SW_PIN);
  } else {
    // 1x Maico PPB30 — S1=FAN1_PWM, S2=FAN2_PWM (both HW channels for one push-pull unit)
    _fan1 = new MaicoPPB30(_fan1Hw, FAN1_S1_PWM_PIN, FAN2_S1_PWM_PIN, FAN1_SW_PIN);
    // fan2 not used in Maico mode — create dummy with invalid pins
    _fan2 = new MaicoPPB30(_fan2Hw, FAN2_S1_PWM_PIN, FAN2_S2_PWM_PIN, FAN2_SW_PIN);
  }

  openknx.logger.logWithPrefixAndValues("FanMod", "Fan1 pins: PWM=%d SW=%d, Fan2 pins: PWM=%d SW=%d",
    FAN1_S1_PWM_PIN, FAN1_SW_PIN, FAN2_S1_PWM_PIN, FAN2_SW_PIN);

  _channel[0] = new FanChannel(0, *_fan1);
  _channel[1] = new FanChannel(1, *_fan2);

  for (int i = 0; i < FAN_ChannelCount; i++) {
    _channel[i]->setup(configured);
  }

  _setupComplete = true;
}

#ifdef OPENKNX_DUALCORE
void FanModule::setup1() {
  while (!_setupComplete)
    ; // wait for core 0 to finish setup

#ifdef FAN1_TACHO_PIN
  _tacho[0].begin(FAN1_TACHO_PIN);
#endif
#ifdef FAN2_TACHO_PIN
  _tacho[1].begin(FAN2_TACHO_PIN);
#endif
}

void FanModule::loop1() {
  for (int i = 0; i < FAN_ChannelCount; i++) {
    _tacho[i].update();
  }
}
#endif

void FanModule::loop() {
  if (!openknx.afterStartupDelay())
    return;

  bool anyFanRunning = false;
  for (int i = 0; i < FAN_ChannelCount; i++) {
    _channel[i]->loop();
    if (_channel[i]->getFanSpeed() > 0) {
      anyFanRunning = true;
    }
  }

  if (ParamFAN_StatusLED == 2) {
    _fan1Hw.setDigital(STATUS_LED_PIN, anyFanRunning);
  } else {
    _fan1Hw.setDigital(STATUS_LED_PIN, false);
  }

  // Update RPM KOs every second
  if (delayCheck(_lastRpmUpdate, 1000)) {
    for (int i = 0; i < FAN_ChannelCount; i++) {
      if (_tacho[i].isEnabled()) {
        // Temporarily set _channelIndex for the KO macro
        uint8_t _channelIndex = i;
        KoFAN_CH_TachoRPM.value(_tacho[i].getRPM(), DPT_Value_2_Ucount);
      }
    }
    _lastRpmUpdate = millis();
  }
}

void FanModule::processInputKo(GroupObject &ko) {
  for (int i = 0; i < FAN_ChannelCount; i++) {
    _channel[i]->processInputKo(ko);
  }
}

// void FanModule::loop(bool configured)
// {
//     for(int i = 0; i < FAN_ChannelCount; i++)
//     {
//         channel[i]->loop(configured);
//     }
// }

void FanModule::processAfterStartupDelay() {
  for (int i = 0; i < FAN_ChannelCount; i++) {
    _channel[i]->resetFan();
  }
}

bool FanModule::sendReadRequest(GroupObject &ko) {
  // ensure, that we do not send too many read requests at the same time
  if (delayCheck(readRequestDelay, 300)) // 3 per second
  {
    // we handle input KO and we send only read requests, if KO is uninitialized
    if (!ko.initialized())
      ko.requestObjectRead();
    readRequestDelay = delayTimerInit();
    return true;
  }
  return false;
}

FanModule openknxFanModule;