### Anzahl Lüfter

Wie viele Lüfter das Gerät steuert, einstellbar von 1 bis 8. Es werden genau so viele
Lüfter-Reiter und Zeilen in der Kanalauswahl eingeblendet.

Dieser Wert sagt nur, **wie viele** Lüfter es gibt. Welcher Art ein Lüfter ist, legt „Modus" auf
seinem eigenen Reiter fest.

> **Mehr Lüfter als Ausgänge:** Die Applikation ist auf bis zu 8 Lüfter ausgelegt, die
> tatsächliche Zahl der Ausgänge bestimmt aber die Hardware. Stellt man mehr Lüfter ein, als das
> Gerät treiben kann, laufen die überzähligen nicht — das Gerät meldet beim Start einen
> Konfigurationsfehler und schreibt die vorhandene Zahl ins Log.

Die Kommunikationsobjekte aller 8 möglichen Lüfter sind in der Applikation fest angelegt; ein
kleinerer Wert blendet sie in der ETS aus, verschiebt aber keine Objektnummern.
