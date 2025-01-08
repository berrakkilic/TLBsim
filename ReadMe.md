Cache Simulation und Analyse (A14)
==================================
Projektaufgabe – Aufgabenbereich Translation Lookaside Buffer (TLB)
------------------------------------------------------------------
### Aufgabenstellung
Dieses Projekt simuliert und analysiert die Leistung von Translation Lookaside Buffers (TLBs) innerhalb eines Computersystems. 
Ziel der Simulation ist es, ihre Auswirkungen auf die Latenz beim Speicherzugriff zu verstehen.

### Hauptziel
- Den Übersetzungsprozess von virtuellen in physische Adressdaten mithilfe von TLBs beschleunigen:
  * Messung von Hits, Misses und Zyklenzahlen.
  * Vergleich der gesammelten Daten mit und ohne TLB-Verwendung.
  → Erwartung: TLBs beschleunigen den Übersetzungsprozess und reduzieren die Latenz beim Speicherzugriff.
    * Simulationsergebnisse:
      - Ohne TLB: Cycles = 300 (Memory Latency) * 250 (Anzahl Inputs) = 75.000 Zyklen
      - Mit TLB: Cycles = 80000; BlockSize = 16; vsb_offset = 0; TLB-Latency = 1; Memory-Latency = 300;
        - FALL 1 → TLB_SIZE = 64; → 45 Hits/250 Requests = 18,0 %   
        - FALL 2 → TLB_SIZE = 128; → 54 Hits/250 Requests = 21,6%                 
        - FALL 3 → TLB_SIZE = 256; → 70 Hits/250 Requests = 28,0%      
        - FALL 4 → TLB_SIZE = 512; → 81 Hits/250 Requests = 32,4%

### Nebenziel
- Den Kompromiss zwischen Komplexität/Kosten und Leistung verstehen:
  * Bestimmung der erforderlichen Gatter und Zyklen.
  → Erwartung: Anzahl der Gatter und Performanz steigen mit der Anzahl der TLBs.
  * Simulationsergebnisse:
    - Fall 1: 13760 Gatter 
    - Fall 2: 21440 Gatter (55,8 % mehr als Fall 1) (20,0 % effizienter als Fall 1)
    - Fall 3: 35520 Gatter (65,8 % mehr als Fall 2) (29,6 % effizienter als Fall 2)
    - Fall 4: 61120 Gatter (72,1 % mehr als Fall 3) (15,7 % effizienter als Fall 3)

--------------------------------------------------------------------------
### Projektstruktur
- Modules.hpp: Definition der Module (TLB, CPU, Memory).
- Structs.h: Definition der Strukturen (Result, Request).
- Simulation.cpp: Simulation der TLB-Verwendung (run_simulation()-Funktion).
- Main.c: Hauptprogramm (main()-Funktion).
- Makefile: Kompilierungsautomatisierung (make und make clean Befehle).

### Kompilierung
- Kompilierung: make
- Ausführung: ./src/simulation --example requests.csv (Beispiel)
- Bereinigung: make clean

### Simulation
- Die Hauptmethode erhält Daten aus CSV-Dateien mit R/W-Operationen, Adressen und Daten (W).
- Prüfung, ob die virtuelle Adresse im TLB vorhanden ist. 
- Wenn gefunden, wird die physische Adresse aus dem TLB ausgelesen und verarbeitet.
- Bei Nichtvorhandensein wird die physische Adresse berechnet und der TLB aktualisiert.
- Lesevorgänge lesen Daten aus dem Hauptspeicher, Schreibvorgänge schreiben Daten in den Hauptspeicher.
- Die Gültigkeit des Tags 0 hängt von der Art des TLB ab. In unserer Simulation wird der Tag 0 als ein Versuch interpretiert, auf die Festplatte (DISK) zuzugreifen, was einen Timeout-Fehler verursacht.

### Ausgabe
- Die Ausgabe erfolgt in der Konsole und enthält die Anzahl der Hits, Misses und Zyklen.

----------------------------------------------------------------------------
### Autoren
- Bilge Abalı - Memory-Modul, main.c
- Berrak Kılıç - run_simulation()-Funktion, ReadMe.md, Gatter-Berechnung
- Emre Özbek - TLB-Modul, CPU-Modul, Makefile, main.c, CSV-Dateien
