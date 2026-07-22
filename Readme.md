# DCC-Signaldekoder mit ATtiny85 

## Hardware
Zum ATtiny85-Digispark-Board wurde zu Beginn eine kleine [Zusatzplatine](http://simandit.de/simwiki/doku.php?id=modellbahn:umbauten:dcc-dekoder#zusatzplatine_4-port-variante) entwickelt, mit der die Anschaltung an das Gleissignal erfolgt. Mit Transistoren für die Funktionsausgänge wird die gleichgerichtete Gleisspannung geschaltet. Zur Erzeugung des ACK-Signals ist ein Anschluss vorgesehen, der während des Lesens der CVs verbunden werden kann. Schreiben ist auch ohne diesen möglich.\
Diese Platinen-Kombination kann mit dem enthaltenen Bootloader über USB programmiert werden. Wegen der Bootzeit von 300 ms (nach Upgrade) bis 5 s (default), die nach jedem Power-On entsteht, nutze ich nur noch die ISP-Programmierung ohne Bootloader.

Eine zweite [Leiterplattenvariante](https://simandit.de/simwiki/doku.php?id=modellbahn:umbauten:dcc-dekoder#multi-funktions-dekoder_4-port)) für den DCC-Dekoder enthält auch den ATtiny85 und dessen Spannungsversorgung. Diese Variante muss über ISP mit einem separaten Programmer programmiert werden.

Eine gute Informationsquelle zur Programmierung des ATtiny85 ist [Wolles Elektronikkiste](https://wolles-elektronikkiste.de/attiny-mit-arduino-code-programmieren). 

## Software
Der Arduino-Sketch verwendet die [NmraDcc-Bibliothek](https://github.com/mrrwa/NmraDcc) von [MRRWA](http://mrrwa.org/), die über die Arduino-Bibliotheksverwaltung eingebunden werden kann.

Es wird das erweiterte DCC-Paket-Format für Zubehör - Extended Accessory Decoder Control Packet Format - verwendet. Damit sind in den meisten Systemen, z.B. DCC-Ex, 32 Signalbegriffe möglich.

Folgende HL-Signal-Begriffe sind mit den 4 LEDs (statisch und blinkend) für eine Nebenbahn-Strecke möglich:
- Hp0  - Halt (Hl13)
- Hl1  - Fahrt mit Streckenhöchstgeschwindigkeit
- Hl3a - Fahrt mit 40 km/h, dann mit Streckenhöchstgeschwindigkeit
- Hl7  - Geschwindigkeit auf 40 km/h ermäßigen
- Hl9a - Fahrt mit 40 km/h, Fahrt mit 40 km/h erwarten
- Hl10 - Halt erwarten
- Hl12a - Fahrt mit 40 km/h, Halt erwarten

Bei EZMG-Ausfahrsignalen sind die Signalbilder meist reduziert ausgeführt, nur Hp0 (Hl13), Hl1 bzw. Hl3a bei abzweigender Weiche werden angezeigt, das gelbe obere Licht ist abgedeckt. Dieser Ausgang kann dann bei der Verdrahtung des Signals für 2x Weiß für das Rangiersignal Ra12 genutzt werden. Dieses Signalbild ist als Option bei entsprechender Verdrahtung auch im Sketch umgesetzt.  
- Hp0 + Ra12 - Rangierfahrt erlaubt

Der Dekoder nutzt eine DCC-Zubehöradresse und den Output-Address-Mode. Es sind folgenden Konfigurationsvariablen (CVs) sind vorhanden:  
- CV1 = 8 bit LSB, default 1 
- CV9 = 3 Bit MSB, default 0
  - Dekoderadresse = LSB + MSB*256
- CV7 Versionsnummer, im Sketch eingestellt
- CV8 Hersteller-ID, entsprechend nmra-Bibliothek 13 für DIY
  - Schreiben auf CV8 führt einen Decoder Reset mit Default-Werten aus
- CV29 Configuration, default 192
  - Bit6=1 = Output Address Mode
  - Bit7=1 = Accessory Decoder Mode
- CV34 Blinking periode - default 4 for 1 sec blink frequency (4 bit for blinking periode in s (0.25 ... 3.75 s))

In der Variante mit SoftDim wird die Helligkeit von 3 Ports mit PWM des ATtiny realisiert, bei PB3 wird das PWM in der Loop-Schleife realisiert. Für die Dim-Werte gibt es vier CVs mit 5 Bit (31 für maximale Helligkeit):
- CV51, CV52, CV53, CV54 - Dimmwert Rot, Grün, Gelb oben, Gelb unten

Die default-Einstellung ist 15

