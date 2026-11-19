#import "../lib.typ": *

= Kalibrierung <calibration>
== Warum
tg-expert misst die Schläge mit dem Takt der Soundkarte. Dieser Takt stammt
von einem billigen Quarz und kann um mehrere Sekunden pro Tag vor- oder
nachgehen. Dieser Fehler geht direkt in die gemessene Gangabweichung ein. Die
Kalibrierung misst ihn gegen eine Quarzreferenz, und das Programm zieht ihn
von da an ab.
Amplitude und Abfallfehler hängen nicht von der Kalibrierung ab.
== Wie
Sie benötigen eine Quarzuhr oder Quarzwanduhr, die einmal pro Sekunde tickt,
zum Beispiel eine analoge Quarzarmbanduhr mit Sekundenzeiger. Ihre Genauigkeit
bestimmt die Genauigkeit der Kalibrierung, typischerweise besser als 0.5 s/t.
1. Die Quarzuhr ganz nah an das Mikrofon legen, am besten direkt daran.
2. In einem ruhigen Raum arbeiten. Lüfter und Lautsprecher in der Nähe
  ausschalten und den Tisch nicht berühren.
3. Im Menü #ui[Kalibrierung starten] wählen. Die rechte Fensterseite zeigt die
  Kalibrierungsanleitung.
4. Den Status in der Ergebnistabelle beobachten:
  - *cal (wait)*: noch kein stabiler Tick pro Sekunde gefunden. Den Aufbau
    prüfen.
  - *cal (acq.) NN %*: Ticks werden gesammelt. Nichts berühren.
  - *cal (done) ±X.X s/d*: abgeschlossen. Der Wert wird in das Feld #ui[cal]
    übernommen und gespeichert.
  - *cal (fail)*: das Signal war zu unregelmäßig. Den Aufbau verbessern und
    erneut starten.
5. Sobald *done* angezeigt wird, im Menü #ui[Kalibrierung beenden] wählen, um
  zur normalen Messung zurückzukehren.
Die Kalibrierung dauert etwa 15 Minuten (900 Ticks). Wiederholen Sie sie
einmal, um das Ergebnis zu bestätigen. Zwei Durchläufe sollten auf wenige
Zehntelsekunden pro Tag übereinstimmen.
#shot("calibration.png", [Kalibrierung läuft, bei 5 %. Das Gangdiagramm zeigt
die Referenzticks, die rechte Seite die Anleitung.])
== Wie lange sie gilt
Der Wert cal gehört zur Soundkarte und ihrem Takt, nicht zur Uhr. Er wird
gespeichert und bei jedem Start verwendet. Kalibrieren Sie erneut:
- wenn Sie den Audioeingang oder die Abtastrate ändern;
- wenn Sie einen anderen Computer oder eine andere Soundkarte verwenden;
- wenn die Raumtemperatur stark abweicht, da der Quarz mit der Temperatur
  etwas driftet;
- von Zeit zu Zeit zur Kontrolle. Einmal im Jahr ist eine sinnvolle
  Gewohnheit.
Der Wert cal lässt sich auch im Feld #ui[cal] eingeben, wenn er von einer
früheren Kalibrierung bekannt ist.
