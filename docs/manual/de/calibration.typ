#import "../lib.typ": *

= Kalibrierung <calibration>
== Warum
Ein Computer enthält mehrere interne elektronische Taktgeber. 
Hauptplatine und Prozessor verwenden eigene Taktreferenzen. 
Für tg-expert ist jedoch vor allem der Takt der Soundkarte 
oder des Audio-Interfaces maßgebend: Er bestimmt die tatsächliche 
Audio-Abtastrate und damit die Zeitskala, mit der das Programm 
die Schläge der Uhr misst.

Diese Taktgeber basieren meist auf preiswerten Quarzen. Ihre 
Abweichung kann einige zehn Millionstel (parts per million, ppm) 
betragen und ändert sich leicht mit der Temperatur. Eine ppm
bedeutet ein Millionstel; als Zeitabweichung entspricht dies 0,0864 
Sekunden pro Tag. So entsprechen beispielsweise 50 ppm einer Abweichung von 4,32 s/t. 
Auch wenn dies gering erscheint, geht dieser Fehler direkt in die 
von tg-expert berechnete Gangabweichung (RATE) ein.

Bei der Kalibrierung wird die tatsächliche Abweichung des 
Soundkartentakts gegen eine Quarzreferenz gemessen und in s/d 
angegeben. Das Programm zieht diesen Wert anschließend von den
Messungen ab, sodass RATE die Gangabweichung der geprüften Uhr 
und nicht den Fehler der Computer-Zeitbasis wiedergibt.

== Wie
Sie benötigen eine Quarzuhr oder Quarzwanduhr, die einmal pro Sekunde tickt,
zum Beispiel eine analoge Quarzarmbanduhr mit Sekundenzeiger. Ihre Genauigkeit
bestimmt die Genauigkeit der Kalibrierung, typischerweise besser als 0.5 s/t.
1. Die Quarzuhr ganz nah an das Mikrofon legen, am besten direkt daran.
2. In einem ruhigen Raum arbeiten. Lüfter und Lautsprecher in der Nähe
  ausschalten und den Tisch nicht berühren.
3. Im Menü #ui[Werkzeuge] #ui[Kalibrierung starten] wählen. Die rechte Fensterseite zeigt die
  Kalibrierungsanleitung.
4. Den Status in der Ergebnistabelle beobachten:
  - *cal (wait)*: noch kein stabiler Tick pro Sekunde gefunden. Den Aufbau
    prüfen.
  - *cal (acq.) NN %*: Ticks werden gesammelt. Nichts berühren.
  - *Vorläufige Schätzung*: erscheint getrennt, sobald genügend gültige
    Beobachtungen vorliegen. Sie ist mit `~` markiert, kann sich ändern und
    ändert die aktuelle Korrektur nicht.
  - *cal (done) ±X.X s/d*: abgeschlossen. Der Wert wird in das Feld #ui[cal]
    übernommen und gespeichert.
  - *cal (fail)*: das Signal war zu unregelmäßig. Den Aufbau verbessern und
    erneut starten.
5. Sobald *done* angezeigt wird, im Menü #ui[Werkzeuge] #ui[Kalibrierung beenden] wählen, um
  zur normalen Messung zurückzukehren.

Wenn Sie vor Abschluss stoppen und eine brauchbare Schätzung vorliegt, fragt
tg-expert, ob sie übernommen werden soll. Die bisherige Korrektur bleibt
erhalten, sofern Sie die Übernahme nicht ausdrücklich bestätigen. Eine
übernommene Schätzung wird zum normalen gespeicherten Kalibrierwert und ist
nicht mehr vorläufig. Bei zu wenigen gültigen Beobachtungen bleibt beim Stoppen
einfach die bisherige Korrektur erhalten.
Mit den Standard-Algorithmuseinstellungen dauert die Kalibrierung etwa
15 Minuten (900 Ticks); andere Einstellungen können die Dauer ändern. Wiederholen Sie sie
einmal, um das Ergebnis zu bestätigen. Zwei Durchläufe sollten auf wenige
Zehntelsekunden pro Tag übereinstimmen.
#shot("calibration.png", [Kalibrierung läuft, bei 5 %. Das Gangdiagramm zeigt
die Referenzticks, die rechte Seite die Anleitung.])
== Wie lange sie gilt
Der Wert cal gehört zur Soundkarte und ihrem Takt, nicht zur Uhr. Er wird
gespeichert und bei jedem Start verwendet. Kalibrieren Sie erneut:
- wenn Sie den Audioeingang oder die Abtastrate ändern;
- wenn Sie einen anderen Computer oder eine andere Soundkarte verwenden;
- wenn die Raumtemperatur stark abweicht, da der Quarz mit der Temperatur
  etwas driftet;
- von Zeit zu Zeit zur Kontrolle. Einmal im Jahr ist eine sinnvolle
  Gewohnheit.
Der Wert cal lässt sich auch im Feld #ui[cal] eingeben, wenn er von einer
früheren Kalibrierung bekannt ist.

== Experteneinstellungen für die Kalibrierung <calibration-parameters>

Normalerweise müssen Sie diese Werte nicht ändern. Falls Sie ihre Wirkung
verstehen und eine Änderung nötig ist, beenden Sie zuerst die Kalibrierung
und öffnen die #ui[Algorithmuskonfiguration…]. Auf dem Reiter #ui[Kalibrierung]
gibt #ui[Kalibrierungseinstellungen bearbeiten (Experte)] die sechs Felder frei.
Ausschalten sperrt sie, ohne Änderungen zu verwerfen oder zu übernehmen.
Anwenden und Speichern funktionieren wie in @algorithm-configuration:
Die Analyse startet neu, und die INI speichert nur Abweichungen vom Standard.
Lassen Sie die Bearbeitung aktiviert, wenn auch diese Felder zurückgesetzt
werden sollen.

Die Einstellungen gehören zur Quarzreferenz, nicht zum später gemessenen
Uhrwerk. Die Referenz tickt weiterhin einmal pro Sekunde; das Programm fasst
die Geräusche in einer einsekündigen Signalform zusammen und verfolgt ihre
zeitliche Drift zur Berechnung der Korrektur. Prüfen Sie zuerst Aufstellung
und Ruhe nach der Anleitung oben, bevor Sie die Erkennung verändern.

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI-Schlüssel], [Standard], [Bereich],
  [Anzahl der Beobachtungen], [`cal_count`], [900], [300–3600],
  [Kalibrierungsflankenschwelle (%)], [`cal_edge`], [5], [3–10],
  [Mindestabstand der Beobachtungen (s)], [`cal_interval`], [0.9], [0.5–2],
  [Akzeptanzgenauigkeit der Anpassung (s/Tag)], [`cal_precision`], [0.1], [0.1–0.5],
  [Untere Position in der gefalteten Sekunde (%)], [`cal_lock_low`], [40], [35–45],
  [Obere Position in der gefalteten Sekunde (%)], [`cal_lock_high`], [60], [55–65],
)

/ Anzahl der Beobachtungen:
  So viele gültige Beobachtungen werden vor der Anpassung gesammelt.
  Es ist keine Dauer in Sekunden: Eine höhere Anzahl erfordert meist,
  die Referenz länger ungestört liegen zu lassen.
/ Kalibrierungsflankenschwelle:
  Hilft, den Beginn des Referenzimpulses zu finden. Zum umgebenden
  Hintergrundmaximum wird dieser Anteil des Abstands zum gefalteten
  Impulsmaximum addiert. Die Schwelle ist weder eine absolute
  Mikrofonlautstärke noch eine Position innerhalb der Sekunde.
/ Mindestabstand der Beobachtungen:
  Verhindert die Annahme zu dicht aufeinanderfolgender Beobachtungen.
  Seit der vorigen muss mehr als diese Zeit vergangen sein. Das garantiert
  keinen entsprechenden Aktualisierungstakt: Fehlende Impulse oder ein
  längerer Abstand verlängern das Sammeln.
/ Akzeptanzgenauigkeit der Anpassung:
  So viel Unsicherheit darf die Kalibrierung zur Annahme haben.
  Der berechnete Wert muss darunter liegen, in s/Tag. Erhöhen erlaubt
  eine weniger genaue Anpassung, verbessert aber weder die Referenz
  noch bestimmt es die später gespeicherte Korrektur.
/ Untere Position in der gefalteten Sekunde:
  Beginnt den Bereich, in dem das Referenzmaximum liegen darf; die
  gewählte steigende Flanke muss nach dieser Position kommen. Die üblichen
  40% bedeuten 400 ms innerhalb der einsekündigen Signalform, keinen
  Beobachtungsabstand.
/ Obere Position in der gefalteten Sekunde:
  Schließt den erlaubten Bereich für das Referenzmaximum. Die
  voreingestellten 60% entsprechen 600 ms innerhalb der einsekündigen
  Signalform. Zusammen mit der unteren Grenze beschreibt dies die
  erwartete Lage des Maximums, keine zusätzliche Flankenschwelle.
