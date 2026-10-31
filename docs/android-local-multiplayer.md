# Multiplayer locale Android (prototipo)

## Studiati
- [TobiasFriedly/sm64-nds](https://github.com/TobiasFriedly/sm64-nds): `nds_net.c` usa frame Wi-Fi DS grezzi, HELLO/nonces e messaggi STATE/FILE/STAR; `nds_netplay.c` gestisce Mario remoto e stelle.
- [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx): sincronizzazione completa di entità e livelli, architettura molto più articolata.

## Stato effettivo
- Menu iniziale Android Single Player / Multiplayer con Host / Join.
- Transport sperimentale UDP IPv4 porta **45565**, discovery broadcast sulla stessa LAN oppure su un hotspot creato da un telefono.
- Sessioni a due partecipanti; pacchetti versionati, nonce di sessione (NON autenticazione), heartbeat, perdita collegamento, posizione/stato del Mario locale.
- Rappresentazione remota visuale sperimentale: *non* è ancora una cooperativa completa. Nemici, stelle, salvataggi, warp e fisica del secondo Mario **non sono sincronizzati**.
- L'host accetta automaticamente il primo peer rilevato: usare solo reti locali fidate e non esporre la porta a Internet.
- Il discovery UDP può essere bloccato dal sistema o dall'isolamento client di alcuni hotspot.
- **Non** è ancora Wi-Fi Direct o Nearby Connections: quelle modalità richiedono codice Android separato con permessi runtime e discovery specifico.

## Come provare
1. Compilare l'app Android dall'ultimo codice `sm64-android-port` nel progetto Gradle `sm64-port-android-base`. Installare su due telefoni.
2. Collegare i telefoni alla stessa rete Wi-Fi, **oppure** attivare hotspot sul telefono A e collegare il telefono B a quell'hotspot. Non occorrono dati mobili.
3. Avviare il titolo originale su entrambi, premere START, selezionare MULTIPLAYER, scegliere HOST su A e JOIN su B.
4. Attendere CONNECTED e premere START su entrambi per passare al normale File Select. La visualizzazione del giocatore remoto è possibile solo nello stesso livello e area ed è ancora sperimentale.
5. B torna indietro dal menu e chiude il socket.

## Prossimi passi necessari
- Aggiungere Nearby Connections (P2P_POINT_TO_POINT) con autorizzazioni Android e verifica umana del codice di associazione, oppure Wi-Fi Direct + gestione gruppo.
- Autorizzazione host prima di ammettere il peer, fallback IP manuale per hotspot con broadcast disabilitato.
- Replica autorevole di oggetti, nemici, stelle e salvataggi; caricamento livelli coordinato.
- Invio e riproduzione della corretta animazione remota (quella iniziale è provvisoria).
- Test build NDK/Gradle e prova reale su due dispositivi; il codice nel repo non implica che queste verifiche siano già superate.
