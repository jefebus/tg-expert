#import "../lib.typ": *

= Dateien <files>
Messungen werden in Dateien mit der Endung `.tgj` gespeichert.

== Sitzungen
Eine Sitzung ist eine `.tgj`-Datei mit allem, was an einer Uhr gemessen wurde: die Projektangaben, die Uhr, ihre Abschnitte und ihre Aufnahmen (siehe @runs). Die Titelleiste zeigt den Projektnamen und die Datei; `*` bedeutet, dass es ungespeicherte Änderungen gibt.

- #ui[Neue Sitzung] beginnt eine leere Sitzung für die in der oberen Leiste gewählte Uhr.
- #ui[Sitzung öffnen…] ersetzt die aktuelle Sitzung durch die einer Datei.
- #ui[Speichern] schreibt die ganze Sitzung in ihre Datei, gleich welcher Reiter angezeigt wird. Beim ersten Mal wirkt es wie #ui[Speichern unter…].
- #ui[Speichern unter…] schreibt die Sitzung in eine andere Datei, die dann zur Sitzungsdatei wird.

Bevor eine Sitzung mit ungespeicherten Änderungen ersetzt oder das Programm beendet wird, fragt das Programm, ob sie gespeichert werden sollen. Änderungen sind neue Abschnitte und Aufnahmen, umbenannte, geschlossene oder mit einer anderen Lage versehene Aufnahmen und die Projektangaben; wird in einer Sitzung mit Abschnitten, Aufnahmen oder Datei aufgezeichnet, auch neue Echtzeit-Messwerte. Das Beobachten des pausierten Echtzeit-Reiters ist keine Änderung.

== Abschnitte und Aufnahmen <runs>
Das Programm zeichnet in *Abschnitten* auf. Ein Abschnitt ist eine Zeit ununterbrochener Aufzeichnung: Er beginnt mit #ui[Aufzeichnen] und endet mit #ui[Pause], beides über die Schaltfläche neben #ui[Messung festhalten]. Der Titel des Echtzeit-Reiters zeigt den Zustand: #ui[Echtzeit ● Abschnitt 2] während der Aufzeichnung, #ui[Echtzeit ❚❚ pausiert] sonst.

- Beim Programmstart und mit #ui[Neue Sitzung] beginnt die Aufzeichnung von Abschnitt 1. Eine geöffnete Sitzung ist pausiert.
- #ui[Pause] beendet den Abschnitt. Er kommt in einen eigenen Reiter, #ui[Abschnitt 1], #ui[Abschnitt 2]…, mit seinen Seiten Verlauf und Korrelationen und dem Echtzeit-Reiter, wie er am Ende war. Seine Messung bleibt, wie sie ist: Er lässt sich weder schließen noch einer anderen Lage zuordnen. Im Feld #ui[Aktueller Abschnitt] kann er einen Namen erhalten, der nach seiner Nummer steht: #ui[Abschnitt 2 nach dem Regulieren].
- #ui[Aufzeichnen] beginnt einen neuen Abschnitt. Sein Verlauf beginnt bei null: Abschnitte sind getrennte Messungen und werden nie zusammengeführt, auch wenn sie aufeinander folgen. Nach einer Pause von mehr als 10 Minuten zeigt ein Hinweis ihre Dauer.
- In der Pause analysiert der Echtzeit-Reiter weiter den Klang, um Uhr und Mikrofon auszurichten, aber es wird nichts aufgezeichnet und keine Aufnahme gemacht.

Eine *Aufnahme* hält den Moment fest: Messwerte, Gangdiagramm und Signalformen. Sie unterbricht die Aufzeichnung nicht und hat daher keinen eigenen Verlauf; der Verlauf gehört zu ihrem Abschnitt, den der Tooltip ihres Reiters nennt. Von älteren Versionen mit Verlauf gespeicherte Aufnahmen behalten ihn.

Aufnahmen werden innerhalb ihres Abschnitts nummeriert: #ui[2.1], #ui[2.2]… sind die erste und zweite Aufnahme von Abschnitt 2, gefolgt von der Lage, falls gesetzt, wie in #ui[2.3 (DU)]. Ein im Feld #ui[Aktuelle Aufnahme] eingegebener Name steht nach der Nummer: #ui[2.3 nach dem Regulieren (DU)]. Die Nummer bleibt: Das Schließen einer Aufnahme nummeriert die anderen nicht neu. Aufnahmen aus Dateien älterer Versionen, die keinen Abschnitt haben, behalten ihren Namen.

Die Reiter folgen nach #ui[Echtzeit] in fester Reihenfolge: zuerst die Aufnahmen des laufenden Abschnitts, dann der neueste Abschnitt mit seinen Aufnahmen, dann der vorherige Abschnitt und so weiter. In jeder Gruppe kommt die neueste Aufnahme zuerst. Aufnahme-Reiter zeigen eine Kamera. Aufnahmen aus Dateien älterer Versionen, die keinen Abschnitt haben, stehen am Ende.

Jeder Abschnitt und jede Aufnahme hat eigene Notizen, die auf der Seite #ui[Messung] des Notizbereichs geschrieben werden (siehe @notes-panel). Im Echtzeit-Reiter enthält diese Seite die Notizen des laufenden Abschnitts: Sie gehen bei #ui[Pause] mit dem Abschnitt in seinen Reiter und beim #ui[Speichern] in die Datei.

== Speichern
#ui[Speichern] schreibt die Abschnitte, die Aufnahmen und den laufenden Abschnitt bis zu diesem Moment, mit dem Echtzeit-Reiter, wie er gerade ist. Erneutes Speichern schreibt den laufenden Abschnitt erneut: Er wird nicht verdoppelt. Während der Kalibrierung wird der Echtzeit-Reiter nicht gespeichert.

Der Speicherdialog schlägt `JJJJ-MM-TT Projektname.tgj` (mit dem heutigen Datum) vor, oder den Uhrennamen, wenn es keinen Projektnamen gibt. Zeichen, die in Dateinamen nicht erlaubt sind, werden durch `_` ersetzt; der in der Sitzung gespeicherte Projektname bleibt unverändert. Die Endung `.tgj` wird bei Bedarf ergänzt.

Die Datei wird zuerst in eine temporäre Datei im selben Ordner geschrieben und ersetzt die vorherige erst, wenn sie vollständig geschrieben ist. Schlägt das Speichern fehl, bleibt die vorherige Datei unverändert und die Sitzung behält ihre ungespeicherten Änderungen.

#note[Verwenden Sie kurze, dateisystemtaugliche Namen, wenn Dateien zwischen Windows, Linux und macOS ausgetauscht werden.]

== Öffnen
- #ui[Sitzung öffnen…] im Menü #ui[Datei];
- Doppelklick auf eine `.tgj`-Datei auf Systemen, in denen der Dateityp registriert ist (das Windows-Installationsprogramm und `make install` tun dies);
- eine Datei auf der Kommandozeile angeben: `tg-expert file.tgj`. Eine Sitzung ist eine einzige Datei: Werden mehrere angegeben, wird nur die erste geöffnet.

Jeder Abschnitt und jede Aufnahme öffnet sich in einem eigenen Reiter, und die Sitzung ist pausiert. Die Messungen der Datei bleiben, wie sie sind: Sie lassen sich weder einer anderen Lage zuordnen noch schließen. Abschnitte und Aufnahmen lassen sich aber umbenennen; der Name gehört nicht zur Messung. Ein Abschnitt lässt sich nicht fortsetzen, da die internen Puffer nicht gespeichert werden.

Um weiterzumessen, etwa nach einem unerwarteten Programmende, klicken Sie auf #ui[Aufzeichnen]: Ein neuer Abschnitt mit der nächsten Nummer beginnt, und neue Aufnahmen gehören zu ihm. Das nächste #ui[Speichern] behält alles aus der Datei und fügt den neuen Abschnitt und die neuen Aufnahmen hinzu.

Das Öffnen einer Sitzung ändert weder die in der oberen Leiste gewählte Uhr noch ihre bph oder ihren Hebungswinkel noch die Kalibrierung. Ein Hinweis zeigt, wie die Uhr der Sitzung zu den gespeicherten Uhren steht: #ui[Diese Uhr verwenden] wählt sie, um mit ihren Einstellungen weiterzumessen. Hat die gespeicherte Uhr inzwischen andere Einstellungen als die Sitzung oder gibt es sie nicht, sagt der Hinweis das und es wird nichts geändert.

== Die Uhr einer Sitzung
Die Uhr einer Sitzung ist die in der oberen Leiste gewählte. Wird eine andere Uhr gewählt, wenn die Sitzung bereits Aufnahmen oder eine Datei hat, fragt das Programm, ob eine #ui[Neue Sitzung] für sie begonnen oder diese Sitzung mit ihr #ui[verknüpft] werden soll. In beiden Fällen behalten die Aufnahmen ihre Namen und ihre eigenen bph und Hebungswinkel.

== Inhalt einer Datei
Für jeden Abschnitt und jede Aufnahme:
- den Aufnahmenamen und die Lage der Uhr;
- Datum und Uhrzeit der Aufnahme oder Beginn und Ende des Abschnitts (Dateien älterer Versionen enthalten sie nicht);
- die Ergebnisse: Gangabweichung, Amplitude, Abfallfehler, bph, Hebungswinkel und cal;
- die im Gangdiagramm angezeigten Schläge einschließlich Tick/Tack-Zuordnung und Amplitudenverlauf;
- die gemittelten Signalformen;
- Zoom und Verschiebung des Gangdiagramms;
- für einen Abschnitt seine Nummer, seine Langzeitstatistik und die Diagramme der Seite Verlauf; für eine Aufnahme den Abschnitt, in dem sie gemacht wurde, und ihre Nummer in diesem Abschnitt;
- seine Notizen.

Die Datei enthält nicht die Audioaufnahme.

== Bericht (PDF)
#ui[Bericht exportieren…] im Menü #ui[Datei] (Strg+P) schreibt einen PDF-Bericht der Sitzung auf A4-Seiten. Der Dialog zeigt die Abschnitte wie die Reiter, den neuesten zuerst, einschließlich des laufenden (#ui[Abschnitt 3 (läuft)]). Unter jedem Abschnitt:

- #ui[Verlauf]: was die Seiten Verlauf und Korrelationen für den Abschnitt zeigen: die Statistik, die Diagramme von Gangabweichung, Amplitude und Abfallfehler und die drei Korrelationsdiagramme, über den ganzen Abschnitt.
- #ui[Endstand]: der Echtzeit-Reiter, wie er am Ende des Abschnitts oder beim Speichern der Sitzung war. Für den laufenden Abschnitt ist es der #ui[Stand beim Export]. Standardmäßig nur bei Abschnitten ohne Aufnahmen gewählt.
- seine Aufnahmen, jede mit einem eigenen Kästchen.

Wird ein Abschnitt abgewählt, entfällt alles darunter. Unter den Abschnitten:

- #ui[Aktueller Stand]: der Echtzeit-Reiter, wie er beim Export ist: Messwerte, Gangdiagramm und Signalformen. Standardmäßig nicht gewählt; ohne gültige Messung oder während der Kalibrierung nicht verfügbar.
- #ui[Weitere Aufnahmen (ältere Dateien)]: die Aufnahmen aus Dateien, die vor Einführung der Abschnitte gespeichert wurden, sofern vorhanden.
- #ui[Tabelle der Abschnitte auf der ersten Seite].

Die Diagramme sind farbig oder schwarz und grau (#ui[Schwarzweiß]), zum Drucken. Der Eigentümer wird nur aufgenommen, wenn #ui[Eigentümer aufnehmen] gewählt ist.

Der Bericht ist so aufgebaut:

- Jede Seite hat einen Kopf mit dem Programmlogo, dem Berichtstitel (dem Projektnamen oder der Uhr), der Auftragsreferenz und dem Datum.
- Die erste Seite hat ein Feld mit Projektname, Auftragsreferenz, Uhr, Datum der letzten Revision und Berichtsdatum, sofern vorhanden, die allgemeinen Notizen und die Tabelle der Abschnitte: für jeden Abschnitt Name, Zeitraum, Dauer, gemessene Lagen, mittlere Gangabweichung, mittlere Amplitude, mittlerer Abfallfehler und Zahl der Aufnahmen, und darunter eine Zeile für jede seiner Aufnahmen im Bericht, mit Lage, Datum und Uhrzeit und Messwerten.
- Danach jeder gewählte Abschnitt in der Reihenfolge der Aufzeichnung (Abschnitt 1 zuerst), ab einer neuen Seite: Nummer und Name, Zeitraum und Dauer, seine Notizen, sein Verlauf, sein Endstand und seine Aufnahmen. Der Endstand und jede Aufnahme stehen auf einer eigenen Seite: zuerst ihre Notizen, dann Messwerte, Gangdiagramm, Signalformen und Zeitpunkt.
- Am Ende der aktuelle Stand und die weiteren Aufnahmen.

#shot("pdf-export.png", [Beispiel der ersten Seite eines exportierten PDF-Berichts.],
width: 85%)

Die Daten werden beim Klick auf #ui[Exportieren] übernommen; der Bericht zeigt also einen einzigen Zeitpunkt, auch wenn die Messung weiterläuft. Er wird zuerst in eine temporäre Datei geschrieben und ersetzt eine vorhandene Datei erst, wenn er vollständig ist. Der Export ändert nie die Sitzung oder ihre Notizen.

Die angezeigten bph, Hebungswinkel und cal sind die, mit denen jede Messung gemacht wurde. Die Signalformen einer Aufnahme sind der gemittelte Klang ihres eigenen Analysefensters, nicht eines ganzen Tests. Der Bericht ist kein Konformitätszertifikat nach irgendeiner Norm.

=== Notizen im Bericht
Notizen werden als reiner Text geschrieben und im Bericht mit diesem Teil von Markdown dargestellt:

- eine Leerzeile beginnt einen neuen Absatz; ein einfacher Zeilenumbruch bleibt erhalten;
- `#`, `##` und `###` am Zeilenanfang ergeben Überschriften;
- `**fett**`, `*kursiv*` und `_kursiv_`;
- Zeilen, die mit `-`, `*` oder `+` und einem Leerzeichen oder mit einer Zahl und einem Punkt (`1.`) beginnen, ergeben Listen.

Alles andere (Links, Tabellen, Code…) wird so gezeigt, wie es geschrieben ist: Kein Text wird ausgelassen.

== Kompatibilität
Mit Tg und tg-timer gespeicherte Dateien lassen sich in tg-expert öffnen. Aufnahmen ohne Langzeitstatistik zeigen auf der Seite Verlauf eine Meldung statt der Statistik. Wird eine tg-expert-Datei in einem älteren Programm geöffnet und erneut gespeichert, können Verlaufsdaten verloren gehen.

Eine Datei kann außerdem Projektangaben (Projektname, Auftragsreferenz, Uhrenbezeichnung, Eigentümer, Datum der letzten Revision und allgemeine Notizen) sowie Notizen zu jeder Aufnahme enthalten. Diese Felder sind optional. Dateien ohne sie öffnen sich wie bisher; ältere Programme überspringen sie beim Lesen, verwerfen sie aber, wenn sie die Datei erneut speichern. Ältere Programme zeigen einen Abschnitt als weitere Aufnahme, benannt #ui[Abschnitt 1], #ui[Abschnitt 2]… in der Sprache, in der gespeichert wurde. In Dateien, die tg-expert vor Einführung der Abschnitte gespeichert hat, öffnet sich jeder #ui[Stand beim Speichern] als Abschnitt. Aufnahmen eines Abschnitts, die vor Einführung ihrer Nummer gespeichert wurden, werden beim Öffnen in der Reihenfolge nummeriert, in der sie gemacht wurden. Mit älteren Versionen gespeicherte Dateien öffnen sich als Sitzung ohne Projektangaben; der Name ihrer Aufnahmen wird nie als Name einer Uhr übernommen. Die Uhrenbezeichnung wird als Text in der Datei gespeichert: Das Öffnen einer Datei legt keine gespeicherte Uhr an, ändert oder übernimmt keine und verändert die Kalibrierung nicht.
