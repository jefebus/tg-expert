#import "../lib.typ": *
= Eine Uhr messen <measuring>
== bph und Hebungswinkel einstellen
1. Schlagzahl und Hebungswinkel des Werks nachschlagen. Werkdatenblätter und
  uhrmacherische Nachschlagewerke nennen beide. Viele gängige Werke haben
  einen Hebungswinkel zwischen 50° und 53°. Der Standardwert von 52° ist eine
  vernünftige Annahme, wenn der Wert unbekannt ist, die Amplitude ist dann
  aber nur eine Näherung.
2. #ui[BEAT] auf die Schlagzahl einstellen oder #ui[Schätzung] wählen. Die
  Schätzung funktioniert bei den meisten Uhren, bei schwachem Signal ist
  jedoch eine falsche Schätzung möglich. Ist die Schlagzahl bekannt, sollte
  sie eingestellt werden.
3. #ui[L.A.] auf den Hebungswinkel einstellen.
Eine Änderung von bph, Hebungswinkel oder cal startet die
Langzeitstatistik neu.
== Gespeicherte Uhren
Für eine Uhr, die Sie oft messen, bph und Hebungswinkel einstellen und die
Speichern-Schaltfläche neben der Uhrenliste drücken. Anschließend einen Namen
vergeben. Wird die Uhr später gewählt, setzt dies beide Werte. Die
Löschen-Schaltfläche entfernt die gewählte Uhr aus der Liste.
== Lagen
Eine Uhr geht in verschiedenen Lagen unterschiedlich, weil die Schwerkraft
unterschiedlich auf die Unruh wirkt. Zeitwaagen verwenden diese
Standardlagen:
#table(
  columns: 2,
  [*Code*], [*Lage*],
  [DU], [Zifferblatt oben],
  [DD], [Zifferblatt unten],
  [CU], [Krone oben],
  [CD], [Krone unten],
  [CL], [Krone links],
  [CR], [Krone rechts],
)
Die Lage in der oberen Leiste wählen, bevor die Uhr in diese Lage gebracht
wird. Die Lage wird mit den Messwerten und den Aufnahmen gespeichert. Nach dem
Drehen der Uhr etwa eine Minute warten, damit sich die Messwerte einpendeln
können.
== Aufnahmen
#ui[Messung festhalten] (Strg+Leertaste) behält die aktuelle Messung in einem
neuen Reiter. Der Reiter trägt den Namen der gewählten Uhr und zeigt die Lage
an. Eine Aufnahme behält die Ergebnistabelle, das Gangdiagramm, die
Signalformen und die Seite #ui[Verlauf] so, wie sie waren.
In einem Aufnahme-Reiter:
- lässt sich der Name im Feld #ui[Aktuelle Aufnahme] ändern;
- lässt sich die Lage in der oberen Leiste ändern;
- wird der Reiter mit dem × in seinem Titel geschlossen.
Ein typischer Ablauf: mit Zifferblatt oben messen, eine Aufnahme machen, die
Uhr auf Krone unten drehen, warten, die nächste Aufnahme machen und so weiter.
Anschließend alle Aufnahmen in einer Datei speichern (siehe @files).
