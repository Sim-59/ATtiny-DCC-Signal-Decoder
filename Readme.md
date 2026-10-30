# DCC-Signaldekoder mit ATtiny85 

## Hardware
Aufbauend auf eine kleine [Zusatzplatine](http://simandit.de/simwiki/doku.php?id=modellbahn:umbauten:dcc-dekoder#zusatzplatine_4-port-variante) zum ATtiny85-Digispark-Board wurde Die aktuelle [Leiterplattenvariante](https://simandit.de/simwiki/doku.php?id=modellbahn:umbauten:dcc-dekoder#multi-funktions-dekoder_4-port) für den DCC-Dekoder enthält auch den ATtiny85 und dessen Spannungsversorgung.\
Mit Transistoren für die Funktionsausgänge wird die gleichgerichtete Gleisspannung geschaltet. Zur Erzeugung des ACK-Signals ist ein Anschluss vorgesehen, der vorübergehend zum Lesens der CVs durch einen Schalter oder Jumper überbrückt werden kann. CV-Schreiben ist auch ohne diesen möglich.

Diese Variante muss über ISP mit einem separaten Programmer programmiert werden.\
Eine gute Informationsquelle zur Programmierung des ATtiny85 ist [Wolles Elektronikkiste](https://wolles-elektronikkiste.de/attiny-mit-arduino-code-programmieren). 

## Software
Der Arduino-Sketch verwendet die [NmraDcc-Bibliothek](https://github.com/mrrwa/NmraDcc) von [MRRWA](http://mrrwa.org/), die über die Arduino-Bibliotheksverwaltung eingebunden werden kann.
Es wird das erweiterte DCC-Paket-Format für Zubehör - Extended Accessory Decoder Control Packet Format - verwendet. Damit sind in den meisten Systemen, z.B. DCC-Ex, 32 Signalbegriffe möglich.
Der Dekoder nutzt eine DCC-Zubehöradresse im Output-Address-Mode. 

Folgende HL-Signal-Begriffe sind mit den 4 LEDs (statisch und blinkend) für eine Nebenbahn-Strecke möglich:
- Hp0  - Halt (Hl13)
- Hl1  - Fahrt mit Streckenhöchstgeschwindigkeit
- Hl3a - Fahrt mit 40 km/h, dann mit Streckenhöchstgeschwindigkeit
- Hl7  - Geschwindigkeit auf 40 km/h ermäßigen (nur bei Vorsignalfunktion)
- Hl9a - Fahrt mit 40 km/h, Fahrt mit 40 km/h erwarten
- Hl10 - Halt erwarten
- Hl12a - Fahrt mit 40 km/h, Halt erwarten

Bei EZMG-Ausfahrsignalen sind die Signalbilder meist reduziert ausgeführt, nur Hp0 (Hl13), Hl1 bzw. Hl3a bei abzweigender Weiche werden angezeigt, das gelbe obere Licht ist abgedeckt. Dieser Ausgang kann dann bei der Verdrahtung des Signals für 2x Weiß für das Rangiersignal Ra12 genutzt werden. Dieses Signalbild ist als Option bei entsprechender Verdrahtung auch im Sketch umgesetzt.  
- Hp0 + Ra12 - Rangierfahrt erlaubt

Die Helligkeitseinstellung der Lichtquellen an 3 Ports ist mit PWM-Dimmen des ATtiny realisiert, bei PB3 wird das PWM in der Loop-Schleife realisiert.
Für das langsame Ein-/Ausschalten (Fade-In und Fade-Out) der Lichtquellen sind weitere Konfigurationsvariablen definiert.

