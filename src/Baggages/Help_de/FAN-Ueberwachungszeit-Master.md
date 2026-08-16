### Überwachungszeit Master

Maximales Alter des Lebenszeichens vom Master.

Solange die Zeit nicht abgelaufen ist, behält der Slave bei ausbleibendem Lebenszeichen seinen
letzten Zustand. Danach fährt er auf Leistung 0 und meldet Störung mit dem Fehlercode für
Master-Timeout.

Die Zeit sollte höchstens eine Zykluszeit betragen. Andernfalls fördert ein reversierender Knoten
ohne Takt so lange in eine Richtung, dass die Gebäudebilanz merklich kippt, bevor die Abschaltung
greift. **0 schaltet die Überwachung ab.**

