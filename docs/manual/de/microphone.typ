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
#shot("microphone-setup.jpg", [Ein Piezo-Mikrofon in einem Gehäuse zur
Uhrmessung mit USB-Schnittstelle.], width: 60%)
== Verstärkung
Die Verstärkung erhöhen, bis die Schläge in den Signalform-Anzeigen deutlich
sichtbar sind und die Pegelanzeige des Eingangs weit über dem Rauschen
ausschlägt, ohne 0 dBFS zu erreichen. Zu hohe Verstärkung verstärkt auch die
Raumgeräusche und kann die Schläge übersteuern.
Audio-"Verbesserungen" des Betriebssystems abschalten: Rauschunterdrückung,
automatische Verstärkungsregelung und Echounterdrückung verändern das Signal
und stören die Messung.
== Live-Abhören
Mit #ui[LIVE abhören] hören Sie das aktuelle Mikrofonsignal über den
ausgewählten Ausgang. Verwenden Sie Kopfhörer: tg-expert kann nicht erkennen,
ob sie physisch angeschlossen sind, und Lautsprecher können eine akustische
Rückkopplung ins Mikrofon verursachen. Das Abhören folgt dem Live-Eingang auch
bei ausgewähltem Snapshot-Tab; Snapshot- oder historische Audiodaten werden
nicht abgespielt.
Wählen Sie den Ausgang und stellen Sie die unabhängige Abhörlautstärke unter
#ui[Audioeinstellungen] ein. Die Lautstärke ist vorsichtig begrenzt und ändert
weder Eingangsverstärkung noch das Messsignal. Fällt der ausgewählte Ausgang
aus, wird das Abhören beendet und muss nach Auswahl eines verfügbaren Ausgangs
erneut aktiviert werden. Manche Systeme schalten dieselbe physische Buchse
zwischen Kopfhörern und Lautsprechern um, ohne die Gerätekennung zu ändern;
das kann das Programm nicht erkennen.
Achten Sie auf die Uhrticks im Verhältnis zum Raumgeräusch, probieren Sie
andere Positionen und Anpressdrücke aus und hören Sie auf Stöße oder
Kabelreiben. Mögliche Übersteuerung auch mit der Eingangspegelanzeige prüfen.
Das Abhören hilft nur beim Einrichten und diagnostiziert das Uhrwerk nicht
automatisch.
== Störgeräusche
- In einem ruhigen Raum arbeiten. Lüfter, Lautsprecher und laute Geräte in
  der Nähe ausschalten.
- Während der Messung weder den Tisch noch die Kabel berühren.
- Eine weiche Unterlage unter dem Halter isoliert die Uhr von
  Tischerschütterungen.
- Die Option #ui[Leichter Algorithmus] (unter #ui[Algorithmuskonfiguration…],
  #ui[Grundlegend]) lässt die
  Rauschunterdrückung aus. Sie sollte ausgeschaltet bleiben, außer der Computer ist zu langsam.
