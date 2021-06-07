## Make

...

## Run

run with 

````bash
sudo LD_LIBRARY_PATH=/home/dietpi/libroyale/bin ./unfolding-app`
````



add options:

```bash
--help      | show help
--log       | enable general log functions – currently no effect
--printLogs | print log messages in console
--mode arg  | set pico flexx camera mode (int from 0:5)
```



## Old

Kurze Übersicht:

Es existieren 3 Repos: 

- [unfolding-space-private](https://github.com/jakobkilian/unfolding-space-private)
  : (hier entwickle ich, ohne dass mir wer zusehen kann, hihi)
- [unfolding-space](https://github.com/jakobkilian/unfolding-space) :
  (hier merge ich releases rüber, wenn sie fertig für die Öffentlichkeit
  sind)
- [unfolding_monitor](unfolding-space-private) : (Unity Projekt, um über
  udp übers Netzwerk Log Daten und das Kamerabild abzufangen und
  anzuzeigen)

Zum Code in unfolding-space-private:

- Main.cpp macht nur Thread Management (unfTh). 
- Getriggert wird die Bildverarbeitung durch die Callback Funktion
  DepthDataListener::onNewData in der camera.cpp, die von der lobroyal
  library (die der Kamera) aufgerufen wird, wenn ein neuer Frame
  „fertig“ ist.
- Von da aus werden die Daten erst kopiert, damit onNewData wie
  erfordert schnell returnen kann.
- Und aus dieser Kopie wird dann in weiteren Threads das 9x9 Array für
  die Motoren generiert (ddCopyTh). 
- In void MotorBoard::sendValuesToGlove in MotorBoard.cpp wird dann das
  Schreiben der Werte auf den Mux und die Treiber angestoßen (ddSendTh).

Parallel dazu maintained der udpSendTh Thread die Verbindung zu
potentiellen Monitoring Clients:

Der Server broadcaster hier alle 0,5s eine online Nachricht ins
Netzwerk. Clients empfangen diese und melden zurück, dass sie Daten
wollen. Der Server nimmt sie in eine Liste auf und schickt immer am Ende
der Bildverarbeitung die Daten an alle raus.

Um die ganzen Daten zu collecten, die ich gerne senden würde, habe ich
mir zb. so eine merkwürdige TimeLogger Klasse gebaut und speichere
einige Werte in Globals.cpp – so gut es geht gemutext oder atomic, aber
hier fängt mein Bauchweh an...

