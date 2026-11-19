#import "../lib.typ": *

#set heading(numbering: "A.1")
#counter(heading).update(0)
= Grenzwerte
#table(
  columns: 2,
  [*Größe*], [*Bereich*],
  [Schlagzahl], [8100 bis 72000 bph],
  [Hebungswinkel], [10° bis 90°, Standardwert 52°],
  [cal], [−100.0 bis +100.0 s/t],
  [Abtastraten], [22.05, 32, 44.1 und 48 kHz, sofern das Gerät sie unterstützt],
  [Aufzeichnung des Verlaufs], [ein Messwert alle 2 s, bis zu 100 Stunden],
  [Kalibrierung], [etwa 15 Minuten (900 Ticks einer 1-Hz-Quarzreferenz)],
)
= Glossar
/ Abfallfehler: die positive zeitliche Asymmetrie zwischen den wechselnden Halbperioden Tick-zu-Tack und Tack-zu-Tick, in ms.
/ Amplitude: der Ausschlag der Unruh aus ihrer Ruhelage, in Grad.
/ Aufnahme: eine eingefrorene Kopie einer Messung, in einem eigenen Reiter
  aufbewahrt.
/ bph: Schläge pro Stunde.
/ cal: die Korrektur für den Takt der Soundkarte, in s/t.
/ dBFS: Dezibel relativ zum Vollaussteuerungspegel. 0 dBFS ist das lauteste
  Signal, das der Eingang aufnehmen kann.
/ Gangabweichung: das Vor- oder Nachgehen der Uhr, in s/t.
/ Gangdiagramm: die Darstellung der Schlagzeitpunkte, benannt nach den
  Papierstreifen älterer Zeitwaagen.
/ Hebungswinkel: der Winkel, über den die Unruh mit der Hemmung in Kontakt
  steht, in Grad.
/ Lagenfehler: die größte Differenz der mittleren Gangabweichung zwischen
  Lagen.
/ Schlag: ein Tick oder ein Tack. Die Unruh macht zwei Schläge pro
  Schwingung.
/ .tgj: das Dateiformat von Tg, tg-timer und tg-expert.
= Danksagung und Lizenz
tg-expert basiert auf *Tg*, geschrieben von Marcello Mamino und den
Mitwirkenden von #link("https://github.com/vacaboja/tg")[vacaboja/tg], sowie
auf *tg-timer*, Alejandro Grigeras Weiterführung von Tg. tg-expert wird von
jefebus gepflegt unter
#link("https://github.com/jefebus/tg-expert")[github.com/jefebus/tg-expert].
tg-expert ist freie Software und wird unter der GNU General Public License,
Version 2, verbreitet. Es wird ohne jede Gewährleistung geliefert. Die
vollständige Lizenz befindet sich in der Datei `LICENSE`, die mit dem Programm
ausgeliefert wird.
Dieses Handbuch wird unter derselben Lizenz verbreitet.
