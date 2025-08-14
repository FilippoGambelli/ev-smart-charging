# **Modello di gestione ricarica colonnine con fotovoltaico e accumulatore**

## **1. Dati disponibili (input)**

### **1.1 Pannelli fotovoltaici**

* Tipo: monocristallino
* Efficienza: 20%
* Potenza nominale per pannello: 500 W
* Area disponibile: 900 m² → 300 pannelli installabili
* Perdita di sistema: 14%

**Calcolo potenza installata:**

$$
P_{\text{PV,totale}} = 300 \cdot 0.5 \text{ kW} \cdot (1 - 0.14) \approx 129 \text{ kW}
$$

**Produzione stimata:**

* Energia giornaliera: $E_{\text{PV,giorno}} = 129 \text{ kW} \cdot 4 \text{ h} \approx 516 \text{ kWh/giorno}$
* Energia mensile: $E_{\text{PV,mese}} \approx 15.480 \text{ kWh}$

> ✅ Nota: hai anche la **produzione istantanea** e la **produzione prevista per le ore successive** (fornita dal modello ML).

### **1.2 Accumulatore (batteria)**

* Capacità: 200 kWh
* Funzione: coprire i picchi di richiesta quando il solare non basta o di notte.

### **1.3 Colonnine di ricarica**

* Numero: 3 colonnine
* Potenza massima per colonnina: 22 kW
* Massimo 1 veicolo per colonnina
* Dati veicolo: stato di carica corrente (%) e priorità

### **1.4 Energia dalla rete elettrica**

* Prezzo energia: $c_{\text{rete}}$ €/kWh
* Uso solo se solare + batteria non coprono la domanda

---

## **2. Variabili decisionali (output)**

Per ogni colonnina $c$:

1. $p_c^{\text{PV,ora}}$ → potenza istantanea dai pannelli
2. $p_c^{\text{PV,prev}}$ → potenza da energia prevista (prossime ore)
3. $p_c^{\text{Batt}}$ → potenza erogata dall’accumulatore
4. $p_c^{\text{Rete}}$ → potenza erogata dalla rete

**Output attesi:** assegnazione di potenze a ciascuna colonnina, ottimizzando la ricarica dei veicoli.

---

## **3. Vincoli**

### **3.1 Potenza massima per colonnina**

$$
p_c^{\text{PV,ora}} + p_c^{\text{PV,prev}} + p_c^{\text{Batt}} + p_c^{\text{Rete}} \le P_c^{\max} \quad \forall c
$$

### **3.2 Energia disponibile dai pannelli**

$$
\sum_{c} p_c^{\text{PV,ora}} \le E_{\text{PV,ora}}
$$

$$
\sum_{c} p_c^{\text{PV,prev}} \le E_{\text{PV,prev}}
$$

> Qui si tiene conto sia dell’energia **istantanea** che di quella **prevista** dal modello ML.

### **3.3 Energia disponibile dall’accumulatore**

$$
\sum_{c} p_c^{\text{Batt}} \le E_{\text{Batt,disp}}
$$

### **3.4 Stato di carica dei veicoli**

Per ogni veicolo $v$ sulla colonnina $c$:

$$
S_v^{\text{fine}} = S_v + \frac{p_c^{\text{PV,ora}} + p_c^{\text{PV,prev}} + p_c^{\text{Batt}} + p_c^{\text{Rete}}}{E_{\text{batt},v}} \cdot 100
$$

* $S_v$ = stato di carica corrente (%)
* $E_{\text{batt},v}$ = capacità batteria del veicolo (kWh)

---

## **4. Funzione obiettivo**

$$
\text{Max } Z = \sum_v w_v \cdot S_v^{\text{fine}} - \sum_c c_{\text{rete}} \cdot p_c^{\text{Rete}}
$$

* $w_v$ = priorità del veicolo $v$
* Massimizza la ricarica dei veicoli **ad alta priorità**
* Minimizza l’uso della **rete elettrica**

---

## **5. Approccio operativo**

1. Risolvi il modello ogni **15 minuti** o ogni volta che un veicolo si collega/scollega:

   * Aggiorna $E_{\text{PV,ora}}$, $E_{\text{PV,prev}}$, energia batteria residua, stato di carica dei veicoli.

2. Ottieni potenze ottimali da assegnare a ciascuna colonnina ($p_c^{\text{PV,ora}}, p_c^{\text{PV,prev}}, p_c^{\text{Batt}}, p_c^{\text{Rete}}$)

3. Applica la ricarica per l’intervallo di 15 minuti

4. Aggiorna stato di carica dei veicoli e batteria, poi ripeti

---

✅ **Vantaggi del modello:**

* Sfrutta **PV istantaneo e previsto**
* Minimizza consumo rete e costi energetici
* Rispetta **priorità veicoli**
* Tiene conto di **potenza massima colonnine** e **capacità batteria**

>LA MACCHINA DEVE DIRE ALLA COLONNINA QUANTA POTENZA MASSIMA È IN GRADO DI SFRUTTARE

>LA MAGGIOR PARTE DELLE CONOLLINE SONO TRIFASE (22kW) MA CI SONO DELLE AUTO CHE POSSONO CARICARE SOLAMENTE MONOFASE QUINDI MASSIMO (7kW)

---

Breve risposta: **sì — nel modello iniziale avevi citato una variabile di produzione prevista (PV\_prev) ma non era stata integrata in modo esplicito nell’ottimizzazione multi-step**.
Ho aggiornato e chiarito il modello per usare **esplicitamente la produzione prevista** su un orizzonte predittivo $H$ (es. 4 ore, step 15 min). Di seguito trovi la versione corretta, compatta e pronta per essere implementata (formulazione LP/rolling horizon).

---

# 1) Idea chiave

Usiamo un orizzonte predittivo $t=0,\dots,H-1$ (ogni passo $\Delta t$ ore). Per ogni passo $t$ usiamo la **produzione PV disponibile prevista** $P_{\text{PV},t}^{\text{avail}}$:

* per $t=0$: è la produzione **istantanea misurata**;
* per $t\ge1$: sono le **previsioni** fornite dal modello ML.

Attribuiamo la produzione PV condivisa ai carichi (colonnine), alla carica della batteria o la lasciamo curtailed. Tutto è vincolato dalla **potenza PV totale prevista** in ogni intervallo.

---

# 2) Indici, parametri (rappel)

* $c\in\{1,\dots,C\}$ colonnine (qui $C=3$).
* $v$ veicolo su colonnina $c$.
* $t=0,\dots,H-1$ passi sull’orizzonte (es. $H=16$ per 4 ore a $\Delta t=0.25$ h).
* Parametri: $P_c^{\max}, E_{\text{batt}}^{\text{cap}}, P_{\text{batt}}^{\max}, \eta_{ch},\eta_{dis}, c_{\text{rete}}, w_v, E_v^{cap}, S_{v,0}$.
* **Forecast PV**: $P_{\text{PV},t}^{\text{avail}}$ per ogni $t$ (misura per $t=0$, previsione ML per $t\ge1$).

---

# 3) Variabili decisionali (su orizzonte)

Per ogni $t$ e ogni colonnina $c$:

* $p_{c,t}^{PV}\ge0$ — potenza PV assegnata a $c$ (kW).
* $p_{c,t}^{Batt}\ge0$ — potenza dalla batteria a $c$ (kW).
* $p_{c,t}^{Rete}\ge0$ — potenza rete a $c$ (kW).

Variabili sistema (per ogni $t$):

* $p_{t}^{batt,ch}\ge0$ — potenza con cui la PV (o rete) carica l’accumulatore (kW).
* $p_{t}^{batt,dis}\ge0$ — potenza totale scaricata dalla batteria (kW) (uguale a $\sum_c p_{c,t}^{Batt}$).
* $p_{t}^{PV,curt}\ge0$ — PV curtailed (kW).
* $E_{t}^{batt}$ — energia accumulata a fine intervallo $t$ (kWh).
* (Opzionale) variabili binarie se vuoi priorità rigide / assegnazioni esclusive — non necessarie per LP.

---

# 4) Vincoli (per ogni $t=0,\dots,H-1$)

### (A) Limite per colonnina

$$
p_{c,t}^{PV} + p_{c,t}^{Batt} + p_{c,t}^{Rete} \le P_c^{\max}\quad\forall c,t.
$$

### (B) Vincolo fondamentale PV (qui si incorpora la previsione)

**Per ogni intervallo $t$**:

$$
\boxed{\displaystyle \sum_{c} p_{c,t}^{PV} \;+\; p_{t}^{batt,ch} \;+\; p_{t}^{PV,curt} \;=\; P_{\text{PV},t}^{\text{avail}}}
$$

* qui $P_{\text{PV},t}^{\text{avail}}$ è la produzione **misurata** per $t=0$ e la **previsione ML** per $t\ge1$.
* Questo è il punto in cui il forecast entra **esplicitamente** nel modello: la quantità PV totale prevista vincola come la PV può essere allocata in ogni intervallo futuro.

### (C) Bilancio batteria potenza

$$
\sum_c p_{c,t}^{Batt} = p_{t}^{batt,dis} \le P_{\text{batt}}^{\max},\qquad p_{t}^{batt,ch} \le P_{\text{batt}}^{\max}.
$$

### (D) Dinamica energia batteria (kWh)

$$
E_{t+1}^{batt} = E_{t}^{batt} + \eta_{ch}\, p_{t}^{batt,ch}\Delta t - \frac{p_{t}^{batt,dis}}{\eta_{dis}}\,\Delta t,
$$

con $0\le E_t^{batt}\le E_{\text{batt}}^{\text{cap}}$.

* nota: se carichi batteria con PV, $p_{t}^{batt,ch}$ appare nella (B) come presa dalla PV; se vuoi permettere carica dalla rete, aggiungi anche termine rete che contribuisce a $p_{t}^{batt,ch}$.

### (E) Bilancio rete (opzionale limite)

$$
\sum_c p_{c,t}^{Rete} \le P_{\text{rete}}^{\max}.
$$

### (F) SOC veicolo (aggiornamento su orizzonte)

Per veicolo $v$ sulla colonnina $c$:
energia erogata al passo $t$:

$$
E_{v,t}^{erog} = (p_{c,t}^{PV}+p_{c,t}^{Batt}+p_{c,t}^{Rete})\Delta t.
$$

SOC aggiornato:

$$
S_{v,t+1} = S_{v,t} + \frac{E_{v,t}^{erog}}{E_v^{cap}}\cdot 100.
$$

Vincolo: $S_{v,t+1}\le 100$.

---

# 5) Funzione obiettivo (orizzonte H) — usa previsioni per decidere oggi

Esempio (massimizzare SOC pesato a fine orizzonte e minimizzare costo rete + penalità curtail):

$$
\max \; Z \;=\; \sum_{t=0}^{H-1}\sum_{v\in\mathcal V_t} w_v \cdot \frac{E_{v,t}^{erog}}{E_v^{cap}}\cdot 100 \;-\; c_{\text{rete}}\sum_{t=0}^{H-1}\sum_{c} p_{c,t}^{Rete}\Delta t \;-\; \lambda_{curt}\sum_{t=0}^{H-1} p_{t}^{PV,curt}\Delta t.
$$

* Poiché la PV prevista compare nei vincoli (B) a ogni $t$, l’ottimizzatore **decide** se usare PV ora, immagazzinarla (p^{batt,ch}) per usi futuri, o curtailed.
* Esempio operativo: il solver valuta che se il forecast prevede picco PV in $t=2$, conviene caricare batteria in $t=2$ (o programmare ricariche future), mentre se ora $t=0$ PV è forte potrebbe assegnare più PV ai veicoli istantaneamente.

---

# 6) Rolling horizon (implementazione pratica)

Ogni intervallo reale (es. ogni 15 min):

1. Leggi misure e forecast: $P_{\text{PV},t}^{\text{avail}}$ per $t=0..H-1$ (t=0 = misurazione istantanea).
2. Costruisci il problema su orizzonte $H$ con le variabili per tutti i $t$.
3. Risolvi (LP). Applica soltanto le variabili del primo passo $t=0$ (azioni reali su colonnine e batteria).
4. Avanza di un passo: aggiorna $E^{batt}$ e $S_v$ con i valori misurati/erogati, ricevi nuovo forecast, ripeti.

Questo approccio sfrutta pienamente le previsioni PV nel decidere strategie di ricarica e di stoccaggio.

---

# 7) Esempio di vincolo PV rapido (per chiarezza)

Per $t$ generico:

$$
\underbrace{\sum_{c} p_{c,t}^{PV}}_{\text{PV→colonnine}} + \underbrace{p_{t}^{batt,ch}}_{\text{PV→batt}} + \underbrace{p_{t}^{PV,curt}}_{\text{curtail}} = P_{\text{PV},t}^{\text{avail}}.
$$

Se vuoi consentire anche che la batteria venga caricata dalla rete, allora:

$$
p_{t}^{batt,ch} = p_{t}^{batt,ch,PV} + p_{t}^{batt,ch,Rete},
$$

e solo la componente $p_{t}^{batt,ch,PV}$ entrerà nel vincolo PV.

---

# 8) Nota su come il modello precedente differiva

* Nel tuo modello iniziale avevi menzionato $p_c^{PV,ora}$ e $p_c^{PV,prev}$ ma non era chiaro **come** la PV prevista fosse vincolata: l’aggiornamento sopra unifica tutto indicando **per ogni passo** la potenza PV totale prevista e dichiara esplicitamente come viene allocata.
* Quindi: **ora la previsione è usata esplicitamente** nelle equazioni (B) per tutti i futuri $t$ nell’orizzonte di ottimizzazione.

---

# 9) Suggerimenti pratici

* Se il forecast è incerto, puoi includere un termine di rischio (es. penalità su uso rete quando forecast è incerto), o usare scenari multipli (stocastico) — ma questo complica il problema.
* Se vuoi semplicità e bassa latenza, risolvi LP su $H$ breve (1–4 h).
* Imposta $\lambda_{curt}$ piccolo ma >0 per evitare sprechi inutili.
* Testa con profili PV reali per calibrare $w_v$ e comportamento batteria (quando conviene caricare la batteria dalla PV o dalla rete).

---

Se vuoi, ora posso **generare il modello matriciale (A,b,c)** pronto per un solver o **scrivere il codice Python (cvxpy)** che implementa questa versione multi-step usando le tue variabili (PV forecast, batteria 200 kWh, 3 colonnine, $\Delta t=0.25$ h). Dimmi quale preferisci e te lo preparo subito.