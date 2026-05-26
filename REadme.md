# DCC-Signaldekoder mit ATtiny85 
Der Sketch verwendet die [NmraDcc-Bibliothek](https://github.com/mrrwa/NmraDcc) von [MRRWA](http://mrrwa.org/), die über die Arduino-Bibliotheksverwaltung eingebunden werden kann.

Es wird das erweiterte DCC-Paket-Format für Zubehör - Extended Accessory Decoder Control Packet Format - verwendet. Damit sind in den meisten Systemen, z.B. DCC.Ex, 32 Signalbegriffe möglich.
Zum Digispark-Board wurde einerseits eine kleine [Zusatzplatine](http://simandit.de/simwiki/doku.php?id=modellbahn:umbauten:dcc-dekoder#funktions-dekoder_mit_digispark-board) entwickelt, mit der die Anschaltung an das Gleissignal erfolgt. Mit Transistoren für die Funktionsausgänge wird die gleichgerichtete Gleisspannung geschaltet. Zur Erzeugung des ACK-Signals ist ein Anschluss vorgesehen, der während des Lesens der CVs verbunden werden kann. Bei einer zweiten Leiterplattenvariante für den DCC-Dekoder enthält auch den ATtiny85 und verwendet die ISP-Programmierung mit einem separaten Programmer.

Folgende Signal-Begriffe für eine Nebenbahn-Strecke sind möglich:
- Hp0  - Halt
- Hl1  - Fahrt mit Streckenhöchstgeschwindigkeit
- Hl3a - Fahrt mit 40 km/h, dann mit Streckenhöchstgeschwindigkeit
- Hl7  - Höchstgeschwindigkeit auf 40 km/h ermäßigen
- Hl9a - Fahrt mit 40 km/h, Fahrt mit 40 km/h erwarten
- Hl10 - Halt erwarten
- Hl12a - Fahrt mit 40 km/h, Halt erwarten

Es sind folgenden Konfigurationsvariablen (CVs) sind vorhanden:  
- CV1 = 6 bit LSB, default 1 
- CV7 Versionsnummer, im Sketch eingestellt
- CV8 Hersteller-ID, entsprechend nmra-Bibliothek 13 für DIY
  - Schreiben auf CV8 führt einen Decoder Reset mit Default-Werten aus
- CV9 = 3 Bit MSB, default 0
- CV29 Configuration, default 192
  - Bit6=1 = Output Address Mode
  - Bit7=1 = Accessory Decoder Mode
- CV34 Blinking periode - default 4 for 1 sec blink frequency (4 bit for blinking periode in s (0.25 ... 3.75 s))
