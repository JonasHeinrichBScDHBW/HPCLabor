# README

# Aufgabe 3 - Performanceanalyse

## 3a

*Berechnen Sie, wie viel Speicherplatz im Arbeitsspeicher und auf der Festplatte für den Gebietsgrößentest benötigt wird.*

- h: Segmenthöhe
- w: Segmentbreite

- Arbeitsspeicher: h * w * sizeof(char) (mindestens)
- Festplatte: h * w * sizeof(float) + sizeof(long) + xml_overhead * sizeof(char)

*Vergleichen Sie ihre berechneten Werte des Festplattenplatzes mit den tatsächlichen Größen der Outputdateien.*

- Segmentgöße: 8x15
- Arbeitsspeicher:       8 * 15
- Festplatte:            898 = 8 * 15 * 4 + 8 + 410 * 1
- Festplatte (gemessen): 898 (`ls -lah output/gol_mtp_00001_0_0.vti`)

Der berechnete und gemessene Wert auf der Festplatte ist gleich. Die Konstante `410` wurde im Code Editor durch
Selektion des XML-Tags exclusive `append-data` gemessen (analog aus C-Code zu berechnen). 

*Wie ist das Verhältnis der benötigten Größe auf dem Arbeitsspeicher zu der Output-Dateigröße? Notieren Sie Ihre Beobachtungen in kurzer Form.*

- Beispiel: 898 / 130 = 6.9
- Relativ (ohne bias): 520 / 130 = 4
    - Umwandeln von char zu float
