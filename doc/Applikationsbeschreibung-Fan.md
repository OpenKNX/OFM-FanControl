# Applikationsbeschreibung — Lüftersteuerung (FAN)

Modul zur Steuerung dezentraler Lüfter, einzeln oder im reversierenden Verbund.

Ein Gerät bedient **zwei Knoten**; ein Knoten ist ein Lüfter mit seiner Ansteuerung und in der
ETS ein Kanal. Mehrere Knoten lassen sich über gemeinsame Gruppenadressen zu einer **Gruppe**
zusammenfassen, in der genau ein Knoten die Rolle des **Masters** übernimmt.

> Die Hilfetexte dieser Datei werden vom OpenKNXproducer in die kontextsensitive Hilfe der ETS
> übernommen. Die Marker `<!-- DOC ... -->` sind Kommentare und für den Leser unsichtbar.

## Grundbegriffe

Drei Größen sind strikt getrennt und dürfen nicht verwechselt werden:

| Größe | Bedeutung |
|---|---|
| **Leistung** | Der logische Befehl, 0–100 % |
| **Stellgröße** | Das physikalische Signal am Gerät, 0–100 % — bei den üblichen Lüftern ein Sollwert an den eingebauten Regler |
| **Drehzahl** | Der gemessene Rückmeldewert |

Zwischen Leistung und Stellgröße wird umgerechnet. Eine Umrechnung zwischen Leistung und
Drehzahl findet **nicht** statt: die Steuerung arbeitet stellgrößengesteuert, nicht
drehzahlgeregelt.

**Phase und Gegenphase** ordnen einen Knoten innerhalb seiner Gruppe zu. Knoten in Gegenphase
fördern gegenläufig zu Knoten in Phase. Die Zuordnung ist fest, die tatsächliche Förderrichtung
wechselt mit dem Taktzustand, den der Master vorgibt.

## Allgemein

<!-- DOC HelpContext="PWM-Frequenz" -->
### PWM-Frequenz

Trägerfrequenz des Ansteuersignals, gültig für **alle Kanäle des Gerätes**.

Sie lässt sich nicht je Kanal einstellen, weil die PWM-Frequenz auf dem RP2040 eine Eigenschaft
des gesamten Ausgabepfades ist.

Der passende Wert steht im Datenblatt des Lüfters. Die **Vorgabe ist 1 kHz**, einstellbar
zwischen 500 Hz und 20 kHz. Übliche Bereiche liegen zwischen 1 und 5 kHz; manche Geräte
vertragen bis 10 kHz und mehr. Eine zu hohe Frequenz kann dazu führen, dass der Lüfter das
Signal nicht mehr sauber auswertet, eine zu niedrige kann hörbar werden.

## Kanaldefinition

<!-- DOC HelpContext="Kanaltyp" -->
### Kanaltyp

Legt fest, ob und wie dieser Kanal existiert.

* **Deaktiviert** — der Kanal ist nicht bestückt. Es erscheint kein Kanalreiter und es werden
  keine Kommunikationsobjekte angelegt.
* **Nicht reversibel** — eine Förderrichtung. Es gibt keine Richtungs- und Taktobjekte.
* **Reversibel** — zwei Förderrichtungen. Richtung und Takt sind verfügbar, die Stellgröße wird
  je Richtung eingestellt.

Der Kanaltyp wird **ausschließlich in der Kanalauswahl** eingestellt; auf dem Kanalreiter
erscheint er bewusst nicht noch einmal.

Ob die Hardware eine zweite Förderrichtung überhaupt unterstützt, hängt vom Board und von der
eingestellten Mittelstellung ab; passt der Kanaltyp nicht dazu, meldet das Gerät einen
Konfigurationsfehler.

## Rolle und Zuordnung

<!-- DOC HelpContext="Ist Master" -->
### Ist Master

Genau ein Knoten je Gruppe wird als Master konfiguriert, alle übrigen als Slave.

Der Master erzeugt die Vorgaben der Gruppe und verteilt Leistung, Richtungsart und Taktzustand
über Gruppenadressen. Er ist zugleich die Zeitreferenz und bestimmt den Zeitpunkt des
Richtungswechsels. Zusätzlich sendet er zyklisch ein Lebenszeichen, an dem die Slaves seinen
Ausfall erkennen.

Ein Rollenwechsel zur Laufzeit findet nicht statt.

<!-- DOC HelpContext="Drehzahlrückmeldung vorhanden" -->
### Drehzahlrückmeldung vorhanden

Gibt an, ob die Hardware einen Tachoeingang für diesen Kanal besitzt.

Ohne Rückmeldung entfallen die Objekte für Drehzahl und Volumenstrom, und die Blockiererkennung
ist nicht verfügbar. Es wird kein Ersatz- oder Schätzwert gebildet.

<!-- DOC HelpContext="Zuordnung" -->
### Zuordnung

Ordnet den Knoten innerhalb seiner Gruppe einer der beiden Halbwellen zu.

Knoten in **Gegenphase** fördern gegenläufig zu Knoten in **Phase**. Die Zuordnung ist fest; die
tatsächliche Förderrichtung wechselt mit dem Taktzustand, den der Master vorgibt.

Der übliche Fall ist ein Lüfterpaar, bei dem in Phase und Gegenphase je genau ein Knoten liegt.

Die Einstellung gibt es **nur beim Slave**. Der Master erzeugt den Takt und ist damit die
Bezugsphase; er läuft immer in Phase.

<!-- DOC HelpContext="Anteilsfaktor" -->
### Anteilsfaktor

Bestimmt, welchen Anteil der Gruppenvorgabe dieser Knoten umsetzt.

Der Knoten rechnet: Stellwert = Gruppenvorgabe × Anteilsfaktor. Damit lassen sich unterschiedlich
starke Lüfter in einer Gruppe aufeinander abstimmen.

Der Faktor wird vom Anwender festgelegt und nicht aus Volumenströmen berechnet.

## Sollwertbildung

Nur der Master erzeugt die Vorgabe für die Gruppe. Woher sie stammt, legt ein einziges Feld
fest — die drei Quellen schließen sich gegenseitig aus, es gibt also keine Betriebsart, die
zwischen ihnen umschaltet.

<!-- DOC HelpContext="Sollwert kommt aus" -->
### Sollwert kommt aus

Bestimmt, woraus der Master die Leistungsvorgabe für die Gruppe bildet.

* **Fester Wert** — die Anlage läuft dauerhaft mit einer eingestellten Leistung. Es wird kein
  Kommunikationsobjekt für die Leistung benötigt.
* **Externes Kommunikationsobjekt** — die Vorgabe kommt von außen, etwa aus einer
  Visualisierung, einer Zeitschaltuhr oder einem Logikbaustein.
* **Interne Regelung** — der Master regelt selbst auf einen Messwert, zum Beispiel CO₂ oder
  relative Luftfeuchte.

Unabhängig von der Quelle multipliziert jeder Knoten die Gruppenvorgabe mit seinem eigenen
Anteilsfaktor.

<!-- DOC HelpContext="Feste Leistung" -->
### Feste Leistung

Dauerhafte Leistungsvorgabe für die Gruppe.

Sinnvoll für Anlagen, die schlicht durchlaufen sollen. Die Stoßlüftung hebt diesen Wert
weiterhin vorübergehend an.

<!-- DOC HelpContext="Regelgröße" -->
### Regelgröße

Physikalische Größe, auf die geregelt wird. Die Auswahl bestimmt nur die Interpretation und
die Einheit des Eingangsobjekts — CO₂ in ppm, relative Feuchte in Prozent.

Alle diese Datentypen teilen sich dasselbe Übertragungsformat; es gibt deshalb nur **ein**
Eingangsobjekt, dessen Beschriftung mit dieser Auswahl wechselt.

<!-- DOC HelpContext="Sollwert" -->
### Sollwert

Der Zielwert der Regelung, in der Einheit der gewählten Regelgröße.

Unterhalb dieses Wertes läuft die Anlage mit der Grundlast. Oberhalb steigt die Leistung
proportional an.

<!-- DOC HelpContext="Proportionalband" -->
### Proportionalband

Der Bereich **oberhalb** des Sollwerts, über den die Leistung von der Grundlast auf die
Maximalleistung ansteigt.

Beispiel: Sollwert 800 ppm, Proportionalband 600 ppm — bei 800 ppm läuft die Grundlast, bei
1400 ppm und darüber die Maximalleistung, dazwischen linear.

Ein schmales Band regelt schärfer, neigt aber zu häufigen Leistungswechseln. Für Lüftung ist
ein breites Band meist angenehmer.

<!-- DOC HelpContext="Grundlast" -->
### Grundlast

Leistung, solange der Messwert den Sollwert nicht überschreitet.

Der Wert 0 schaltet die Anlage unterhalb des Sollwerts ab. Für eine Grundlüftung wählt man
stattdessen einen kleinen Wert, damit immer ein Mindestluftwechsel bleibt.

<!-- DOC HelpContext="Maximalleistung" -->
### Maximalleistung

Obergrenze der Regelung. Der Regler geht nie über diesen Wert hinaus, auch wenn der Messwert
das Proportionalband überschreitet.

## Richtungsart

<!-- DOC HelpContext="Richtungsart" -->
### Richtungsart

Legt fest, wie die Gruppe fördert. Nur der Master stellt das ein; die Slaves übernehmen es.

* **Automatisch reversieren** — die Gruppe wechselt im Takt die Förderrichtung. Nur in diesem
  Fall gibt es eine Zykluszeit.
* **Nur Richtung A** / **Nur Richtung B** — dauerhafte Förderung in eine Richtung, kein
  Taktwechsel. Die Zuordnung Phase/Gegenphase ist dann wirkungslos.
* **Über Kommunikationsobjekt** — die Richtungsart kommt zur Laufzeit von außen.

Bei fester Vorgabe sendet der Master die Richtungsart zyklisch an die Gruppe, sodass auch ein
später hinzukommender Knoten sie erhält.

## Stellgröße

Die unterstützten Lüfter werden über **einen einzigen Ansteuerpfad** gefahren, der Drehzahl und
Förderrichtung gemeinsam trägt. Die Mittelstellung bedeutet Stillstand, die beiden Enden volle
Leistung in je einer Richtung:

| Stellgröße | Wirkung |
|---|---|
| 0 % | volle Leistung Richtung A |
| 50 % (Mittelstellung) | Stillstand |
| 100 % | volle Leistung Richtung B |

Eine Leistungsvorgabe größer 0 wird von der Mittelstellung aus in die jeweilige Richtung
abgebildet, begrenzt durch Minimum und Maximum:

```
Richtung A:  Stellgröße = Mitte − Leistungsanteil × Mitte / 100
Richtung B:  Stellgröße = Mitte + Leistungsanteil × (100 − Mitte) / 100
```

Leistung 0 bedeutet Stillstand: die Stellgröße geht auf die Mittelstellung, und sofern die
Hardware einen Lastschalter hat, wird dieser geöffnet.

> **Wichtig:** 0 % Stellgröße ist **nicht** „aus", sondern volle Leistung in Richtung A. Der
> sichere Zustand bei Reset, Störung und Spannungsausfall ist deshalb die Mittelstellung.

<!-- DOC HelpContext="Stellgröße Mittelstellung" -->
### Stellgröße Mittelstellung

Die Stellgröße, bei der der Lüfter steht. Sie trennt die beiden Förderrichtungen.

Für die üblichen reversierenden Lüfter liegt sie bei **50 %**: darunter fördert das Gerät in
Richtung A, darüber in Richtung B.

Der Wert **0 %** stellt einen gewöhnlichen, nicht reversierenden Lüfter ein: dann gibt es keine
untere Hälfte mehr, 0 % ist aus und 100 % volle Leistung.

Dieser Parameter bestimmt zugleich den sicheren Zustand — bei Stillstand, Störung und nach einem
Neustart wird genau dieser Wert ausgegeben.

<!-- DOC HelpContext="Stellgröße Minimum" -->
### Stellgröße Minimum

Kleinste Stellgröße, mit der der Lüfter im Dauerlauf noch stabil dreht.

Unterhalb dieses Wertes wird nie gefahren. Der Wert beschreibt bewusst die **Haltegrenze** und
nicht die Anlaufgrenze; das Losbrechen aus dem Stillstand übernimmt bei Bedarf der Anlaufpuls.

Bei reversiblen Knoten je Richtung einstellbar, weil Zu- und Abluft unterschiedliche
aerodynamische Last sehen.

<!-- DOC HelpContext="Stellgröße Maximum" -->
### Stellgröße Maximum

Stellgröße bei einer Leistungsvorgabe von 100 %.

Ein Wert unter 100 % begrenzt den Lüfter dauerhaft, etwa zur Geräuschreduzierung oder zum
Derating.

Bei reversiblen Knoten je Richtung einstellbar.

## Anlaufpuls

Kurzzeitig erhöhte Stellgröße nach dem Anlauf, um die Haftreibung zu überwinden. Die Parameter
gelten für beide Förderrichtungen gemeinsam: beim Losbrechen liegt noch keine Strömung an, die
aerodynamische Last ist also praktisch nicht vorhanden, und übrig bleibt die Haftreibung des
Motors.

<!-- DOC HelpContext="Anlaufpuls Stellgröße" -->
### Anlaufpuls Stellgröße

Erhöhte Stellgröße unmittelbar nach dem Anlauf.

Der Puls wird bei jedem Übergang von Stillstand auf Lauf ausgelöst, also auch nach einem
Richtungswechsel, weil der Lüfter während der Totzeit steht. Er erscheint nicht in der
Leistungsrückmeldung, da er eine Stellgröße und keine Leistung ist.

**0 schaltet den Anlaufpuls ab.** Lüfter mit eingebautem Regler bewältigen Kommutierung und
Sanftanlauf meist selbst, und ein plötzlicher Sollwertsprung kann einen regelnden Lüfter eher
stören als ihm helfen. Der Puls ist deshalb standardmäßig aus und nur bei Bedarf zu aktivieren.

<!-- DOC HelpContext="Anlaufpuls Dauer" -->
### Anlaufpuls Dauer

Dauer des Anlaufpulses. Danach wird auf den Zielwert zurückgefahren.

Die Dauer ist fest; die Drehzahlrückmeldung wird dafür nicht ausgewertet. Übliche Werte liegen
zwischen 100 und 500 ms.

Eine anliegende Sperre über die Freigabe bricht den Puls sofort ab.

## Takt und Richtungswechsel

<!-- DOC HelpContext="Totzeit" -->
### Totzeit

Pause zwischen Abschalten und Wiederanlauf beim Richtungswechsel.

Der Lüfter steht während dieser Zeit vollständig, bevor er in die andere Richtung anläuft. Das
schützt Motor und Elektronik davor, gegen die noch drehende Masse geschaltet zu werden.

Bei Lüftern mit eingebautem Regler genügen in der Regel ein bis zwei Sekunden.

<!-- DOC HelpContext="Zykluszeit je Richtung" -->
### Zykluszeit je Richtung

Dauer, die die Gruppe in einer Förderrichtung verbleibt, bevor der Master umschaltet.

Nur der Master kennt diesen Wert und gibt die verbleibende Restzeit als Kommunikationsobjekt aus.
Die Slaves folgen ausschließlich dem Taktzustand.

## Überwachung

Beide Überwachungen arbeiten nach dem Ruhestromprinzip: ein ausbleibendes Signal wird als Fehler
gewertet, nicht als Erlaubnis.

<!-- DOC HelpContext="Überwachungszeit Freigabe" -->
### Überwachungszeit Freigabe

Maximales Alter des Freigabesignals.

Bleibt die zyklische Aktualisierung der Freigabe länger als diese Zeit aus, gilt der Knoten als
gesperrt und fährt auf Leistung 0. Ein fehlendes Signal bedeutet niemals „freigegeben".

Die Zeit muss deutlich über dem Sendezyklus des freigebenden Gerätes liegen. **0 schaltet die
Überwachung ab**; das explizite Sperren wirkt weiterhin.

Die Sperre ist selbsthaltend und übersteht einen Spannungsausfall. Sie wird ausschließlich durch
ein empfangenes Freigabe-Telegramm aufgehoben — nicht durch Zeitablauf, nicht durch eine
Quittierung und nicht durch Aus- und Einschalten.

> **Hinweis:** KNX ist kein Sicherheitsbus. Für den Verbund mit einer Feuerstätte ist
> üblicherweise ein fest verdrahteter, potentialfreier Kontakt gefordert. Der Busweg ist dann
> eine Ergänzung, kein Ersatz. Die Festlegung trifft die zuständige Stelle.

<!-- DOC HelpContext="Überwachungszeit Master" -->
### Überwachungszeit Master

Maximales Alter des Lebenszeichens vom Master.

Solange die Zeit nicht abgelaufen ist, behält der Slave bei ausbleibendem Lebenszeichen seinen
letzten Zustand. Danach fährt er auf Leistung 0 und meldet Störung mit dem Fehlercode für
Master-Timeout.

Die Zeit sollte höchstens eine Zykluszeit betragen. Andernfalls fördert ein reversierender Knoten
ohne Takt so lange in eine Richtung, dass die Gebäudebilanz merklich kippt, bevor die Abschaltung
greift. **0 schaltet die Überwachung ab.**

<!-- DOC HelpContext="Sendeabstand Lebenszeichen" -->
### Sendeabstand Lebenszeichen

Abstand, in dem der Master sein Lebenszeichen sendet.

Im selben Zyklus sendet der Master auch Taktzustand, Leistung und Richtungsart erneut. Damit
findet ein Knoten, der neu dazukommt oder ein Telegramm verpasst hat, spätestens nach einem
Zyklus wieder in den richtigen Takt — ohne dass er dafür Werte abfragen müsste.

Der Wert muss deutlich unter der Überwachungszeit der Slaves liegen; ein Drittel davon ist ein
brauchbarer Anhaltspunkt.

## Stoßlüftung

Die Stoßlüftung ist eine Funktion des Masters: der Anstoß hebt für eine feste Zeit die
Gruppenvorgabe an, danach gilt wieder der anliegende Leistungswert. Ein Rückschaltbefehl ist
nicht nötig. Jeder Slave multipliziert die erhöhte Vorgabe wie immer mit seinem eigenen
Anteilsfaktor.

Ein erneuter Anstoß während einer laufenden Stoßlüftung startet die Zeit neu. Die Richtungsart
bleibt unverändert.

<!-- DOC HelpContext="Stoßlüftung Laufzeit" -->
### Stoßlüftung Laufzeit

Dauer eines Stoßlüftungs-Anstoßes.

Nach Ablauf fällt die Gruppenvorgabe selbsttätig auf den anliegenden Leistungswert zurück. Ein
Telegramm mit dem Wert 0 auf dem Stoßlüftungsobjekt bricht vorzeitig ab, eine anliegende Sperre
über die Freigabe ebenfalls.

<!-- DOC HelpContext="Stoßlüftung Leistung" -->
### Stoßlüftung Leistung

Leistung, die während der Stoßlüftung als Gruppenvorgabe gilt, typisch 100 %.

Da jeder Slave seinen Anteilsfaktor anwendet, wirkt die Stoßlüftung anteilig — ein Knoten mit
50 % Anteil fährt auch im Stoßbetrieb nur die Hälfte.

## Volumenstrom

Der Volumenstrom wird aus der gemessenen Drehzahl über eine Kennlinie ermittelt und ist **rein
anzeigend**; er wirkt nicht auf die Ansteuerung zurück.

Die Kennlinie besteht aus vier Punkten: dem impliziten Nullpunkt, zwei frei platzierbaren
Zwischenpunkten und dem Endpunkt. Zwischen den Punkten wird linear interpoliert. Weil Lüfter
zwischen Drehzahl und Volumenstrom keinen linearen Zusammenhang haben, legt man die
Zwischenpunkte dorthin, wo die Kennlinie am stärksten gekrümmt ist.

Bei reversiblen Knoten werden die Punkte je Richtung getrennt eingegeben und **nicht** gespiegelt.

<!-- DOC HelpContext="Volumenstrom invertieren" -->
### Volumenstrom invertieren

Dreht das Vorzeichen des ausgegebenen Volumenstroms.

Die Konvention lautet: **positiv = Zuluft** in den Raum, **negativ = Abluft**. Mit diesem Schalter
wird die Einbaulage bzw. die Verdrahtung an die Konvention angepasst.

Dadurch lassen sich die Werte mehrerer Knoten aufsummieren; eine Summe nahe 0 bedeutet eine
ausgeglichene Bilanz. Die Steuerung selbst regelt nicht danach.

<!-- DOC HelpContext="Zwischenpunkt 1: Drehzahl" -->
### Zwischenpunkt 1: Drehzahl

Drehzahl des ersten frei platzierbaren Punktes der Kennlinie.

Der Wert muss zwischen 0 und der maximalen Drehzahl liegen und unter dem zweiten Zwischenpunkt.

<!-- DOC HelpContext="Zwischenpunkt 1: Volumenstrom" -->
### Zwischenpunkt 1: Volumenstrom

Volumenstrom, der zur Drehzahl des ersten Zwischenpunktes gehört. Der Wert stammt üblicherweise
aus dem Datenblatt des Lüfters.

<!-- DOC HelpContext="Zwischenpunkt 2: Drehzahl" -->
### Zwischenpunkt 2: Drehzahl

Drehzahl des zweiten frei platzierbaren Punktes der Kennlinie.

Muss über dem ersten Zwischenpunkt und unter der maximalen Drehzahl liegen.

<!-- DOC HelpContext="Zwischenpunkt 2: Volumenstrom" -->
### Zwischenpunkt 2: Volumenstrom

Volumenstrom, der zur Drehzahl des zweiten Zwischenpunktes gehört.

<!-- DOC HelpContext="Endpunkt: maximale Drehzahl" -->
### Endpunkt: maximale Drehzahl

Maximale Drehzahl des Lüfters, oberer Endpunkt der Kennlinie.

Oberhalb dieses Punktes wird **nicht extrapoliert**, sondern begrenzt: es gilt der Volumenstrom
des Endpunktes. Reale Geräte drehen fertigungs- und betriebsbedingt geringfügig schneller als
angegeben; die Begrenzung hält die Anzeige an der Bereichsgrenze stabil. Eine Meldung erfolgt
dabei nicht, dieser Fall ist normal.

Bleibt der Wert 0, gilt die Kennlinie als nicht hinterlegt und die Volumenstromausgabe entfällt.

<!-- DOC HelpContext="Endpunkt: Volumenstrom" -->
### Endpunkt: Volumenstrom

Volumenstrom bei maximaler Drehzahl.

Bleibt der Wert 0, gilt die Kennlinie als nicht hinterlegt; es wird kein Ersatzwert und keine
Annahme eines linearen Verlaufs gebildet.

## Sendebedingungen

Eine gemessene Drehzahl schwankt permanent. Ohne Totband würde jeder Knoten dauerhaft senden und
die Linie belasten. Gesendet wird deshalb erst, wenn sich der Wert gegenüber dem letzten
gesendeten um die relative **oder** die absolute Schwelle geändert hat.

<!-- DOC HelpContext="Drehzahl: Änderung um" -->
### Drehzahl: Änderung um

Relatives Totband für das Senden der Drehzahl, bezogen auf den letzten gesendeten Wert.

<!-- DOC HelpContext="Drehzahl: mindestens aber" -->
### Drehzahl: mindestens aber

Absolutes Totband für das Senden der Drehzahl.

Der absolute Wert ist nötig, weil eine rein relative Schwelle in der Nähe des Stillstands
beliebig klein wird und dort ihre Wirkung verliert.

<!-- DOC HelpContext="Volumenstrom: Änderung um" -->
### Volumenstrom: Änderung um

Relatives Totband für das Senden des Volumenstroms.

<!-- DOC HelpContext="Volumenstrom: mindestens aber" -->
### Volumenstrom: mindestens aber

Absolutes Totband für das Senden des Volumenstroms.

Hier besonders wichtig: der Volumenstrom ist vorzeichenbehaftet und geht durch 0, wo eine rein
relative Schwelle wirkungslos wäre.

<!-- DOC HelpContext="Mindest-Sendeabstand" -->
### Mindest-Sendeabstand

Kleinster Abstand zwischen zwei Telegrammen desselben Messwertes.

Wirkt als harte Sperre gegen Dauerverkehr, zusätzlich zu den Totbändern. **0 hebt die Sperre auf.**

## Suspendieren und Stilllegen

Zwei verschiedene Dinge, die nicht verwechselt werden sollten:

* **Kanaltyp „Deaktiviert"** — der Kanal ist nicht bestückt. Er hat keinen Reiter und keine
  Kommunikationsobjekte, kann also auch nichts melden.
* **Suspendiert** — der Kanal bleibt vollständig bestehen, mit allen Kommunikationsobjekten und
  Gruppenadress-Verknüpfungen; nur seine Funktion ruht. Gedacht für Service und Fehlersuche.

Ein suspendierter Knoten fährt auf Leistung 0 und nimmt nicht am Gruppenbetrieb teil. Laufabhängige
Überwachungen ruhen und werden als „nicht verfügbar" gekennzeichnet; es wird **keine** Störung
wegen Nichtlaufens gemeldet. Der Zustand ist über ein Kommunikationsobjekt zur Laufzeit setzbar,
bleibt über einen Spannungsausfall erhalten und wird vom Master nicht überschrieben.

## Störung und Fehlercode

`Störung` ist die Sammelmeldung für Alarmlisten und Visualisierungen. `Fehlercode` nennt die
Ursache. Liegen mehrere Ursachen an, wird die mit der höchsten Priorität gesendet.

| Wert | Bedeutung |
|---|---|
| 0 | kein Fehler |
| 1 | Freigabe fehlt oder Überwachungszeit abgelaufen |
| 2 | Master-Timeout |
| 3 | Konfigurations- bzw. Kennlinienfehler |
| 4 | ungültiger Empfangswert |
| 5 | keine Drehzahl trotz Ansteuerung |
| 6 | Überwachung ausgesetzt (suspendiert) |

Fehlercode 5 fasst den blockierten Rotor und den defekten Drehzahlgeber zusammen: das Symptom ist
in beiden Fällen „angesteuert, aber keine Drehzahl", und unterscheiden lassen sich die Ursachen
ohne zusätzliche Sensorik nicht. Erkannt wird der Zustand zyklisch — soll der Lüfter laufen und
werden in zwei aufeinanderfolgenden Fünf-Sekunden-Fenstern keine Tacho-Pulse gezählt, wird die
Störung gemeldet. Ein einzelnes leeres Fenster löst bewusst nicht aus.

Fehlercode 3 erscheint auch, wenn der eingestellte Kanaltyp nicht zur Hardware passt, etwa
„Reversibel" auf einem Board mit nur einem Ansteuerpfad je Kanal.

Ein Telegramm auf `Quittierung` setzt einen gespeicherten Fehler zurück.
