#import "../lib.typ": *

= Dateien <files>
Messungen werden in Dateien mit der Endung `.tgj` gespeichert.

== Speichern
- #ui[Aktuelle Anzeige speichern] speichert den gewählten Reiter: entweder die Live-Messung oder eine Aufnahme.
- #ui[Alle Aufnahmen speichern] speichert alle Aufnahme-Reiter in einer einzigen Datei.

Der Speicherdialog schlägt einen Dateinamen aus dem Messkontext vor. Ist eine gespeicherte Uhr gewählt, wird ihr Name als Grundlage verwendet. Ist eine Lage gesetzt, wird das Lagenkürzel ergänzt. Werden mehrere Aufnahmen gemeinsam gespeichert, sollte der Sitzungs- oder Uhrenname statt des Namens nur einer Lage verwendet werden. Der vorgeschlagene Dateiname kann vor dem Speichern geändert werden. Die Endung `.tgj` wird bei Bedarf ergänzt.

#note[Verwenden Sie kurze, dateisystemtaugliche Uhren- und Aufnahmenamen, wenn Dateien zwischen Windows, Linux und macOS ausgetauscht werden. Das Umbenennen einer Aufnahme ändert den in der `.tgj`-Datei gespeicherten Aufnahmenamen; der Dateiname lässt sich im Speicherdialog unabhängig davon wählen.]

== Öffnen
- #ui[Öffnen] im Menü;
- Doppelklick auf eine `.tgj`-Datei auf Systemen, in denen der Dateityp registriert ist (das Windows-Installationsprogramm und `make install` tun dies);
- eine oder mehrere Dateien auf der Kommandozeile angeben: `tg-expert file.tgj`.

Jede Aufnahme in der Datei öffnet sich in einem eigenen Reiter. Aufnahmen lassen sich ansehen, umbenennen, mit einer Lage versehen und erneut speichern, aber nicht als Live-Messung fortsetzen.

== Inhalt einer Datei
Für jede Aufnahme:
- den Aufnahmenamen und die Lage der Uhr;
- die Ergebnisse: Gangabweichung, Amplitude, Abfallfehler, bph, Hebungswinkel und cal;
- die im Gangdiagramm angezeigten Schläge einschließlich Tick/Tack-Zuordnung und Amplitudenverlauf;
- die gemittelten Signalformen;
- Zoom und Verschiebung des Gangdiagramms;
- die Langzeitstatistik und die Diagramme der Seite Verlauf.

Die Datei enthält nicht die Audioaufnahme.

== Kompatibilität
Mit Tg und tg-timer gespeicherte Dateien lassen sich in tg-expert öffnen. Aufnahmen ohne Langzeitstatistik zeigen auf der Seite Verlauf eine Meldung statt der Statistik. Wird eine tg-expert-Datei in einem älteren Programm geöffnet und erneut gespeichert, können Verlaufsdaten verloren gehen.
