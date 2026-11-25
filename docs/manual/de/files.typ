#import "../lib.typ": *

= Dateien <files>
Messungen werden in Dateien mit der Endung `.tgj` gespeichert.

== Sitzungen
Eine Sitzung ist eine `.tgj`-Datei mit allem, was an einer Uhr gemessen wurde: die Projektangaben, die Uhr, alle Aufnahmen und der Stand des Echtzeit-Reiters beim Speichern. Die Titelleiste zeigt den Projektnamen und die Datei; `*` bedeutet, dass es ungespeicherte Änderungen gibt.

- #ui[Neue Sitzung] beginnt eine leere Sitzung für die in der oberen Leiste gewählte Uhr.
- #ui[Sitzung öffnen…] ersetzt die aktuelle Sitzung durch die einer Datei.
- #ui[Speichern] schreibt die ganze Sitzung in ihre Datei, gleich welcher Reiter angezeigt wird. Beim ersten Mal wirkt es wie #ui[Speichern unter…].
- #ui[Speichern unter…] schreibt die Sitzung in eine andere Datei, die dann zur Sitzungsdatei wird.

Bevor eine Sitzung mit ungespeicherten Änderungen ersetzt oder das Programm beendet wird, fragt das Programm, ob sie gespeichert werden sollen. Änderungen sind neue, umbenannte, geschlossene oder mit einer anderen Lage versehene Aufnahmen und die Projektangaben; hat die Sitzung bereits Aufnahmen oder eine Datei, auch neue Echtzeit-Messwerte. Das bloße Beobachten des Echtzeit-Reiters ist keine Änderung.

== Speichern
#ui[Speichern] schreibt die Aufnahmen und, als #ui[Stand beim Speichern], was der Echtzeit-Reiter in diesem Moment zeigt, mit seiner Langzeitstatistik. Erneutes Speichern im selben Programmlauf ersetzt diesen Stand durch den aktuellen. Während der Kalibrierung wird der Echtzeit-Reiter nicht gespeichert.

Der Speicherdialog schlägt `JJJJ-MM-TT Projektname.tgj` (mit dem heutigen Datum) vor, oder den Uhrennamen, wenn es keinen Projektnamen gibt. Zeichen, die in Dateinamen nicht erlaubt sind, werden durch `_` ersetzt; der in der Sitzung gespeicherte Projektname bleibt unverändert. Die Endung `.tgj` wird bei Bedarf ergänzt.

Die Datei wird zuerst in eine temporäre Datei im selben Ordner geschrieben und ersetzt die vorherige erst, wenn sie vollständig geschrieben ist. Schlägt das Speichern fehl, bleibt die vorherige Datei unverändert und die Sitzung behält ihre ungespeicherten Änderungen.

#note[Verwenden Sie kurze, dateisystemtaugliche Namen, wenn Dateien zwischen Windows, Linux und macOS ausgetauscht werden.]

== Öffnen
- #ui[Sitzung öffnen…] im Menü #ui[Datei];
- Doppelklick auf eine `.tgj`-Datei auf Systemen, in denen der Dateityp registriert ist (das Windows-Installationsprogramm und `make install` tun dies);
- eine Datei auf der Kommandozeile angeben: `tg-expert file.tgj`. Eine Sitzung ist eine einzige Datei: Werden mehrere angegeben, wird nur die erste geöffnet.

Jede Aufnahme öffnet sich in einem eigenen Reiter, der gespeicherte Stand in einem Reiter namens #ui[Stand beim Speichern]. Die Messungen der Datei bleiben, wie sie sind: Sie lassen sich weder einer anderen Lage zuordnen noch schließen. Aufnahmen lassen sich aber umbenennen; der Name gehört nicht zur Messung. Sie lassen sich auch nicht als Live-Messung fortsetzen, da die internen Puffer nicht gespeichert werden.

Nach dem Öffnen kann die Sitzung weitergehen, etwa nach einem unerwarteten Programmende. Der Echtzeit-Reiter misst wie gewohnt, seine Langzeitstatistik beginnt bei null, und neue Aufnahmen werden der Sitzung hinzugefügt. Das nächste #ui[Speichern] behält alles aus der Datei, einschließlich ihres #ui[Stand beim Speichern], und fügt die neuen Aufnahmen und einen neuen gespeicherten Stand hinzu. Eine Sitzung kann so mehrere gespeicherte Stände enthalten, einen für jeden Abschnitt der Aufzeichnung; jeder behält seine eigene Statistik und seine Einstellungen, und sie werden nicht zusammengeführt.

Das Öffnen einer Sitzung ändert weder die in der oberen Leiste gewählte Uhr noch ihre bph oder ihren Hebungswinkel noch die Kalibrierung. Ein Hinweis zeigt, wie die Uhr der Sitzung zu den gespeicherten Uhren steht: #ui[Diese Uhr verwenden] wählt sie, um mit ihren Einstellungen weiterzumessen. Hat die gespeicherte Uhr inzwischen andere Einstellungen als die Sitzung oder gibt es sie nicht, sagt der Hinweis das und es wird nichts geändert.

== Die Uhr einer Sitzung
Die Uhr einer Sitzung ist die in der oberen Leiste gewählte. Wird eine andere Uhr gewählt, wenn die Sitzung bereits Aufnahmen oder eine Datei hat, fragt das Programm, ob eine #ui[Neue Sitzung] für sie begonnen oder diese Sitzung mit ihr #ui[verknüpft] werden soll. In beiden Fällen behalten die Aufnahmen ihre Namen und ihre eigenen bph und Hebungswinkel.

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

Eine Datei kann außerdem Projektangaben (Projektname, Auftragsreferenz, Uhrenbezeichnung, Eigentümer, Datum der letzten Revision und allgemeine Notizen) sowie Notizen zu jeder Aufnahme enthalten. Diese Felder sind optional. Dateien ohne sie öffnen sich wie bisher; ältere Programme überspringen sie beim Lesen, verwerfen sie aber, wenn sie die Datei erneut speichern. Ältere Programme zeigen einen gespeicherten Stand als weitere Aufnahme, benannt #ui[Stand beim Speichern] in der Sprache, in der gespeichert wurde. Mit älteren Versionen gespeicherte Dateien öffnen sich als Sitzung ohne Projektangaben; der Name ihrer Aufnahmen wird nie als Name einer Uhr übernommen. Die Uhrenbezeichnung wird als Text in der Datei gespeichert: Das Öffnen einer Datei legt keine gespeicherte Uhr an, ändert oder übernimmt keine und verändert die Kalibrierung nicht.
