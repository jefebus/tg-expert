#import "../lib.typ": *

= Das Hauptfenster <main-window>
#shot("main-window.png", [Das Hauptfenster bei der Messung einer Uhr mit 28800 bph.])
Das Fenster besteht aus einer oberen Leiste, einer Reiterzeile, der
Ergebnistabelle, links dem Gangdiagramm und rechts den Signalform-Anzeigen
oder der Seite Verlauf.
== Die obere Leiste
Von links nach rechts:
/ Uhr: eine Liste gespeicherter Uhren. Die Auswahl einer Uhr setzt deren bph
  und Hebungswinkel. Die Speichern-Schaltfläche legt die aktuellen bph und den
  Hebungswinkel unter einem Namen ab, die Löschen-Schaltfläche entfernt die
  gewählte Uhr.
/ BEAT: die Schlagzahl in bph. Eine Voreinstellung wählen, einen Wert
  eingeben oder #ui[Schätzung] wählen, damit das Programm sie selbst
  bestimmt.
/ L.A.: der Hebungswinkel in Grad, von 10 bis 90.
/ Audio-Schaltfläche: zeigt den verwendeten Audioeingang an. Ein Klick öffnet
  die Audioeinstellungen.
/ cal: der Kalibrierwert in s/t, von −100 bis +100 s/t. Er wird von der
  Kalibrierung eingetragen und kann auch von Hand eingegeben werden.
/ Lage: die Lage der Uhr (Zifferblatt oben, Zifferblatt unten, Krone oben,
  Krone unten, Krone links, Krone rechts oder nicht gesetzt).
/ Messung festhalten: behält die aktuelle Messung in einem neuen Reiter. Das
  Tastenkürzel ist Strg+Leertaste.
/ Aktuelle Aufnahme: der Name des gewählten Aufnahme-Reiters, der bearbeitet
  werden kann.
/ Menü: #ui[Öffnen], #ui[Aktuelle Anzeige speichern],
  #ui[Alle Aufnahmen speichern], #ui[Leichter Algorithmus],
  #ui[Audioeinstellungen…], #ui[Kalibrierung starten], #ui[Benutzerhandbuch],
  #ui[Alle Aufnahmen schließen] und #ui[Beenden].
== Die Ergebnistabelle
Die Tabelle oben in jedem Reiter zeigt:
/ RATE: die Gangabweichung in s/d, korrigiert um den Wert cal.
/ AMP.: die Amplitude in Grad, sofern sie sich messen lässt.
/ B.E.: der Abfallfehler in ms.
/ L.A.: der verwendete Hebungswinkel.
/ BEAT: die verwendete Schlagzahl in bph. Ein mit \~ markierter Wert ist eine
  freie Schätzung, die angezeigt wird, wenn das Signal nicht zur gewählten bph
  passt.
Links neben der Tabelle zeigen ein Uhrsymbol und vier Quadrate den Zustand des
Signals. Jedes Quadrat ist ein Analysefenster mit gültiger Periode. Das
Uhrsymbol zeigt:
- *0*: kein Audioeingang;
- *?*: Audio ist vorhanden, aber es wurde noch keine Schlagzahl erkannt;
- *!*: kein gültiger Rhythmus bei der gewählten Schlagzahl;
- das einfache Zifferblatt: Audio und eine gültige Schlagperiode wurden
  erkannt.
Wenn Sie den Mauszeiger über das Symbol halten, erscheint diese Erklärung im
Programm.
== Das Gangdiagramm
Das Gangdiagramm zeichnet jeden Schlag als Punkt. Die Zeit läuft nach unten.
Die horizontale Position eines Punkts ist der Zeitpunkt des Schlags innerhalb
des Fensters, eine Uhr mit richtigem Gang zeichnet also senkrechte Linien. Eine
vorgehende Uhr zeichnet Linien, die sich in die eine Richtung neigen, eine
nachgehende Uhr Linien, die sich in die andere neigen. Tick und Tack werden in
verschiedenen Farben gezeichnet. Bei einem Abfallfehler bilden sie zwei
getrennte Linien.
Das Gangdiagramm hat eine feste Breite. Am oberen Rand verlaufen zwei Skalen,
die sich dieselben senkrechten Rasterlinien teilen: die Amplitude in Grad und
die Zeit innerhalb des Fensters in ms. Die Zeitangaben links sind Minuten und
Sekunden.
Die weiteren Linien, wie in der Legende unten aufgeführt:
- *Amplitude*: die rosa Linie, abzulesen auf der Gradskala oben;
- *Gang-Hilfslinie*: die gestrichelten blauen Linien neigen sich mit der
  gemessenen Gangabweichung, geglättet über etwa 10 Sekunden. Verlaufen die
  Punkte parallel dazu, ist die Gangabweichung stabil.
#shot("paper-strip.png", [Das Gangdiagramm mit seinen Bedienelementen. Tick und
Tack bilden zwei nahe beieinander liegende Linien, und die Amplitude sinkt von
etwa 300° auf 250°.],
width: 45%)
Die Schaltflächen unter dem Diagramm:
/ < und >: verschieben das Diagramm um eine Rasterteilung.
/ Löschen: löscht das Diagramm und beginnt von vorn (nur im Live-Reiter).
/ Zentrieren: setzt den neuesten Schlag in die Mitte des Diagramms.
/ −, 1x, +: Zoom. Die Zoomstufen sind 0.25×, 0.5×, 1×, 2×, 4× und 8×.
  Bei 1× ist das Diagramm einen Schlag breit. 1x kehrt zu dieser Stufe zurück.
Die Einstellungen unter den Schaltflächen:
/ Y-Marke (s): der Abstand zwischen den Marken auf der senkrechten Achse, in
  Sekunden. + und − ändern ihn um 1 s. Kleinere Werte, etwa 0.5, lassen sich
  eintippen.
/ Y-Fenster (s): der senkrecht angezeigte Zeitraum in Sekunden. Er ist fest
  und ändert sich nicht von selbst.
== Die Signalform-Anzeigen
Die Seite #ui[Signalformen] rechts zeigt den gemittelten Klang der Schläge:
/ Tick: der gemittelte Klang des ersten Schlags.
/ Tack: der gemittelte Klang des zweiten Schlags.
/ Volle Periode: Tick und Tack zusammen, die beiden hervorgehoben.
/ Debug: das aufbereitete Signal, das die Analyse verwendet.
#shot("waveform-panels.png", [Die Signalform-Anzeigen. Jeder Schlag zeigt die
getrennten Klicks der Hemmung.])
Die Anzeigen für Tick und Tack haben unten eine ms-Skala und oben eine
Amplitudenskala in Grad. Die blaue senkrechte Linie markiert die aus der
Signalform gemessene Amplitude.
Eine saubere Signalform mit scharfen, getrennten Klicks bedeutet ein gutes
Signal. Eine flache oder unscharfe Signalform bedeutet, dass das Mikrofon die
Uhr nicht gut aufnimmt.
Die Seite #ui[Verlauf] wird in @trend beschrieben.
