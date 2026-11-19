#import "../lib.typ": *

= Das Mikrofon <microphone>
Die meisten schlechten Messwerte stammen vom Audioaufbau und nicht vom
Programm: ein schwaches oder rauschendes Mikrofon, ein Mikrofon zu weit von
der Uhr entfernt oder Raumgeräusche.
== Welches Mikrofon
Besser geeignet:
- ein Kontaktmikrofon oder Piezo-Tonabnehmer;
- ein Ohrhörer- oder Headset-Mikrofon, das gegen das Gehäuse gedrückt wird;
- ein ordentliches USB-Mikrofon, das an die Uhr gelegt wird.
Meist schlechter geeignet:
- das eingebaute Mikrofon eines Laptops;
- ein Tischmikrofon in einigem Abstand;
- Mikrofone mit aktivierter Rauschunterdrückung oder automatischer
  Verstärkung.
Ein einfaches Ohrhörer-Mikrofon, das am Gehäuse anliegt, funktioniert oft
besser als ein teures Mikrofon in einigen Zentimetern Abstand.
== Platzierung
- Das Mikrofon in Kontakt mit dem Gehäuse, dem Gehäuseboden oder dem
  Werkhalter bringen.
- Uhr und Mikrofon ruhig halten. Ein kleiner Halter oder eine Klemme hilft.
- Ist das Signal schwach, andere Stellen am Gehäuse ausprobieren. Kleine
  Änderungen der Position können einen großen Unterschied machen.
- Beim Messen der Lagen (siehe @measuring) das Signal nach dem Drehen der Uhr
  erneut prüfen.
#shot("microphone-setup.jpg", [Das Inline-Mikrofon eines Ohrhörerkabels,
direkt auf das Uhrglas gelegt.], width: 60%)
== Verstärkung
Die Verstärkung erhöhen, bis die Schläge in den Signalform-Anzeigen deutlich
sichtbar sind und die Pegelanzeige des Eingangs weit über dem Rauschen
ausschlägt, ohne 0 dBFS zu erreichen. Zu hohe Verstärkung verstärkt auch die
Raumgeräusche und kann die Schläge übersteuern.
Audio-"Verbesserungen" des Betriebssystems abschalten: Rauschunterdrückung,
automatische Verstärkungsregelung und Echounterdrückung verändern das Signal
und stören die Messung.
== Störgeräusche
- In einem ruhigen Raum arbeiten. Lüfter, Lautsprecher und laute Geräte in
  der Nähe ausschalten.
- Während der Messung weder den Tisch noch die Kabel berühren.
- Eine weiche Unterlage unter dem Halter isoliert die Uhr von
  Tischerschütterungen.
- Die Option #ui[Leichter Algorithmus] (im Menü) lässt die
  Rauschunterdrückung aus. Sie sollte ausgeschaltet bleiben, außer der Computer ist zu langsam.
