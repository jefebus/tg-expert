#import "../lib.typ": *

= Installation
== Windows
Das Windows-Installationsprogramm `.msi` aus der neuesten Version auf der
#link("https://github.com/jefebus/tg-expert/releases")[Releases-Seite]
herunterladen und ausführen. Das Installationsprogramm legt tg-expert in
einem eigenen Ordner unter `Program Files` ab, fügt Startmenü-Einträge für das
Programm und für dieses Handbuch hinzu und verknüpft `.tgj`-Dateien mit dem
Programm.
Auf jedem System öffnet #ui[Benutzerhandbuch] im Menü dieses Handbuch. Ist das
Handbuch nicht installiert, öffnet sich die Releases-Seite, von der sich das
PDF herunterladen lässt.
tg-expert wird neben tg-timer installiert und ersetzt es nicht. Beide
Programme verwenden getrennte Einstellungen.
Zum Deinstallieren dient *Einstellungen > Apps* in Windows.
#note[Windows fragt möglicherweise, ob das Programm das Mikrofon verwenden
darf. Wurde der Zugriff verweigert, siehe @troubleshooting.]
== Linux
Für Debian- und Ubuntu-Systeme mit x86-64 (amd64) steht in der neuesten Version auf
der #link("https://github.com/jefebus/tg-expert/releases")[Releases-Seite]
ein `.deb`-Paket zur Verfügung. Dieses herunterladen und mit der
Paketverwaltung installieren.

Alternativ lässt sich das Programm aus dem Quellcode bauen, wie in
`BUILDING.md` im Quellarchiv beschrieben; anschließend ist `make install`
auszuführen. Dies installiert das Programm, die Man-Page, einen Desktop-Eintrag
und den Dateityp `.tgj`, außerdem dieses Handbuch, sofern Typst beim Bauen
verfügbar war.
== macOS
Es gibt kein macOS-Paket. Das Bauen aus dem Quellcode mit Homebrew sollte
nach `BUILDING.md` funktionieren, wurde für diese Version aber nicht
überprüft. Beim ersten Start fragt macOS nach der Mikrofonerlaubnis für das
Programm oder für das Terminal, aus dem es gestartet wurde. Die Erlaubnis ist
unter *Systemeinstellungen > Datenschutz & Sicherheit > Mikrofon* zu erteilen.
== Einstellungsdatei
tg-expert speichert seine Einstellungen in `tg-expert.ini` im
Konfigurationsordner des Benutzers (`%LOCALAPPDATA%` unter Windows,
`~/.config` unter Linux). Darin liegen die Fenstergröße, die
Audioeinstellungen, der Wert cal, die gespeicherten Uhren und die
Einstellungen des Gangdiagramms. Wird die Datei gelöscht, kehrt das Programm
zu seinen Standardwerten zurück.
== Sprache
Das Programm zeigt seine Texte in der Sprache des Betriebssystems an:
Englisch, Spanisch oder Deutsch. Für jede andere Sprache verwendet es
Englisch. #ui[Benutzerhandbuch] im Menü öffnet das Handbuch in derselben
Sprache.
Einiges bleibt in jeder Sprache gleich, damit sich die Messwerte mit jeder
Zeitwaage vergleichen lassen:
- Zahlen verwenden immer den Punkt als Dezimaltrennzeichen, wie in +2.5 s/t;
- die Spaltenköpfe der Ergebnistabelle RATE, AMP., B.E., L.A. und BEAT sowie
  deren Einheiten;
- die Lagenkürzel DU, DD, CU, CD, CL und CR;
- der Kalibrierstatus: cal (wait), cal (acq.), cal (done), cal (fail).
Unter Linux lässt sich die Sprache beim Start des Programms wählen, zum
Beispiel mit `LANGUAGE=de tg-expert`.
