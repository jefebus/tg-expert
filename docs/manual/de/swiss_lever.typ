#import "../lib.typ": *

= Das Schlaggeräusch der Schweizer Ankerhemmung
Dieses Kapitel erklärt, was die von tg-expert dargestellte Signalform zeigt, wie sie mit der Schweizer Ankerhemmung zusammenhängt und welche akustischen Zeitmarken das Programm für Gangabweichung, Abfallfehler und Amplitude verwendet.

Ein Tick oder Tack ist kein einzelner Stoß. Jeder Schlag enthält mehrere, zeitlich dicht aufeinanderfolgende mechanische Vorgänge. Im Schlaggeräusch lassen sich meist drei Hauptimpulse erkennen. tg-expert verarbeitet das gesamte akustische Paket und gewinnt daraus wiederholbare Zeitmarken.

== Funktion der Hemmung und Entstehung von Tick und Tack
Die Schweizer Ankerhemmung verbindet das Räderwerk mit dem Regulierorgan aus Unruh und Spiralfeder. Bei jedem Schlag bewegt der Hebelstein den Anker, das Ankerrad wird ausgelöst, ein Impuls wird übertragen und das Ankerrad wieder gesperrt. Beim folgenden Schlag läuft derselbe Vorgang an der gegenüberliegenden Palette ab.

=== Die fünf mechanischen Phasen
#table(
  columns: (25mm, 50mm, 50mm, 50mm),
  table.header([*Phase*], [*Vorgang*], [*Wahrscheinliche akustische Erscheinung*], [*Diagramm*]),
  [*1. Auslösung*], [Der Hebelstein berührt die Ankergabel. Der Anker beginnt sich zu drehen und der Ankerradzahn verlässt die Sperrfläche der Palette.], [Erstes Geräusch, zeitlich meist präzise, aber schwächer als der letzte Impuls.], [#shot("fase_1.png", [], width: 50%)],
  [*2. Hebung / Impuls*], [Der Zahn läuft über die Hebefläche. Energie wird vom Ankerrad über Palette und Anker auf Hebelstein und Unruh übertragen.], [Verteilter, unregelmäßiger Impuls, der sich mit der Auslösung überlagern kann.], [#shot("fase_2.png", [], width: 50%)],
  [*3. Fall*], [Nach der Hebung dreht sich das Ankerrad um einen kleinen Winkel frei, bis der nächste Zahn die gegenüberliegende Palette erreicht.], [Während der freien Bewegung entsteht wenig Geräusch; das Fallgeräusch entsteht hauptsächlich an ihrem Ende.], [#shot("fase_3.png", [], width: 50%)],
  [*4. Sperrung*], [Der nächste Zahn trifft auf die Sperrfläche der gegenüberliegenden Palette und stoppt das Ankerrad.], [Starker, abrupter Stoß.], [#shot("fase_4.png", [], width: 50%)],
  [*5. Lauf bis zum Anschlag*], [Nach dem ersten Sperrkontakt setzt sich der Zahn in der Sperrung und der Anker läuft noch um einen kleinen Winkel bis zum Anschlag weiter.], [Liegt sehr nahe an der Sperrung; beide bilden meist ein gemeinsames starkes Geräuschpaket.], [#shot("fase_5.png", [], width: 50%)],
)

Die Reihenfolge lautet *Auslösung -> Hebung/Impuls -> Fall -> Sperrkontakt -> Lauf bis zum Anschlag*. Die Sperrung beginnt vor dem Anschlag des Ankers, akustisch überlagern sich beide Vorgänge jedoch weitgehend.

=== Die drei akustischen Hauptimpulse
+ *Auslösung.* Der Hebelstein tritt in die Ankergabel ein und setzt den Anker in Bewegung. Dieses erste Geräusch liefert eine präzise Zeitmarke, kann aber relativ schwach sein.
+ *Hebung / Impuls.* Der Ankerradzahn läuft über die Hebefläche und überträgt Energie auf die Unruh. Das Geräusch ist unregelmäßiger und erscheint nicht immer als einzelner Peak.
+ *Sperrung und Anschlag.* Der Fall endet, wenn der nächste Zahn die Sperrfläche erreicht; unmittelbar danach beendet der Anker seinen Weg am Anschlag. Dieses dritte Geräuschpaket ist meist das stärkste.

#shot("waveform_3_pulses.png", [Prinzipdarstellung der Hüllkurve eines Tick oder Tack.], width: 50%)
Die reale Signalform enthält bipolare Schwingungen, Rückpralleffekte und Resonanzausläufer. Die Peakhöhe hängt daher nicht nur vom mechanischen Kontakt ab, sondern auch von Gehäuse, Kontaktstelle, Uhrenlage, Anpressdruck des Mikrofons und Übertragungsweg.
#note[Ordnen Sie nicht jeden Peak einem bestimmten Bauteil zu. Die Signalform ist die gemeinsame Antwort von Hemmung, Werk, Gehäuse, Halter und Mikrofon.]

=== Unterschiede zwischen Tick und Tack
Tick und Tack entsprechen den wechselnden Vorgängen an Eingangs- und Ausgangspalette. Ihre Zeiten sollten um die Ruhelage der Unruh symmetrisch sein, ihre akustische Form und ihr Pegel können sich jedoch unterscheiden. Ein Höhenunterschied allein ist kein Fehler; wichtiger sind zeitliche Stabilität und Wiederholbarkeit.

=== Aus dem Geräusch gewonnene Messgrößen
#shot("parameters_from_sound.png", [Wie die Messgrößen aus der Signalform gewonnen werden.], width: 100%)

== Wie tg-expert das Signal verarbeitet
Der Algorithmus stammt aus Tg. Er erkennt keine einzelnen Bauteile und klassifiziert keine mechanischen Phasen. Er wandelt das Audio in eine Hüllkurve um, sucht die periodische Wiederholung und misst daraus Zeitmarken.

=== Verarbeitungskette
#table(columns: (12mm, 45mm, 99mm),
  table.header([*Nr.*], [*Operation*], [*Zweck*]),
  [1], [Hochpass], [Reduziert Drift, langsame Schwingungen und Handhabungsgeräusche.],
  [2], [Rauschunterdrückung], [Verwirft länger anhaltende Abschnitte mit ungewöhnlich hoher Energie.],
  [3], [Gleichrichtung], [`fabs()` wandelt das bipolare Signal in einen positiven Betrag um.],
  [4], [Tiefpass], [Extrahiert die zeitliche Hüllkurve des Schlaggeräuschs.],
  [5], [FFT-Autokorrelation], [Findet die Periodizität ohne fest vorgegebene Tick-Signalform.],
  [6], [Periodenfaltung], [Richtet viele Tick und Tack aus und erzeugt eine repräsentative Signalform.],
  [7], [Getrimmter Mittelwert], [Verwirft die obersten 20 %, um einzelne Stöße zu unterdrücken.],
  [8], [Tick-Tack-Trennung], [Sucht den Gegenschlag nahe der halben Periode.],
  [9], [Hebungszeit], [Findet den erkennbaren Beginn des Geräuschpakets vor der letzten Referenz.],
  [10], [Vorlagenkorrelation], [Lokalisiert einzelne Schläge für das Gangdiagramm.],
)

=== Signalaufbereitung und Hüllkurve
Der Hochpass erhält schnelle Hemmungsstöße und reduziert langsame Bewegungen. Die Gleichrichtung entfernt das Vorzeichen der Schwingung, der Tiefpass erzeugt die Hüllkurve. Die weitere Erkennung arbeitet hauptsächlich mit dieser Hüllkurve.

=== Gangabweichung und Periode
Die Autokorrelation bestimmt, nach welcher Zeit sich ein gleichartiger Schlag wiederholt. Bei 28800 bph entstehen acht Schläge pro Sekunde: Tick und der folgende Tack liegen etwa 0.125 s auseinander, zwei aufeinanderfolgende Tick etwa 0.25 s. Diese Tick-zu-Tick-Periode ist die Hauptreferenz für die Gangabweichung.

#note[Eine gute Gangmessung bestätigt nicht automatisch die Amplitudenmessung. Die Periodizität kann stabil sein, obwohl die schwache Auslösung nicht zuverlässig erkannt wird.]

=== Abfallfehler
Der Abfallfehler beschreibt die zeitliche Asymmetrie der beiden wechselnden Halbperioden: Tick zum folgenden Tack und Tack zum folgenden Tick. tg-expert vergleicht beide Intervalle und zeigt B.E. als positiven Wert in Millisekunden an.

=== Amplitudenberechnung
Die Amplitude wird nicht aus der Peakhöhe bestimmt. tg-expert ermittelt die Hebungszeit aus der Signalform und kombiniert sie mit dem eingestellten Hebungswinkel. Das Modell nimmt eine annähernd sinusförmige Unruhbewegung an: Beim gleichen Hebungswinkel entspricht eine kürzere Hebungszeit einer größeren Amplitude.

Der Algorithmus glättet die Signalform mit einem Fenster von ungefähr 1 ms und bestimmt eine Schwelle aus Maximalpegel und Hintergrund. Für Tick und Tack sucht er rückwärts von der Hauptreferenz nach dem erkennbaren Beginn des Geräuschpakets, normalerweise nahe der Auslösung. Dieser Beginn wird mit der starken letzten Referenz aus Sperrung/Anschlag zur akustischen Hebungszeit kombiniert.

#note[Liegt die Auslösung unterhalb der Schwelle, beginnt tg-expert zu spät. Die gemessene Hebungszeit wird zu kurz und die angezeigte Amplitude kann zu hoch ausfallen.]

Der Hebungswinkel ist ebenfalls kritisch. Ein falscher Hebungswinkel liefert eine falsche Amplitude, beeinflusst RATE und B.E. jedoch nicht.

=== Was der Algorithmus unterscheidet
#table(columns: (70mm, 86mm),
  table.header([*Element*], [*Behandlung in tg-expert*]),
  [Tick gegenüber Tack], [Ja. Beide Richtungen werden getrennt lokalisiert.],
  [Drei akustische Impulse], [Sie werden nicht einzeln klassifiziert, sondern bilden eine gemeinsame Hüllkurve.],
  [Peakhöhe], [Sie beeinflusst die Erkennung, bestimmt aber nicht die Amplitude.],
  [Hebungszeit], [Ja. Sie wird aus den erkennbaren akustischen Referenzen geschätzt.],
  [Amplitude], [Sie wird aus Hebungszeit, Periode und eingestelltem Hebungswinkel abgeleitet.],
)

== Qualität der Messung beurteilen
#table(columns: (35mm, 61mm, 60mm),
  table.header([*Merkmal*], [*Gute Messung*], [*Fragliche Messung*]),
  [Hintergrund], [Niedrig und zwischen den Schlägen stabil.], [Dauerrauschen, Stöße oder wechselnder Pegel.],
  [Auslösung], [Beginn sichtbar, sauber und wiederholbar.], [Beginn verdeckt, unstetig oder wechselnd.],
  [Letzter Impuls], [Stark, aber nicht übersteuert.], [Flache Spitze, Übersteuerung oder langer Resonanzausläufer.],
  [Tick und Tack], [Beide werden stabil erkannt.], [Einer verschwindet oder ändert sich ständig.],
  [Gangdiagramm], [Definierte Linien ohne Doppelereignisse.], [Punktwolken, Sprünge oder Doppelauslösung.],
  [Gangabweichung], [Über mehrere Fenster stabil.], [Sprunghaft ohne physische Änderung der Uhr.],
  [Amplitude], [Plausibel und konsistent.], [Große Sprünge bei stabiler Gangabweichung.],
)

== Grenzen der Interpretation
Die Signalform eignet sich zur Beurteilung von Periodizität, Stabilität und Erkennungsqualität. Allgemein lässt sich der Anfang der Auslösung, der mittlere Bereich der Hebung und das letzte Paket der Sperrung/des Anschlags zuordnen. Eine einzelne Resonanz erlaubt jedoch keine sichere Diagnose eines bestimmten Bauteils und ersetzt bei dauerhaften Auffälligkeiten nicht die direkte Prüfung der Hemmung.

== Quellen
1. Witschi Electronic AG. _Prüfmethoden mechanischer Uhren_. Büren an der Aare, 2025. Abschnitte „Akustische Messung“, „Das Schlaggeräusch der Schweizer Ankerhemmung“, „Gangabweichung“, „Abfallfehler“ und „Amplitude“.
2. Israel, Robin. „The Swiss lever escapement - Part 1: For everyone“. _R.I.O. Uhrenwerkstätte_, 28. Oktober 2026.
3. Velásquez Enriquez, Guido Alfonso. „The Beat Noise Of The Swiss Lever Escapement“. Video, 21. Oktober 2013.
