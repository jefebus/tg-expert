#import "../lib.typ": *

= Langzeitmessung <trend>
Die Gangabweichung einer Uhr ändert sich, wenn sich die Zugfeder entspannt
und wenn die Uhr zwischen Lagen wechselt. Die Seite #ui[Verlauf] zeichnet die
Messung über Stunden auf, sodass diese Änderungen sichtbar werden.
#shot("trend.png", [Die Seite Verlauf nach einer Messung in drei Lagen:
Zifferblatt oben, Zifferblatt unten und Krone oben.])
== Aufzeichnung
Alle 2 Sekunden, solange das Signal gut ist, zeichnet das Programm
Gangabweichung, Amplitude und Abfallfehler mit der gewählten Lage auf.
Zeiträume ohne gutes Signal werden ausgelassen.
Die Aufzeichnung beginnt neu, wenn sich eine Einstellung ändert, die die
Ergebnisse beeinflusst: bph, Hebungswinkel, cal, der Audioeingang, die
Abtastrate oder die Option #ui[Leichter Algorithmus]. #ui[Zurücksetzen] startet sie von
Hand neu. Nach 100 Stunden endet die Aufzeichnung.
Ist #ui[BEAT] auf #ui[Schätzung] eingestellt, kann das Programm kurzzeitig
eine andere Schlagzahl erkennen, während die Uhr gedreht wird oder das Signal
schwach ist. Diese Messwerte werden ausgelassen und die Aufzeichnung läuft
weiter. Nur eine geschätzte Schlagzahl, die eine Minute lang abweicht, startet
die Aufzeichnung neu. Bei langen Aufzeichnungen sollte die Schlagzahl von
Hand eingestellt werden.
Während der Kalibrierung wird nichts aufgezeichnet.

== Stabilisierung nach einem Lagenwechsel

Stellen Sie auf der Live-Verlaufsseite die #ui[Stabilisierungszeit (s)] ein
(standardmäßig 0, was sie deaktiviert). Die Einstellung bleibt
zwischen Sitzungen erhalten; Änderungen gelten für den nächsten Countdown.

Wählen Sie die neue Lage zuerst in der Software und bewegen Sie *danach* die
Uhr. Beim Wechsel von einer eingestellten Lage zu einer anderen, auch zu
#ui[ohne], beginnt der Countdown sofort und läuft auch ohne gutes Signal
weiter. Die Rückkehr zu einer zuvor verwendeten Lage startet ihn erneut. Die
erste Auswahl aus #ui[ohne] und eine bereits beim Aufzeichnungsbeginn
eingestellte Lage starten keinen Countdown. #ui[Zurücksetzen] startet oder
verlängert keinen Countdown; ein bereits laufender Countdown läuft weiter.

Messwerte mit gutem Signal während des Countdowns bleiben erhalten, werden
in den Diagrammen hinterlegt und im CSV als #ui[Stabilisierung] markiert.
Sie werden aus der Gesamt- und Lagenstatistik einschließlich Lagenfehler und
Diagramm-Mittelwerten ausgeschlossen. Die Spalte *aktuell* zeigt weiterhin
den letzten Messwert. Aufnahmen speichern die Markierungen; ältere
Programmversionen können sie ignorieren.

== Statistik seit dem Start
Die Tabelle oben zeigt für Gangabweichung (s/t), Amplitude (°) und
Abfallfehler (ms):
- *aktuell*: den letzten Messwert;
- *Mittelw.*: den Mittelwert;
- *Min.* und *Max.*;
- *Spanne*: Max. minus Min.;
- *σ*: die Standardabweichung.
Darunter zeigt die Seite, wie lange die Aufzeichnung läuft, welchen Anteil
dieser Zeit ein gutes Signal vorlag, und die verwendeten Werte für bph,
Hebungswinkel und cal.
== Statistik pro Lage
Wurde die Lage während der Aufzeichnung eingestellt, zeigt eine zweite
Tabelle für jede Lage die darin verbrachte Zeit sowie die mittlere
Gangabweichung, Amplitude und den mittleren Abfallfehler. Messwerte ohne
eingestellte Lage werden unter #ui[ohne] aufgeführt.
Bei zwei oder mehr Lagen zeigt die Seite den *Lagenfehler*: die größte
Differenz zwischen den mittleren Gangabweichungen der Lagen. Ein kleiner
Lagenfehler bedeutet, dass die Uhr gut reguliert ist.
== Diagramme
Unten teilen sich drei Diagramme eine Zeitachse: Gangabweichung, Amplitude
und Abfallfehler. Jedes zeigt eine gestrichelte Linie für den Mittelwert,
wenn dieser im sichtbaren Bereich liegt. Eine senkrechte Linie mit dem
Lagenkürzel (DU, DD, ...) markiert jeden Lagenwechsel. Die Bedienelemente
unten auf der Seite legen fest:
/ Gangabweichung: den vertikalen Bereich des Gangabweichungs-Diagramms:
  #ui[Auto], #ui[±10 s/t] oder #ui[±50 s/t].
/ Abfallfehler: #ui[Alle] skaliert den Abfallfehler automatisch; < 1 ms und
  < 2 ms legen die Obergrenze fest. Größere Werte erscheinen am Rand; Daten
  und Statistiken bleiben unverändert. Die Einstellung gilt auch für Korrelationen.
/ Zeige: den angezeigten Zeitraum: #ui[Alle], #ui[Letzte Std.],
  #ui[Letzte 6 Std.] oder #ui[Letzte 24 Std.].

== Korrelationen

Die Seite #ui[Korrelationen] verwendet dieselben aufgezeichneten Messwerte.
Die drei Streudiagramme zeigen Amplitude → Gangabweichung, Abfallfehler →
Gangabweichung und Abfallfehler → Amplitude. Die Punkte werden nicht
verbunden; ein Zusammenhang beweist keine Ursache. Der Lagenfilter zeigt alle
Lagen als getrennte Reihen oder eine einzelne Lage. Farben und Symbole sind
in allen Diagrammen gleich. #ui[(nicht gesetzt)] bezeichnet eine unbekannte,
keine weitere physische Lage.

Der Zeitraum #ui[Zeige] und die Achsenbereiche werden unabhängig vom
Lagenfilter mit Verlauf geteilt. Werte außerhalb dieser Bereiche erscheinen
am Rand. Ein Hinweis beim Überfahren eines Punkts zeigt die vergangene Zeit,
aufgezeichnete Lage und verfügbaren Werte. Ein Messwert ohne Amplitude kann
im Diagramm Abfallfehler → Gangabweichung erscheinen; null ist für beide
Werte gültig. Als Stabilisierung markierte Messwerte werden ausgeschlossen,
beeinflussen aber weiterhin die Achsenbereiche von Verlauf. In alten Dateien
ohne solche Markierungen lassen sie sich nicht identifizieren. Werte mit nur
Gangabweichung werden nicht im Verlauf aufgezeichnet.

Jeder Messwert behält die bei der Aufzeichnung gewählte Lage. Der Verlauf
speichert nicht den Beginn des Audiofensters des Detektors; Fenster über
einen Lagenwechsel lassen sich daher nicht zuverlässig erkennen oder
ausschließen. Alte Dateien ohne aufgezeichnete Lagen werden als
#ui[(nicht gesetzt)] dargestellt; Dateien ohne Verlauf enthalten keine
Korrelationspunkte.

== Daten exportieren

Wählen Sie im Befehlsmenü #ui[Verlauf als CSV exportieren], um die
Verlaufsdaten der aktiven Registerkarte zu speichern. Die Zeitwerte sind die
verstrichenen Sekunden seit Beginn der Aufzeichnung, keine Uhrzeiten. Die
Datei enthält Lagenkürzel und Messeinstellungen. Aufgezeichnet werden nur
Messwerte mit gutem Signal, daher erscheinen ausgelassene Zeiträume als
Lücken; die Zeilen sind als gültig oder in Stabilisierung markiert. Ist keine Amplitude
verfügbar, bleibt das Feld leer.

Der Algorithmus wird als #ui[Leicht (angepasst)] oder #ui[Vollständig
(angepasst)] gekennzeichnet, wenn ein in diesem Modus verwendeter
Messparameter vom Standard abweicht. Kalibrierungsparameter zählen nicht,
ebenso wenig Rauschunterdrückungsparameter im leichten Modus. Aufnahmen
bewahren diese Information; ältere Aufnahmen ohne sie werden als #ui[Leicht]
oder #ui[Vollständig] exportiert.

Im Speicherdialog können Sie zwischen portablem CSV (Komma als Trennzeichen,
Dezimalpunkt) und lokalisiertem CSV (Semikolon als Trennzeichen,
Dezimalkomma) wählen. Das lokalisierte Format wird nur angezeigt, wenn die
Oberflächensprache Deutsch oder Spanisch ist. Die Auswahl wird gespeichert
und bei späteren Exporten, auch nach einem Programmneustart, wiederverwendet.
Spaltennamen und beschreibende Werte richten sich nach der
Oberflächensprache; die Lagenkürzel bleiben unverändert.
Beide Formate verwenden UTF-8 mit Byte-Reihenfolge-Markierung, damit Excel
Sonderzeichen erkennt.

== Gangreserve messen
1. Die Uhr voll aufziehen und die Lage einstellen.
2. Die Messung starten und die Uhr ungestört liegen lassen.
3. Nach einigen Stunden zurückkehren. Das Amplitudendiagramm zeigt, wie die
  Amplitude mit dem Ablaufen der Zugfeder sinkt, und das
  Gangabweichungs-Diagramm zeigt, wie die Gangabweichung ihr folgt.
4. Eine Aufnahme machen und speichern. Die Daten des Verlaufs werden mit
  gespeichert.
#note[*Vor dem Ändern von Einstellungen:* Erstellen und speichern Sie eine Aufnahme, wenn die aktuelle Verlaufsaufzeichnung erhalten bleiben soll. Änderungen an bph, Hebungswinkel, cal, Audioeingang, Abtastrate oder Algorithmuseinstellungen setzen die Verlaufsdaten zurück. Für lange Aufzeichnungen die bekannte Schlagzahl eingeben, anstatt sich auf die automatische Erkennung zu verlassen.]
