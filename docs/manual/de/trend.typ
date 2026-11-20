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
wenn dieser im sichtbaren Bereich liegt. Das Abfallfehler-Diagramm wählt
seinen vertikalen Bereich automatisch. Eine senkrechte Linie mit dem
Lagenkürzel (DU, DD, ...) markiert jeden Lagenwechsel. Die Bedienelemente
unten auf der Seite legen fest:
/ Gangabweichung: den vertikalen Bereich des Gangabweichungs-Diagramms:
  #ui[Auto], #ui[±10 s/t] oder #ui[±50 s/t].
/ Zeige: den angezeigten Zeitraum: #ui[Alle], #ui[Letzte Std.],
  #ui[Letzte 6 Std.] oder #ui[Letzte 24 Std.].
== Gangreserve messen
1. Die Uhr voll aufziehen und die Lage einstellen.
2. Die Messung starten und die Uhr ungestört liegen lassen.
3. Nach einigen Stunden zurückkehren. Das Amplitudendiagramm zeigt, wie die
  Amplitude mit dem Ablaufen der Zugfeder sinkt, und das
  Gangabweichungs-Diagramm zeigt, wie die Gangabweichung ihr folgt.
4. Eine Aufnahme machen und speichern. Die Daten des Verlaufs werden mit
  gespeichert.
#note[*Vor dem Ändern von Einstellungen:* Erstellen und speichern Sie eine Aufnahme, wenn die aktuelle Verlaufsaufzeichnung erhalten bleiben soll. Änderungen an bph, Hebungswinkel, cal, Audioeingang, Abtastrate oder Leichter Algorithmus setzen die Verlaufsdaten zurück. Für lange Aufzeichnungen die bekannte Schlagzahl eingeben, anstatt sich auf die automatische Erkennung zu verlassen.]
