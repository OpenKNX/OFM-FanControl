#pragma once

#include "OpenKNX.h"
#include "FanTypes.h"
#include "IFanHardware.h"

/**
 * @brief Ein Luefterknoten. Kleinste adressierbare Einheit, in der ETS ein Kanal.
 *
 * Master und Slave benutzen dieselbe Klasse; die Rolle entscheidet der Parameter "Ist Master".
 * Der Knoten leitet seine Foerderrichtung selbst aus Zuordnung (Phase) und Taktzustand ab.
 */
class FanChannel : public OpenKNX::Channel
{
  public:
    /** Persistenter Teil des Kanalzustands. Wird von FanModule in den Flash geschrieben. */
    struct PersistentState
    {
        // Bit0 = Freigabe-Latch gesperrt, Bit1 = suspendiert,
        // Bit2 = Freigabe wurde schon einmal empfangen (E-1f)
        uint8_t flags;
        uint32_t runSeconds;   // Betriebssekunden
    };

    FanChannel(uint8_t index, IFanHardware &hardware);

    const std::string name() override { return "Fan"; }

    void setup();
    void loop();
    void processInputKo(GroupObject &ko);

    /** true, wenn der Kanal in der ETS aktiviert ist (Kanaltyp != Deaktiviert). */
    bool isActive() const { return _type != Fan::ChannelType::Disabled; }

    /** Drehzahl aus der Messung auf Core 1 uebernehmen. */
    void setMeasuredRpm(uint16_t rpm, uint32_t pulseCount);

    PersistentState persistentState() const;
    void restore(const PersistentState &state);

    /** true genau einmal, nachdem sich ein persistenter Wert geaendert hat. */
    bool consumePersistDirty()
    {
        const bool dirty = _persistDirty;
        _persistDirty = false;
        return dirty;
    }

  private:
    // --- Ablauf ---
    void updateRole();
    void updateEnable();
    void applyOutput();
    void runMaster();
    void publish();
    void updateDiagnostics();

    // --- Rechnen ---
    uint8_t targetPower() const;
    uint8_t groupPower() const;
    uint8_t controlOutput() const;
    uint8_t powerToDrive(uint8_t power) const;
    Fan::Direction desiredDirection() const;
    int32_t rpmToFlow(uint16_t rpm) const;
    Fan::Fault activeFault() const;

    // --- Senden mit Totband und Mindestabstand ---
    bool passesDeadband(int32_t value, int32_t lastSent, uint8_t percentBand, uint16_t absBand) const;

    IFanHardware &_hw;

    // Konfiguration, in setup() eingelesen
    Fan::ChannelType _type = Fan::ChannelType::Disabled;
    bool _isMaster = false;
    bool _hasTacho = false;
    bool _configFault = false;

    // Zustand
    Fan::State _state = Fan::State::Off;
    Fan::DirMode _dirMode = Fan::DirMode::Reversing;
    Fan::Direction _direction = Fan::Direction::A;
    Fan::Direction _pendingDirection = Fan::Direction::A;
    uint8_t _powerSet = 0;      // empfangene Gruppenvorgabe
    float _ctrlValue = 0.0f;    // Istwert fuer die interne Regelung
    bool _ctrlValueSeen = false;
    uint8_t _powerAct = 0;      // tatsaechlich kommandierte Leistung
    uint8_t _driveAct = 0;      // ausgegebene Stellgroesse
    bool _tact = false;
    bool _suspended = false;
    bool _enableLatched = true;      // false = gesperrt (selbsthaltend)
    bool _enableSeenEver = false;    // persistent: Freigabe-Objekt ist verknuepft (E-1f)
    bool _enableWatchRunning = false; // Ueberwachungszeit laeuft
    bool _invalidValue = false;
    bool _masterTimeout = false;
    bool _persistDirty = false;

    // Zeitmarken
    uint32_t _stateSince = 0;
    uint32_t _lastEnableSeen = 0;
    uint32_t _lastMasterSeen = 0;
    uint32_t _boostUntil = 0;
    uint32_t _cycleStarted = 0;
    uint32_t _lastMasterSend = 0;
    uint32_t _lastSecondTick = 0;
    uint32_t _runSeconds = 0;

    // Drehzahl und Blockiererkennung
    uint16_t _rpm = 0;
    uint32_t _lastPulseCount = 0;
    uint32_t _blockWindowStart = 0;
    uint32_t _blockWindowPulses = 0;
    uint8_t _emptyWindows = 0;
    bool _blocked = false;

    // Letzte gesendete Werte fuer die Sendebedingungen
    int32_t _lastSentRpm = INT32_MIN;
    int32_t _lastSentFlow = INT32_MIN;
    uint32_t _lastSentRpmAt = 0;
    uint32_t _lastSentFlowAt = 0;
    uint8_t _lastSentPower = 0xFF;
    uint8_t _lastSentFault = 0xFF;
    bool _publishedOnce = false;

    // Nachgefuehrte Diagnosewerte (geschrieben, nicht gesendet)
    uint32_t _lastRunHoursWrite = 0;
    uint32_t _lastCycleRestWrite = 0;
    uint8_t _lastWrittenMasterOk = 0xFF;
    uint8_t _lastWrittenSuspended = 0xFF;
};
