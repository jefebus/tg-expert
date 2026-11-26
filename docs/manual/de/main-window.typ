#import "../lib.typ": *

= Das Hauptfenster <main-window>
#shot("main-window.png", [Das Hauptfenster bei der Messung einer Uhr mit 28800 bph.])
Das Fenster besteht aus einer oberen Leiste, einer Reiterzeile, der
Ergebnistabelle, links dem Gangdiagramm und rechts den Signalform-Anzeigen
oder den Seiten Verlauf und Korrelationen.
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
/ Aufzeichnen / Pause: beginnt und beendet einen Abschnitt, eine Zeit
  ununterbrochener Aufzeichnung (siehe @runs).
/ Messung festhalten: behält die aktuelle Messung in einem neuen Reiter, ohne
  die Aufzeichnung zu unterbrechen. Nur während der Aufzeichnung. Das
  Tastenkürzel ist Strg+Leertaste.
/ Aktuelle Aufnahme: der Name des gewählten Aufnahme-Reiters, der bearbeitet
  werden kann.
Die Menüleiste am oberen Fensterrand hat vier Menüs:

/ Datei: #ui[Neue Sitzung], #ui[Sitzung öffnen…], #ui[Speichern],
  #ui[Speichern unter…], #ui[Daten exportieren…], #ui[Bericht exportieren…]
  (siehe @files) und #ui[Beenden].
/ Einstellungen: #ui[Audioeinstellungen…] und #ui[Algorithmuskonfiguration…].
/ Werkzeuge: #ui[Kalibrierung starten] (#ui[Kalibrierung beenden] während der
  Kalibrierung) und #ui[Projekt- und Notizbereich], der diesen Bereich ein- oder
  ausblendet.
/ Hilfe: #ui[Benutzerhandbuch] und #ui[Über tg-expert], mit der Version, der
  Lizenz und einem Link zum Quellcode auf GitHub.

Tastenkürzel, neben den Menüeinträgen angezeigt: Strg+N neue Sitzung, Strg+O
Sitzung öffnen, Strg+S speichern, Strg+Umschalt+S speichern unter, Strg+E
Daten exportieren, Strg+P Bericht exportieren, Strg+Q beenden, F1 Benutzerhandbuch und Strg+Leertaste für
eine Aufnahme. Ein Kürzel wirkt nur, solange sein Eintrag verfügbar ist (zum
Beispiel nicht während der Kalibrierung).

== Projekt- und Notizbereich <notes-panel>
Er schwebt über dem rechten Teil der Reiter und verdeckt sie nur teilweise,
damit der größte Teil der Diagramme beim Schreiben sichtbar bleibt. Anfangs
ist er ausgeblendet: #ui[Projekt- und Notizbereich] im Menü #ui[Werkzeuge]
blendet ihn ein oder aus, seine Schließen-Schaltfläche blendet ihn aus. Der
Streifen an seinem linken Rand lässt sich ziehen, um seine Breite zu ändern.
Das Programm merkt sich, ob er angezeigt war, und seine Breite. Er hat zwei
Seiten:

/ Projekt: #ui[Projektname], #ui[Auftragsreferenz], #ui[Eigentümer] und
  #ui[Letzte Revision] (ein Jahr, ein Monat oder ein Tag: `JJJJ`, `JJJJ-MM`
  oder `JJJJ-MM-TT`, so gespeichert wie eingegeben; ein ungültiges Datum wird
  rot angezeigt und erst gespeichert, wenn es gültig ist) in der
  einklappbaren Gruppe #ui[Angaben]; die Uhr der Sitzung, also die in der
  Liste der gespeicherten Uhren gewählte, die hier nicht bearbeitet wird; und
  die #ui[Allgemeinen Notizen].
/ Messung: die Notizen des Abschnitts oder der Aufnahme im gewählten Reiter,
  mit ihrem Titel (#ui[Notizen zu: Abschnitt 2], #ui[Notizen zu: 2.3 (DU)]).
  Jeder Abschnitt und jede Aufnahme hat eigene, zunächst leere Notizen. Im
  Echtzeit-Reiter sind es während der Aufzeichnung die Notizen des laufenden
  Abschnitts (#ui[Notizen zu: Abschnitt 3 (läuft)]); sie gehen bei #ui[Pause]
  mit dem Abschnitt in seinen Reiter. In der Pause wird kein Abschnitt
  aufgezeichnet, und die Seite zeigt das an.

#shot("notes_project.png", [Die Seite Projekt mit den Angaben zur Identifikation
und den allgemeinen Notizen.], width: 60%)

#shot("notes_measurement.png", [Die Seite Messung mit den Notizen zu einer
Aufnahme.], width: 65%)

Eingaben ändern die Sitzung sofort (es gibt keine Schaltfläche Übernehmen) und
werden mit ihr gespeichert. Notizen sind reiner Text, bis zu 64 KiB je Notiz.

Der Erkennungsablauf und die #ui[Algorithmuskonfiguration…] werden in
@algorithm beschrieben; die vollständige Parameterreferenz steht in
@algorithm-configuration.

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
Punkte, die einen senkrechten Rand überschreiten, erscheinen am anderen Rand
wieder. Der Wiederholungsabstand folgt der aktuellen ms-Skala und dem Zoom.
Nahe einem Rand kann ein Punkt auf beiden Seiten erscheinen; Zentrieren setzt
den neuesten Schlag weiterhin in die Mitte.
Die weiteren Linien, wie in der Legende unten aufgeführt:
- *Amplitude*: die rosa Linie, abzulesen auf der Gradskala oben;
- *Gang-Hilfslinie*: die gestrichelten blauen Linien neigen sich mit der
  gemessenen Gangabweichung, geglättet über etwa 10 Sekunden. Verlaufen die
  Punkte parallel dazu, ist die Gangabweichung stabil.
#shot("paper-strip.png", [Das Gangdiagramm mit seinen Bedienelementen. Tick und
Tack bilden zwei nahe beieinander liegende Linien, daneben der
Amplitudenverlauf.],
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
Signalform gemessene Amplitude. Die Ansicht erweitert sich bei Bedarf nach
links, um die konfigurierte Mindestamplitude einzubeziehen; die
hervorgehobenen Fenster der vollen Periode werden
entsprechend erweitert. Dies ändert nur die Darstellung, nicht die
Amplitudenerkennung.
Eine saubere Signalform mit scharfen, getrennten Klicks bedeutet ein gutes
Signal. Eine flache oder unscharfe Signalform bedeutet, dass das Mikrofon die
Uhr nicht gut aufnimmt.
Die Statuszeile unter den Signalformen bleibt auch leer sichtbar. Den
vollständigen Text einer gekürzten Meldung zeigt ein Mauszeiger darüber.
Wird der Schlag erkannt, aber die Amplitude kann nicht gemessen werden,
meldet der Status unter den Signalformen "Amplitude nicht verfügbar". Gang
und Abfallfehler bleiben verfügbar; der Abfallfehler wird nicht anhand der
Hebungsimpulse verfeinert. Die Debug-Version zeigt zusätzlich den Grund für
die Ablehnung der Amplitude.
Wird eine stabile Periode gefunden, aber Tick und Tack lassen sich nicht
trennen, wechselt die Anzeige automatisch zu "Nur Gang": Abfallfehler und
Amplitude sind nicht verfügbar. Bis Tick und Tack wieder erkannt werden,
werden keine neuen Signalformen, Papierstreifenereignisse oder Verlaufswerte
erzeugt. Der bisherige Verlauf bleibt sichtbar.
Die Seiten #ui[Verlauf] und #ui[Korrelationen] werden in @trend beschrieben.
