#import "../lib.typ": *

= Der Messalgorithmus <algorithm>

TG-Expert nutzt den Hemmungsklang, um Gang und Unruhamplitude zu messen.
Es vergleicht mehrere Schläge, um Tick und Tack zu unterscheiden.
Die Geräusche müssen sich deutlich wiederholen; ein guter Mikrofonkontakt
hilft oft mehr als geänderte Algorithmuseinstellungen.

== Wie die Messung entsteht

=== Das verwendete Signal

Vor der Messung filtert das Programm langsame Schwingungen und bildet eine
Hüllkurve, die die Geräusche jedes Schlags hervorhebt. Im Vollmodus entfernt
es außerdem ungewöhnlich energiereiche Abschnitte, um Störungen zu reduzieren.
Die Rauschunterdrückung kann dabei einen sehr kräftigen Tick nicht selbst
von einer Störung unterscheiden. Verschwinden echte Schläge, prüfen Sie ihre Schwelle.

Für die Erkennung wird verglichen, wie gut das Signal nach einer zeitlichen
Verschiebung wieder zu sich selbst passt. Gesucht wird eine regelmäßige
Wiederholung, nicht das lauteste Geräusch. Mehr Aufnahmepegel bedeutet
also nicht unbedingt eine bessere Messung.

=== Schlagzahl und Gangabweichung

Die gemessene Periode ist ein vollständiger *Tick-zu-Tick*-Zyklus:
*7200/BPH Sekunden*. Der mittlere Abstand benachbarter Schläge beträgt
die Hälfte, *3600/BPH Sekunden*. Bei 14,400 BPH dauert der Zyklus beispielsweise
500 ms, der mittlere Tick-zu-Tack-Abstand 250 ms.

Wählen Sie die bekannte BPH des Uhrwerks, damit nahe ihrer Periode gesucht
wird. Auto schätzt den Rhythmus und wählt die nächstgelegene BPH-Voreinstellung.
Prüfen Sie überraschende Ergebnisse: Auch stabile Schätzungen können falsch
sein. Ein alternativer Kandidat im manuellen Modus ändert die gewählte BPH nicht.

RATE zeigt, wie viel die Uhr gegenüber diesem nominalen Rhythmus vor-
oder nachgeht. Auch die Genauigkeit des Soundkartentakts wirkt darauf;
@calibration beschreibt die Korrektur dieser Zeitbasis.

Im Vollmodus sind die Blöcke 2, 4, 8 und 16 Sekunden lang, im leichten Modus
1, 2, 4 und 8. Ergebnisse werden nach Stabilität ausgewählt. Nach Änderungen
die Blöcke neu füllen lassen: Eine als alt markierte Anzeige ist keine neue Messung.

=== Signalformen und Abfallfehler

Die Signalformen vereinen viele Zyklen in einem Bild. Vor der Mittelung
werden die höchsten Werte jeder Position entfernt, um einzelne Stöße
weniger zu gewichten. Die Anzeige zeigt einen repräsentativen Klang,
nicht die Aufnahme eines einzelnen Ticks.

Tack wird nahe der Zyklusmitte gesucht. Sind Tick-zu-Tack- und
Tack-zu-Tick-Abstände ungleich, entsteht Abfallfehler. Eine breitere Suche
kann bei tatsächlich ungleichen Schlägen helfen, korrigiert aber keinen
gleichmäßigen Vor- oder Nachgang.

=== Amplitude und Punkte im Gangdiagramm

Die Amplitude folgt aus akustischer Hebungszeit und einem zum Uhrwerk
passenden Hebungswinkel. Beide Schätzungen müssen über dem Minimum, unter
360° und nahe genug beieinander liegen. Eine plausible Zahl beweist keine
korrekte Impulserkennung.

Erkannte Hebungszeiten verfeinern auch B.E. und die Zeitpunkte der
Diagrammpunkte. Ohne Amplitude lässt sich der vorläufige Abfallfehler
anzeigen; ohne Tick/Tack-Trennung aktualisiert die derzeitige Implementierung
RATE nicht. Punkte werden einzeln gesucht. Daher können einige fehlen,
obwohl der Rhythmus erkannt wurde, besonders wenn um einen Schlag noch
nicht genügend Audio verfügbar ist.

== Vollmodus und leichter Modus

Der Vollmodus ist die übliche Wahl und der Standard. Der leichte Modus
verringert die Rechenlast und liefert früher Werte, ist aber meist weniger
stabil und störanfälliger: Er verarbeitet die Hälfte der Samples und
lässt die Rauschunterdrückung aus.

Dabei werden die drei Rauschregler ausgegraut, ohne Werte oder Hilfe zu
löschen. Zurück im Vollmodus sind sie wieder verfügbar. Die übrige Erkennung
arbeitet in beiden Modi weiter; BPH und Hebungswinkel bleiben unverändert.

== Konfigurationsparameter <algorithm-configuration>

#note(color: orange)[*Für erfahrene Nutzer:* Ändern Sie Algorithmusparameter
nur, wenn Sie ihre Bedeutung und ihren Einfluss auf die Erkennung verstehen.
Andernfalls lassen Sie die Standardwerte unverändert. Änderungen nach
Versuch und Irrtum können falsche Messwerte liefern, die trotzdem plausibel
aussehen.]

=== Dialog, Ebenen und Übernahme von Änderungen

Öffnen Sie #ui[Algorithmuskonfiguration…] im Menü. Unter #ui[Grundlegend]
wählen Sie den leichten Modus; #ui[Fortgeschritten] zeigt fünf häufiger
benötigte Bewegungseinstellungen. Für die Untersuchung einer bestimmten
Stufe ergänzt #ui[Experteneinstellungen anzeigen] dreizehn weitere unter
denselben Überschriften. Das Einblenden ändert oder übernimmt keine Werte.

Jede Gruppe lässt sich zusammenklappen, jede #ui[Hilfe] mit Maus oder
Tastatur öffnen. Vom Standard abweichende Werte sind zur leichteren
Kontrolle hervorgehoben. BPH, Hebungswinkel und Abtastrate bleiben in den
üblichen Bewegungs- und Audioeinstellungen. Der Reiter #ui[Kalibrierung]
ist getrennt und wird in @calibration-parameters erklärt.

- #ui[Anwenden] übernimmt und speichert Änderungen; der Dialog bleibt offen.
- #ui[Speichern] übernimmt und speichert ebenfalls, schließt dann den Dialog.
- #ui[Standardwerte wiederherstellen] setzt Bewegungseinstellungen und leichten
  Modus im Dialog zurück. Kalibrierungswerte werden nur bei aktivierter
  Bearbeitung zurückgesetzt. Übernommen wird der Reset erst mit Anwenden
  oder Speichern.
- #ui[Abbrechen] verwirft nicht übernommene Änderungen, macht aber ein früheres
  Anwenden nicht rückgängig.

Speichern Sie vor dem Anwenden eine Aufnahme zum Vergleich: Änderungen
starten die Analyse neu und löschen den Live-Verlauf. Filter und Puffer
werden dabei mit den neuen Werten aufgebaut. Kann die Konfiguration nicht
gespeichert werden, erscheint ein Fehler; die Änderungen werden nicht übernommen.

=== Einheiten, Bereiche und gespeicherte Werte

Die Tabellen helfen, den ursprünglichen Wert und den Einstellbereich
nachzuschlagen. Achten Sie auf *± ms*: 20 bedeutet 20 ms auf jeder Seite,
nicht insgesamt. Ohne ± beschreiben Millisekunden eine Fenster- oder
Abschnittslänge. *×* ist ein Faktor; jeder Prozentsatz bezieht sich auf
die verglichene Größe. Amplitude steht in Grad, Kalibrierung verwendet auch
Sekunden und s/Tag. Alle Zahlen verwenden einen Dezimalpunkt.

Ein Wert im erlaubten Bereich reicht nicht immer: Nach der Umrechnung
in Samples müssen die Fenster ins verfügbare Audio passen. Das Programm
prüft, ob mindestens ein Rauschreferenzabschnitt und Platz für die Glättung
bleiben und ob die Kalibrierungspositionen im mittleren Bereich der
gefalteten Sekunde geordnet sind.

Die INI-Datei speichert Änderungen unter `[algorithm]` mit den
Tabellenschlüsseln. Zurück auf Standard wird der Eintrag entfernt, sodass
nur Anpassungen bleiben. Gültige ältere Werte und unbekannte Schlüssel
werden erhalten. Ein ungültiger alter Wert löst eine Meldung aus; gemessen
wird dann mit dem Standard. Das Speichern von Bewegungseinstellungen
überschreibt keine gesperrten, unbearbeiteten Kalibrierungswerte.

=== Fortgeschrittene Bewegungseinstellungen

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI-Schlüssel], [Standard], [Bereich],
  [Grenzfrequenz für Audio und Hüllkurve (Hz)], [`filter`], [3000], [500–5000],
  [Rauschunterdrückungsschwelle (× Referenz)], [`noise_threshold`], [2], [1–3],
  [Halbbreite der Periodensuche (± ms)], [`guess_search`], [20], [10–30],
  [Suchhalbbreite für den Gegenschlag (± ms)], [`be_search`], [20], [10–30],
  [Minimal akzeptierte Unruhamplitude (°)], [`amp_min`], [135], [90–135],
)

/ Grenzfrequenz für Audio und Hüllkurve:
  Nimmt das Mikrofon langsame Schwingungen auf, kann eine höhere Grenzfrequenz
  helfen, sie zu unterdrücken und feinere Hüllkurvendetails zu erhalten.
  Audio-Hochpass und Hüllkurven-Tiefpass nutzen denselben Wert; er ist
  kein Lautstärkeregler. Mit Anwenden werden die Filter neu aufgebaut.
/ Rauschunterdrückungsschwelle:
  Im Vollmodus werden Abschnitte stummgeschaltet, deren Energie dieses
  Vielfache der Referenz überschreitet. Verschwinden kräftige echte Ticks,
  versuchen Sie einen höheren Wert: *Höher schaltet weniger stumm*.
  Niedrigere Werte verstärken die Unterdrückung. Es gibt keine unabhängige
  Aus-Option; im leichten Modus wirkt der Regler nicht.
/ Halbbreite der Periodensuche:
  Gibt Spielraum für eine Uhr, deren Periode vom Nominalwert abweicht.
  Bei manueller BPH liegt die Mitte bei 7200/BPH Sekunden: Mit 14,400 BPH
  und ±20 ms wird von 480 bis 520 ms gesucht, nicht von 230 bis 270.
  Erweitern Sie den Bereich, wenn der echte Zyklus außerhalb liegt.
  Er wird auch für automatische Kandidatenprüfungen verwendet.
/ Suchhalbbreite für den Gegenschlag:
  Tack wird nahe der Mitte des gemessenen Zyklus gesucht. Mehr Spielraum
  kann bei tatsächlich ungleichen Tick/Tack-Abständen helfen, erlaubt
  aber auch ein fremdes Maximum und damit einen falschen Abfallfehler.
  Gleichmäßiger Vor- oder Nachgang ist kein Grund für eine Erweiterung.
/ Minimal akzeptierte Unruhamplitude:
  Für ein tatsächlich amplitudenarmes Uhrwerk können Sie den Wert senken.
  Tick- und Tack-Schätzung müssen ihn beide überschreiten; niedrigere Werte
  verbessern nicht die Impulserkennung. Bei Bedarf erweitert sich die
  Tick/Tack-Ansicht nach links für das gewählte Minimum; die Hervorhebungen
  der vollen Periode folgen ihr. Das verändert die Darstellung, bestätigt
  aber keine korrekte Erkennung.

=== Experteneinstellungen für die Bewegung

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parameter], [INI-Schlüssel], [Standard], [Bereich],
  [Rauschenergiefenster (ms)], [`noise_window`], [20], [10–30],
  [Länge des Rauschreferenzabschnitts (ms)], [`noise_segment`], [500], [300–700],
  [Halbbreite der Periodenverfeinerung (± ms)], [`period_refine`], [20], [10–30],
  [Mindestmaximum der kürzeren Periode (%)], [`harmonic`], [90], [80–100],
  [Prüfschwelle für Doppelauslösung (%)], [`double_trigger`], [20], [10–30],
  [Glättung der Tick/Tack-Ausrichtung (ms)], [`tic_smoothing`], [0.5], [0.1–1],
  [Verworfene hohe Samples je Phasenintervall (%)], [`trim`], [20], [10–30],
  [Halbbreite der Spurereignissuche (± ms)], [`event_search`], [20], [10–30],
  [Glättung des Hebungsimpulses (ms)], [`amp_smoothing`], [1], [0.5–2],
  [Anfangsschwelle des Hebungsimpulses (% des Maximums)], [`amp_initial`], [1], [0.1–5],
  [Hintergrundfaktor für Hebungsimpulse (×)], [`amp_noise`], [1.4], [1.2–2],
  [Höchstschwelle des Hebungsimpulses (% des Maximums)], [`amp_limit`], [20], [10–30],
  [Maximaler Tick/Tack-Amplitudenunterschied (°)], [`amp_difference`], [60], [60–100],
)

/ Rauschenergiefenster:
  So viel Audio geht in jede lokale Energiemessung ein. Ein längeres
  Fenster lässt einen kurzen Stoß über mehr Zeit wirken. Betrachten Sie
  es zusammen mit der Rauschschwelle, wenn Sie stummgeschaltete Abschnitte
  untersuchen; es arbeitet nur im Vollmodus.
/ Länge des Rauschreferenzabschnitts:
  Die Referenz ist keine mittlere Mikrofonlautstärke: Jeder Abschnitt
  liefert seine höchste lokale Energie, danach wird der Median dieser
  Maxima genommen. Längere Abschnitte liefern weniger Referenzmessungen
  je Block. Wie die übrigen Rauschregler wirkt dieser nicht im leichten Modus.
/ Halbbreite der Periodenverfeinerung:
  Nach dem Finden der Periode sucht das Programm spätere Korrelationsmaxima
  nahe ihren erwarteten Positionen, um die Messung zu verfeinern.
  Dieser Spielraum gehört zu jener zweiten Suche. Erweitern hilft, wenn
  das richtige Maximum außerhalb liegt; sonst kann ein benachbartes gewählt werden.
/ Mindestmaximum der kürzeren Periode:
  Macht die Wahl einer kürzeren automatischen Periode strenger oder
  großzügiger. Ihr Korrelationsmaximum muss diesen Anteil des ursprünglichen
  Maximums überschreiten. Ein höherer Prozentsatz lässt weniger kurze
  Kandidaten zu. Verglichen werden Korrelationsmaxima, keine Mikrofonlautstärken.
/ Prüfschwelle für Doppelauslösung:
  Hilft zu entscheiden, ob eine doppelte Periode geprüft werden soll.
  Die Korrelation nahe der 1.5-fachen Kandidatenperiode wird mit der
  Korrelation bei dieser Periode verglichen. Liegt das Verhältnis unter
  der Schwelle, wird die doppelte Periode geprüft. Erhöhen macht diese
  Prüfung wahrscheinlicher, setzt aber keine zweite Lautstärkeschwelle.
  Dieser und der vorige Regler wirken auch auf alternative Schätzungen
  im manuellen Modus, nicht auf dessen Hauptsuche um die gewählte BPH.
/ Glättung der Tick/Tack-Ausrichtung:
  Vor der Wahl der Zeitreferenz wird die kombinierte Tick/Tack-Hüllkurve
  geglättet. Stärkere Glättung entfernt feine Details, kann aber den
  Ausrichtungspunkt verschieben. Ein saubereres Bild allein ist kein
  Grund, sie zu erhöhen.
/ Verworfene hohe Samples je Phasenintervall:
  Für die repräsentative Signalform werden vor der Mittelung die höchsten
  Samples jeder Position entfernt. Höhere Prozentsätze reduzieren einzelne
  Stöße, geben aber auch ungewöhnlich lauten Schlägen weniger Gewicht.
  Die verworfene Anzahl wird aufgerundet; selbst bei wenigen Samples
  bleibt mindestens eines erhalten.
/ Halbbreite der Spurereignissuche:
  Fehlen Punkte, kann mehr Spielraum um jeden erwarteten Schlag sie
  wiederfinden. Dabei kann auch ein anderes akustisches Merkmal desselben
  Ticks gewählt werden. Dieser Regler betrifft einzelne Diagrammpunkte,
  nicht die anfängliche Periodensuche.
/ Glättung des Hebungsimpulses:
  Der Detektor sucht Impulse in einer geglätteten Signalform. Mehr
  Glättung kann Welligkeit entfernen, aber nahe Merkmale auch zusammenführen
  und ihre Erkennung erschweren.
/ Anfangsschwelle des Hebungsimpulses:
  Der Ausgangspunkt der Suche ist dieser Anteil des stärksten geglätteten
  Signalformmaximums. Er wird nicht immer verwendet: Liefert die
  Hintergrundberechnung einen höheren Wert, beginnt die Suche dort.
/ Hintergrundfaktor für Hebungsimpulse:
  Der geschätzte Hintergrund wird mit diesem Wert zur Erkennungsschwelle
  multipliziert. Erhöhen erschwert die Annahme schwacher Merkmale als
  Hebungsimpulse; das aufgenommene Signal wird dadurch nicht stärker.
/ Höchstschwelle des Hebungsimpulses:
  Die Suche versucht steigende Schwellen bis zu diesem Anteil des Maximums.
  Eine höhere Grenze erlaubt Versuche mit höheren Schwellen, keine höhere
  maximale Amplitude. Erreicht die Anfangsschwelle bereits die Grenze,
  werden keine Versuche mit niedrigerer Schwelle durchgeführt.
/ Maximaler Tick/Tack-Amplitudenunterschied:
  Für die Annahme des Paars muss der Unterschied seiner Amplituden kleiner
  als diese Grenze sein. Erhöhen erlaubt mehr Abweichung, macht die
  Schätzungen aber nicht genauer. Auch ein akzeptiertes Paar kann aus
  falsch erkannten Impulsen stammen.

== Jeweils eine Änderung ausprobieren

1. Beginnen Sie mit Standardwerten, richtiger BPH und richtigem Hebungswinkel
  sowie sauberem Audio. Erst den Mikrofonkontakt verbessern, dann die
  Erkennung lockern. Eine Aufnahme zum Vergleich speichern.
2. Die fehlerhafte Stufe bestimmen und einen passenden Regler ändern.
  Anwenden, Blöcke füllen lassen und stabile Werte vergleichen—nicht nur
  die erste plausible Zahl.
3. Nur hilfreiche Änderungen behalten; sonst zurücksetzen und erneut anwenden.
  Kalibrierungsresets benötigen das aktivierte Expertenkästchen.

Für reine Gangmessungen bleiben Hebungsimpulsregler auf Standard. Bei
tatsächlich geringer Amplitude zuerst das fortgeschrittene Minimum versuchen.
Ein akzeptiertes Ergebnis ist noch kein Beweis für Genauigkeit.
