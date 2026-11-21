#import "../lib.typ": *

= Fehlersuche <troubleshooting>
== Kein Eingang
*Beim Start erscheint die Meldung "Es konnte kein Audioeingang geöffnet
werden."*
- Die Audioeinstellungen öffnen und ein anderes Audiosystem oder
  Eingabegerät wählen.
- Prüfen, ob das Betriebssystem das Mikrofon erkennt und ob es nicht
  stummgeschaltet oder deaktiviert ist.
- Prüfen, dass kein anderes Programm den Eingang exklusiv belegt.
*Die Pegelanzeige in den Audioeinstellungen bewegt sich nicht.*
- Es ist das falsche Gerät gewählt, oder seine Verstärkung steht auf null. Auf
  das Mikrofon klopfen und prüfen, ob die Anzeige reagiert.
*Windows: Mikrofonzugriff.* Windows 10 und 11 können Desktop-Programmen den
Zugriff auf das Mikrofon sperren. *Einstellungen > Datenschutz und Sicherheit
> Mikrofon* öffnen und sowohl *Mikrofonzugriff* als auch *Zulassen, dass
Desktop-Apps auf Ihr Mikrofon zugreifen* einschalten. Danach tg-expert neu
starten.
*Windows: WDM-KS-Eingänge.* Das Audiosystem WDM-KS listet viele Eingänge auf,
von denen sich einige nicht öffnen lassen oder sich öffnen, aber Stille
liefern. Schlägt ein Eingang unter WDM-KS fehl, stattdessen dasselbe Gerät
unter WASAPI oder MME wählen. MME ist die kompatibelste Wahl.
*Windows: "Verbesserungen".* Einige Treiber wenden auf das Mikrofon
Rauschunterdrückung oder automatische Verstärkung an. Diese in den
Geräteeigenschaften in den Windows-Soundeinstellungen ausschalten.
*Linux: Gerät belegt.* Läuft JACK oder PipeWire-JACK, können ALSA-Geräte
belegt sein. JACK als Audiosystem wählen oder JACK beenden.
*macOS: kein Eingang.* Den Mikrofonzugriff unter *Systemeinstellungen >
Datenschutz & Sicherheit > Mikrofon* für tg-expert oder für das Terminal, aus
dem es gestartet wurde, erlauben.
== Falsche bph
*Die Spalte BEAT zeigt einen mit \~ markierten Wert, oder die
Gangabweichung ist absurd (Hunderte von s/t).*
- Mit #ui[Schätzung] kann das Programm bei schwachem Signal auf eine falsche
  Schlagzahl einrasten. Ist die bph bekannt, sollte sie von Hand eingestellt
  werden.
- Ist die bph von Hand eingestellt, mit den Werkdaten vergleichen. 21600 und
  28800 sind die häufigsten Werte. Ein Werk mit 18000 bph, das auf 21600
  eingestellt ist, zeigt eine Gangabweichung von Tausenden s/t.
- Das Statussymbol zeigt *!*, wenn bei der gewählten bph kein gültiger
  Rhythmus vorliegt.
== Instabile Messwerte
*Die Gangabweichung springt, oder das Gangdiagramm ist verstreut.*
- Zuerst das Signal verbessern: das Mikrofon in Kontakt mit dem Gehäuse
  bringen, Raumgeräusche verringern und die Verstärkung anpassen. Siehe
  @microphone.
- Die Signalform-Anzeigen prüfen. Scharfe, getrennte Klicks bedeuten ein gutes
  Signal.
- Die Option #ui[Leichter Algorithmus] unter #ui[Algorithmuskonfiguration…] ausschalten, falls sie eingeschaltet ist.
- Die Uhr und den Tisch während der Messung nicht berühren.
- Etwas Instabilität liegt an der Uhr selbst. Ein verschlissenes oder
  verschmutztes Werk schwankt von Schlag zu Schlag. Das σ auf der Seite
  #ui[Verlauf] zeigt, wie stark.
*Die Amplitude fehlt oder ist unplausibel.*
- Den Hebungswinkel prüfen.
- Für die Amplitude muss die akustische Hebungszeit zuverlässig messbar sein: Der Beginn der Auslösung und die starke Schlussreferenz aus Sperrung/Anschlag müssen erkennbar sein. Ein schwaches oder verrauschtes Signal kann eine stabile Gangabweichung, aber keine zuverlässige Amplitude liefern.
*Die Gangabweichung weicht durchgehend vom tatsächlichen Gang der Uhr ab.*
- Die Soundkarte kalibrieren (siehe @calibration) und nach jeder Änderung des
  Audioeingangs oder der Abtastrate erneut kalibrieren.
