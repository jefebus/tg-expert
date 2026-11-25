#import "../lib.typ": *

= Erster Start
Beim Start öffnet tg-expert das Standardeingabegerät des Standard-Audiosystems.
Lässt sich kein Eingang öffnen, erscheint die Meldung #ui[Es konnte kein
Audioeingang geöffnet werden.] mit einer Schaltfläche zu den
Audioeinstellungen.
== Der Dialog Audioeinstellungen
Er wird über #ui[Audioeinstellungen…] im Menü #ui[Einstellungen] geöffnet oder über die Audio-Schaltfläche in der
oberen Leiste, die den aktuell verwendeten Eingang anzeigt.
#shot("audio-settings.png", [Der Dialog Audioeinstellungen. Die Zeile nahe dem
unteren Rand zeigt den verwendeten Eingang.], width: 55%)
/ Audiosystem: das Soundsystem für Aufnahme und Live-Abhören. Unter Windows
  sind das MME, DirectSound, WASAPI und WDM-KS. Unter Linux ALSA und, falls
  es läuft, JACK. Bei einem Wechsel des Audiosystems werden dessen
  Standardeingang und Standardrate gewählt; die Ausgabegeräteliste folgt
  ebenfalls diesem System.
/ Eingabegerät: das Mikrofon oder der Line-Eingang, von dem aufgenommen wird.
/ Ausgabegerät: das Gerät für Live-Abhören. Es werden nur Geräte des
  ausgewählten Audiosystems angezeigt.
/ Abtastrate: die Rate, mit der der Eingang aufgenommen wird. Aufgelistet
  werden nur die Raten, die das Gerät akzeptiert, aus 22.05, 32, 44.1 und
  48 kHz. 44.1 kHz ist ein guter Standardwert. Eine höhere Rate ist nicht
  immer besser, besonders bei einem rauschenden Mikrofon: Verwenden Sie die
  niedrigste Rate, die einen stabilen Messwert liefert.
/ Eingangspegel: eine Anzeige für den Pegel des eingehenden Signals in dBFS,
  die bei geöffnetem Dialog aktualisiert wird.
Änderungen wirken sofort: Der Audioeingang wird neu gestartet und die Messung
beginnt von vorn. Die Auswahl wird gespeichert und beim nächsten Start
verwendet.
Die Audioeinstellungen lassen sich während der Kalibrierung oder bei
gewähltem Aufnahme-Reiter nicht ändern. Der Dialog weist in diesem Fall
darauf hin.
== Eingangspegel prüfen
Die Uhr an das Mikrofon halten und die Pegelanzeige beobachten.
- Bewegt sich die Anzeige kaum, ist das falsche Gerät gewählt oder die
  Verstärkung zu niedrig. Die Mikrofonverstärkung in den Audioeinstellungen
  des Betriebssystems erhöhen.
- Bleibt die Anzeige nahe 0 dBFS, ist der Eingang übersteuert. Die
  Verstärkung verringern.
- Ein Klopfen auf das Mikrofon sollte die Anzeige ausschlagen lassen. Tut es
  das nicht, hört das Programm auf ein anderes Gerät.
Die Statusanzeige neben der Uhr in der Ergebnistabelle zeigt, ob das Programm
einen Schlag erkennt. Siehe @main-window.
#note[Kalibrieren Sie die Soundkarte, bevor Sie der Gangabweichung trauen (siehe @calibration). Der Wert cal bleibt zwischen Sitzungen gespeichert. Nach einem Wechsel des Audioeingangs oder der Abtastrate muss neu kalibriert werden; weitere Fälle stehen unter @calibration.]
