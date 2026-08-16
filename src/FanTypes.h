#pragma once

#include <stdint.h>

namespace Fan
{
    /** Kanaltyp, entspricht PT-FanChannelType in Fan.share.xml. */
    enum class ChannelType : uint8_t
    {
        Disabled = 0,
        NonReversible = 1,
        Reversible = 2
    };

    /** Richtungsart, empfangen ueber KO "Richtungsart" (DPT 5.010). */
    enum class DirMode : uint8_t
    {
        Reversing = 0,
        OnlyA = 1,
        OnlyB = 2,
        Last = OnlyB
    };

    /** Woher der Master seine Gruppenvorgabe nimmt (Parameter "Sollwert kommt aus"). */
    enum class SetpointSource : uint8_t
    {
        Fixed = 0,
        ExternalKo = 1,
        InternalControl = 2
    };

    /** Richtungsart-Vorgabe in der ETS (Parameter "Richtungsart"). */
    enum class DirModeSel : uint8_t
    {
        Reversing = 0,
        OnlyA = 1,
        OnlyB = 2,
        ViaKo = 3
    };

    /** Physikalische Foerderrichtung. */
    enum class Direction : uint8_t
    {
        A = 0,
        B = 1
    };

    /**
     * Fehlercode, gesendet ueber KO "Fehlercode" (DPT 5.010).
     * Die Reihenfolge ist die Prioritaet: der kleinste anliegende Wert > 0 gewinnt.
     */
    enum class Fault : uint8_t
    {
        None = 0,
        EnableMissing = 1,       // Freigabe fehlt oder Ueberwachungszeit abgelaufen
        MasterTimeout = 2,       // Lebenszeichen des Masters ausgeblieben
        Config = 3,              // Kanaltyp passt nicht zur Hardware, Kennlinie unplausibel
        InvalidValue = 4,        // ungueltiger Empfangswert
        NoRotation = 5,          // angesteuert, aber keine Drehzahl (auch blockierter Rotor)
        MonitoringSuspended = 6  // Ueberwachung ausgesetzt, weil suspendiert
    };

    /** Zustaende der Kanal-Zustandsmaschine. */
    enum class State : uint8_t
    {
        Off,        // Stellgroesse 0
        StartPulse, // Anlaufpuls laeuft
        Running,    // Zielwert wird gefahren
        DeadTime    // Totzeit vor dem Richtungswechsel, Lüfter steht
    };

    /** Blockiererkennung nach S-7: Fensterlaenge und Anzahl leerer Fenster bis zum Fehler. */
    constexpr uint32_t BlockWindowMs = 5000;
    constexpr uint8_t BlockWindowsToFault = 2;

    /**
     * Nachfuehrintervalle der Diagnose-Objekte. Diese Werte werden nur in das KO geschrieben,
     * nicht gesendet: eine Leseanforderung liefert damit den aktuellen Stand, ohne dass
     * zyklischer Busverkehr entsteht.
     */
    constexpr uint32_t RunHoursWriteMs = 30UL * 60UL * 1000UL; // Betriebsstunden
    constexpr uint32_t CycleRestWriteMs = 5000;                // Restzeit des Taktes

    /** Aktualisierungsintervall der Drehzahlmessung auf Core 1. */
    constexpr uint32_t TachoUpdateMs = 1000;
}
