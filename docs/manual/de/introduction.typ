#import "../lib.typ": *
= Einführung
tg-expert hört über den Audioeingang des Computers eine mechanische Uhr ab
und misst, wie das Werk läuft. Das Programm erfüllt die Aufgabe einer
Zeitwaage und verwendet dazu ein Mikrofon statt eines eigenen Sensors.
== Was eine Zeitwaage misst
Ein mechanisches Werk erzeugt pro Schwingung der Unruh zwei Geräusche, den
*Tick* und den *Tack*. Jeder Schlag ist eine Gruppe kurzer Klicks der Hemmung.
Aus dem zeitlichen Verlauf dieser Schläge leitet das Programm Folgendes ab:
/ Gangabweichung: wie viel die Uhr vor- oder nachgeht, in Sekunden pro Tag
  (s/t). Eine Gangabweichung von +5 s/t bedeutet, dass die Uhr jeden Tag 5
  Sekunden vorgeht.
/ Amplitude: wie weit die Unruh aus ihrer Ruhelage ausschwingt, in Grad. Ein
  gesundes, voll aufgezogenes Werk zeigt in der Lage Zifferblatt oben typischerweise
  270° bis 310°. Eine niedrige Amplitude weist häufig auf eine schwache
  Zugfeder, verharztes Öl oder Verschleiß hin.
/ Abfallfehler: der zeitliche Unterschied zwischen Tick und Tack, in
  Millisekunden. Null bedeutet, dass die Unruh zentriert ist (die Uhr ist "im
  Abfall").
/ Schlagzahl (bph): die Anzahl der Schläge pro Stunde, für die das Werk
  ausgelegt ist, etwa 18000, 21600, 28800 oder 36000. Die Gangabweichung wird
  gegen diesen Wert berechnet, er muss also stimmen.
/ Hebungswinkel: der Winkel in Grad, über den die Unruh mit der Hemmung in
  Kontakt steht. Er ist eine Eigenschaft des Werks und wird vom Hersteller
  angegeben. Die Amplitude wird aus ihm berechnet, ein falscher Hebungswinkel
  ergibt also eine falsche Amplitude. Gangabweichung und Abfallfehler hängen
  nicht von ihm ab.
== Was tg-expert kann und was nicht
tg-expert kann:
- Gangabweichung, Amplitude und Abfallfehler in Echtzeit und in jeder Lage
  messen;
- die Schlagzahl eines unbekannten Werks schätzen;
- die Schläge in einem Gangdiagramm darstellen, sodass Unregelmäßigkeiten
  sichtbar werden;
- Gangabweichung, Amplitude und Abfallfehler bis zu 100 Stunden lang
  aufzeichnen, mit Statistiken pro Lage der Uhr;
- Aufnahmen von Messungen behalten und in Dateien speichern.
Im Vergleich zu einer Hardware-Zeitwaage gilt:
- *Der Sensor ist entscheidend.* Eine Hardware-Zeitwaage hat einen
  Piezosensor, der an der Uhr festgeklemmt wird. Ein Mikrofon nimmt auch
  Raumgeräusche auf, der Aufbau erfordert daher mehr Sorgfalt (siehe
  @microphone).
- *Der Takt muss kalibriert werden.* Eine Hardware-Zeitwaage hat einen
  eigenen Referenzoszillator. tg-expert verlässt sich auf den Takt der
  Soundkarte, der um mehrere Sekunden pro Tag abweichen kann. Die
  Kalibrierung (siehe @calibration) misst diesen Fehler und gleicht ihn aus.
- *Kein Lagenhalter.* Sie bringen die Uhr selbst in jede Lage und teilen dem
  Programm mit, in welcher Lage sie sich befindet.
- *Die Genauigkeit hängt vom Aufbau ab.* Mit einem guten Mikrofon am Gehäuse,
  einem ruhigen Raum und einer kalibrierten Soundkarte sind die Messwerte auf
  einen Bruchteil einer Sekunde pro Tag reproduzierbar. Bei einem schlechten
  Aufbau können die Zahlen irreführend sein.
