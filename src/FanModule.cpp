#include "FanModule.h"

FanModule openknxFanModule;

// Boards ohne Tachoeingang definieren die Pins nicht.
#ifndef FAN1_TACHO_PIN
    #define FAN1_TACHO_PIN -1
#endif
#ifndef FAN2_TACHO_PIN
    #define FAN2_TACHO_PIN -1
#endif

namespace
{
    struct BoardPins
    {
        int8_t drive;
        int8_t driveMirror;
        int8_t sw;
        int8_t tacho;
    };

    // Reihenfolge entspricht den ETS-Kanaelen. Der Ansteuerpfad traegt Drehzahl und Richtung
    // gemeinsam; der Spiegel-Ausgang fuehrt dasselbe Signal ein zweites Mal heraus, fuer den
    // zweiten Luefter eines Maico-Paares. Boards mit nur einem Ausgang geben -1 an.
    const BoardPins boardPins[2] = {
        {FAN1_S1_PWM_PIN, FAN1_S2_PWM_PIN, FAN1_SW_PIN, FAN1_TACHO_PIN},
        {FAN2_S1_PWM_PIN, FAN2_S2_PWM_PIN, FAN2_SW_PIN, FAN2_TACHO_PIN},
    };
}

// ===========================================================================
// Setup
// ===========================================================================

void FanModule::setup(bool configured)
{
    // Geraeteweit: die PWM-Frequenz ist auf dem RP2040 keine Eigenschaft des einzelnen Pins.
    // Ohne gueltige Konfiguration ein unauffaelliger Vorgabewert.
    RP2040FanHardware::configurePwm(configured ? ParamFAN_PwmFreq : 1000);

    // Die Polaritaet ist eine Board-Eigenschaft und stellt bei diesen Luftern die
    // Foerderrichtung auf den Kopf, wenn sie falsch ist. Deshalb beim Start protokollieren.
#ifdef FAN_PWM_ACTIVE_LOW
    logInfoP("PWM-Ausgang invertiert (Open-Drain mit Pullup)");
#else
    logInfoP("PWM-Ausgang nicht invertiert");
#endif

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
    {
        const BoardPins &pins = boardPins[i];

        // Die Hardware wird immer zugeordnet, damit die Ausgaenge auch bei einem
        // deaktivierten Kanal definiert in der Mittelstellung stehen.
        _hw[i].init(pins.drive, pins.driveMirror, pins.sw);

        if (!configured) continue;

        FanChannel *channel = new FanChannel(i, _hw[i]);
        channel->setup();

        if (!channel->isActive())
        {
            delete channel;
            continue;
        }

        _channel[i] = channel;
    }

    _setupComplete = true;
}

#ifdef OPENKNX_DUALCORE
void FanModule::setup1(bool configured)
{
    // Auf das Setup von Core 0 warten, damit die Kanalkonfiguration steht.
    while (!_setupComplete)
        delay(1);

    if (!configured) return;

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
    {
        if (_channel[i] == nullptr) continue;
        if (boardPins[i].tacho < 0) continue;
        _tacho[i].begin((uint8_t)boardPins[i].tacho);
    }
}

void FanModule::loop1(bool configured)
{
    if (!configured) return;

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
        if (_tacho[i].isEnabled()) _tacho[i].update();
}
#endif

// ===========================================================================
// Ablauf
// ===========================================================================

void FanModule::processAfterStartupDelay()
{
    // Erst ab hier darf ein Luefter anlaufen. Empfangene Werte werden vorher bereits
    // entgegengenommen, sodass der Start sofort mit aktuellen Vorgaben erfolgt.
    _startupDelayDone = true;
}

void FanModule::loop(bool configured)
{
    if (!configured) return;
    if (!_startupDelayDone) return;

    const uint32_t now = millis();
    const bool takeRpm = (now - _lastTachoUpdate) >= Fan::TachoUpdateMs;
    if (takeRpm) _lastTachoUpdate = now;

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
    {
        FanChannel *channel = _channel[i];
        if (channel == nullptr) continue;

        if (takeRpm)
            channel->setMeasuredRpm(_tacho[i].getRPM(), _tacho[i].getPulseCount());

        channel->loop();

        // Freigabe-Latch und Suspendierung muessen einen Spannungsausfall ueberleben,
        // deshalb bei Aenderung sofort sichern und nicht auf das periodische Speichern warten.
        if (channel->consumePersistDirty())
            openknx.flash.save(true);
    }
}

void FanModule::processInputKo(GroupObject &ko)
{
    const int8_t index = FAN_KoCalcChannel(ko.asap());
    if (index < 0 || index >= FAN_ChannelCount) return;

    FanChannel *channel = _channel[index];
    if (channel != nullptr) channel->processInputKo(ko);
}

void FanModule::processBeforeRestart()
{
    writeFlash();
}

// ===========================================================================
// Persistenz
// ===========================================================================

uint16_t FanModule::flashSize()
{
    // Versionsbyte plus je Kanal ein Flagbyte und die Betriebssekunden.
    return 1 + FAN_ChannelCount * 5;
}

void FanModule::writeFlash()
{
    openknx.flash.writeByte(FlashVersion);

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
    {
        FanChannel::PersistentState state = {0x00, 0};
        if (_channel[i] != nullptr) state = _channel[i]->persistentState();

        openknx.flash.writeByte(state.flags);
        openknx.flash.writeInt(state.runSeconds);
    }
}

void FanModule::readFlash(const uint8_t *data, const uint16_t size)
{
    if (size < flashSize()) return; // noch nichts oder ein aelteres Layout gespeichert

    if (openknx.flash.readByte() != FlashVersion)
    {
        logInfoP("Gespeicherte Daten haben eine andere Version, werden verworfen");
        return;
    }

    for (uint8_t i = 0; i < FAN_ChannelCount; i++)
    {
        FanChannel::PersistentState state;
        state.flags = openknx.flash.readByte();
        state.runSeconds = openknx.flash.readInt();

        if (_channel[i] != nullptr) _channel[i]->restore(state);
    }
}
