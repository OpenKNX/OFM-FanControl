#include "FanModule.h"

FanModule openknxFanModule;

#ifndef FAN_BOARD_PIN_TABLE
    #error "Das Geraete-Header muss FAN_BOARD_CHANNELS und FAN_BOARD_PIN_TABLE definieren."
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

    // Die Pinbelegung ist eine Eigenschaft des Boards und kommt deshalb aus dem
    // Geraete-Header, nicht aus diesem Modul. Der Ansteuerpfad traegt Drehzahl und Richtung
    // gemeinsam; der Spiegel-Ausgang fuehrt dasselbe Signal ein zweites Mal heraus, fuer den
    // zweiten Luefter eines Maico-Paares. Boards mit nur einem Ausgang geben -1 an.
    const BoardPins boardPins[] = FAN_BOARD_PIN_TABLE;

    constexpr uint8_t BoardChannels = (uint8_t)(sizeof(boardPins) / sizeof(boardPins[0]));

    static_assert(BoardChannels == FAN_BOARD_CHANNELS,
                  "FAN_BOARD_CHANNELS passt nicht zur Laenge von FAN_BOARD_PIN_TABLE.");
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

    // Die Hardware wird fuer alle vorhandenen Ausgaenge zugeordnet, damit sie auch bei einem
    // ungenutzten Kanal definiert in der Mittelstellung stehen.
    for (uint8_t i = 0; i < BoardChannels; i++)
        _hw[i].init(boardPins[i].drive, boardPins[i].driveMirror, boardPins[i].sw,
                    boardPins[i].tacho);

    if (!configured)
    {
        _setupComplete = true;
        return;
    }

    // Die ETS erlaubt bis zu FAN_ChannelCount Luefter - das ist eine Eigenschaft der
    // Applikation. Wie viele davon Pins haben, weiss nur das Board. Ueberzaehlige Kanaele
    // werden gemeldet und nicht angelegt, statt stumm nichts zu tun.
    if (ParamFAN_FanCount > BoardChannels)
        logErrorP("%u Luefter konfiguriert, das Board hat nur %u Ausgaenge - die uebrigen bleiben aus",
                  (unsigned)ParamFAN_FanCount, (unsigned)BoardChannels);

    for (uint8_t i = 0; i < BoardChannels; i++)
    {
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

    // Der Tacho gehoert dem Kanal, nicht dem Modul: das Modul sagt nur, wann Core 1 dran ist.
    for (uint8_t i = 0; i < BoardChannels; i++)
        if (_channel[i] != nullptr) _channel[i]->setup1();
}

void FanModule::loop1(bool configured)
{
    if (!configured) return;

    for (uint8_t i = 0; i < BoardChannels; i++)
        if (_channel[i] != nullptr) _channel[i]->loop1();
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

    for (uint8_t i = 0; i < BoardChannels; i++)
    {
        FanChannel *channel = _channel[i];
        if (channel == nullptr) continue;

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
    if (index < 0 || index >= BoardChannels) return;

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
    return 1 + BoardChannels * 5;
}

void FanModule::writeFlash()
{
    openknx.flash.writeByte(FlashVersion);

    for (uint8_t i = 0; i < BoardChannels; i++)
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

    for (uint8_t i = 0; i < BoardChannels; i++)
    {
        FanChannel::PersistentState state;
        state.flags = openknx.flash.readByte();
        state.runSeconds = openknx.flash.readInt();

        if (_channel[i] != nullptr) _channel[i]->restore(state);
    }
}
