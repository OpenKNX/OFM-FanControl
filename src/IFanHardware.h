#pragma once

#include <stdint.h>
#include "FanTypes.h"

/**
 * @brief Ansteuerung eines Luefterknotens, entkoppelt von der konkreten Hardware.
 *
 * Die unterstuetzten Luefter (Maico PPB30, Fawas HST) tragen Drehzahl UND Foerderrichtung
 * auf **einem** Ansteuerpfad: die Mittelstellung ist Stillstand, das eine Ende volle Leistung
 * Richtung A, das andere volle Leistung Richtung B. Ein Maico enthaelt zwar zwei Luefter,
 * die aber innerhalb des Geraetes immer gleich herum drehen und deshalb gemeinsam an einem
 * Ausgang haengen.
 *
 * Daraus folgt die wichtigste Eigenschaft dieser Schnittstelle: **0 % Stellgroesse ist nicht
 * "aus", sondern volle Leistung Richtung A.** Der sichere Zustand ist die Mittelstellung.
 */
class IFanHardware
{
  public:
    virtual ~IFanHardware() = default;

    /**
     * @brief Pins zuordnen und die Ausgaenge in den sicheren Zustand bringen.
     *
     * @param pinDrive       Ansteuerpfad, traegt Drehzahl und Richtung
     * @param pinDriveMirror zweiter Ausgang mit **identischem** Signal, < 0 wenn nicht vorhanden
     * @param pinSwitch      Lastschalter, < 0 wenn nicht vorhanden
     *
     * Der Spiegel-Ausgang ist keine zweite Richtung, sondern dieselbe Ansteuerung ein zweites
     * Mal: das MrSpieb-Board fuehrt je Knoten zwei Ausgaenge heraus, einen je Luefter eines
     * Maico-Paares. Beide Luefter eines Geraetes drehen ohnehin immer gleich herum. Boards mit
     * nur einem Ausgang (Reg1 Fan-Addon-X2) klemmen beide Luefter auf dieselbe Klemme, was der
     * hochohmige PWM-Eingang der Luefter erlaubt.
     */
    virtual void init(int8_t pinDrive, int8_t pinDriveMirror, int8_t pinSwitch, int8_t pinTacho) = 0;

    /** @brief true, wenn dieses Board fuer diesen Knoten einen Tacho-Eingang hat. */
    virtual bool hasTacho() const = 0;

    /**
     * @brief Tacho-Eingang scharf schalten. Aus dem Kontext aufrufen, der messen soll.
     *
     * Getrennt von init(), weil der Interrupt auf dem Core landet, der ihn registriert -
     * die Messung laeuft auf Core 1, die Ansteuerung auf Core 0.
     */
    virtual void beginTacho() = 0;

    /** @brief Messung fortschreiben. Aus demselben Kontext wie beginTacho(). */
    virtual void updateTacho() = 0;

    /** @brief Letzte gemessene Drehzahl. Aus einem anderen Core lesbar. */
    virtual uint16_t rpm() const = 0;

    /** @brief Monotoner Pulszaehler fuer die Blockiererkennung. */
    virtual uint32_t tachoPulses() const = 0;

    /**
     * @brief Mittelstellung setzen (Stellgroesse bei Stillstand).
     *
     * 50 % entspricht einem reversierenden Luefter, 0 % einem gewoehnlichen Luefter,
     * bei dem 0 aus und 100 volle Leistung bedeutet.
     */
    virtual void setMidpoint(uint8_t percent) = 0;

    /**
     * @brief Drehzahl in der angegebenen Richtung ausgeben.
     * @param speedPercent 0..100, bezogen auf die jeweilige Haelfte. 0 ergibt Stillstand.
     */
    virtual void drive(Fan::Direction dir, uint8_t speedPercent) = 0;

    /** @brief Sicherer Zustand: Mittelstellung ausgeben und den Lastschalter oeffnen. */
    virtual void stop() = 0;
};
