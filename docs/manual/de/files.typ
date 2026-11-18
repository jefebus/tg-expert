#import "../lib.typ": *
= Dateien <files>
Messungen werden in Dateien mit der Endung `.tgj` gespeichert.
== Speichern
- #ui[Aktuelle Anzeige speichern] speichert den gewählten Reiter: die Live-Messung
  oder eine Aufnahme.
- #ui[Alle Aufnahmen speichern] speichert jeden Aufnahme-Reiter in einer
  Datei.
== Öffnen
- #ui[Öffnen] im Menü;
- Doppelklick auf eine `.tgj`-Datei, auf Systemen, in denen der Dateityp
  registriert ist (das Windows-Installationsprogramm und `make install` tun
  dies);
- eine oder mehrere Dateien auf der Kommandozeile angeben:
  `tg-expert file.tgj`.
Jede Aufnahme in der Datei öffnet sich in einem eigenen Reiter. Aufnahmen
lassen sich ansehen, umbenennen, mit einer Lage versehen und erneut
speichern, aber nicht fortsetzen.
== Was eine Datei enthält
Für jede Aufnahme:
- den Namen und die Lage der Uhr;
- die Ergebnisse: Gangabweichung, Amplitude, Abfallfehler, bph,
  Hebungswinkel und cal;
- die im Gangdiagramm angezeigten Schläge mit Tick und Tack sowie den Verlauf
  der Amplitude;
- die gemittelten Signalformen;
- Zoom und Position des Gangdiagramms;
- die Langzeitstatistik und die Diagramme der Seite #ui[Verlauf].
Die Datei enthält nicht die Audioaufnahme.
== Kompatibilität
Mit Tg und tg-timer gespeicherte Dateien lassen sich in tg-expert öffnen.
Aufnahmen, die ohne Langzeitstatistik gespeichert wurden, zeigen auf der Seite
#ui[Verlauf] eine Meldung statt der Statistik. Mit tg-expert gespeicherte
Dateien können beim Öffnen in den älteren Programmen die Daten des Verlaufs
verlieren.
