# Multiplayer locale Android

Sviluppo su `feature/local-multiplayer`, senza modificare `main`.

## Implementato

- All'avvio Android, dopo il logo iniziale, schermata con il logo originale del debug level select e scelta Single Player / Multiplayer.
- Due trasporti: rete Wi-Fi/hotspot con discovery UDP e **Wi-Fi Direct** con discovery del servizio SM64, scelta del telefono e richiesta di associazione gestita da Android. Nessun server Internet o dipendenza da Google Play Services.
- Host = Mario; chi entra = Luigi, anche sul proprio schermo. Luigi usa mesh e texture di `showfloor-nds/actors/player`, non una semplice palette verde. Provenienza in `actors/player/SOURCE.md`.
- Posizione e rotazioni interpolate, animazione e frame del compagno trasmessi in rete. Buffer animazioni separato da quello locale.
- Entrambi devono premere START nella lobby. Protocollo v2, nonce destinatario, heartbeat con timer monotono indipendente dagli FPS, rifiuto dei pacchetti vecchi/malformati e avviso di disconnessione.
- Ultimo warp dell'host ritrasmesso finché la sessione rimane attiva. Il guest lo applica nel gameplay quando la destinazione dell'host è caricata. Nel selettore missioni il guest attende la scelta dell'host.
- Stelle e chiavi **raccolte durante la sessione** replicate con bitset monotoni e ritrasmissione. I progressi vengono aggiunti al file scelto su ciascun telefono; i file preesistenti non vengono uniti integralmente.

## Come provare

Installare la stessa APK multiplayer su due telefoni.

1. Multiplayer → scegliere su entrambi `WIFI OR HOTSPOT` oppure `NEARBY WIFI DIRECT`.
2. Su A scegliere Create Game, su B Find Game.
3. Per LAN: stessa rete o hotspot locale (dati mobili non necessari). Per Direct: Wi-Fi acceso, autorizzazione richiesta dall'app e, con il target SDK legacy corrente, localizzazione di sistema attiva. Su B confermare il telefono trovato, su A accettare l'associazione Android se richiesta.
4. Quando connessi, premere START su entrambi, poi scegliere un file su ciascun telefono.
5. L'host guida i cambi di livello e seleziona la missione; il guest segue nel gameplay. Usare file dedicati alle prove perché stelle/chiavi ottenute insieme vengono aggiunte ai salvataggi.
6. B annulla la lobby e libera il gruppo Wi-Fi Direct. Dopo una perdita di connessione la sessione non ammette automaticamente un altro peer: tornare al titolo o riavviare per crearne una nuova.

La modalità Direct non richiede router, Internet o dati mobili, **ma usa comunque la radio Wi-Fi**. Non è Bluetooth/NFC. La disponibilità dipende dal supporto Wi-Fi Direct del telefono. Per errori di discovery controllare Wi-Fi, permessi e localizzazione; gli errori del framework sono mostrati nella lobby.

## Limiti ancora aperti

Questo branch **non è ancora equivalente a sm64coopdx o a una cooperativa completa**:

- nemici, oggetti dinamici, monete, piattaforme, oggetti tenuti in mano e RNG simulano ancora localmente;
- niente collisioni/interazioni fra i due giocatori;
- i warp istantanei che cambiano area senza passare da `initiate_warp`, cutscene speciali e tutti i percorsi di morte/rientro richiedono verifica e ulteriore coordinamento;
- Luigi è la mesh near originale: effetti speciali dei cappelli, espressioni, oggetti tenuti e LOD non sono ancora adattati completamente;
- la LAN accetta il primo peer rilevato e non fornisce autenticazione crittografica; usare reti locali fidate;
- discovery LAN broadcast può essere bloccato dal router/hotspot. Direct non usa quel broadcast, ma la sua discovery dipende dal framework Android.

## Verifiche

Eseguito nell'ambiente di sviluppo:

- `bash tests/run_netplay.sh`: due processi indipendenti con socket UDP reali su loopback; handshake ready, animazioni diverse, warp, progressi e reset. Backend Direct Android sostituito da un gruppo simulato **solo nel test**.
- Test pacchetti: versione errata, dimensione eccessiva, destinatario errato, NaN, replay e timeout.
- Compilazione del trasporto Android/non Android con `-Wall -Wextra -Werror`.
- Controllo sintattico C dei file modificati, del layout e della mesh Luigi con texture convertite.
- Compilazione Java 8 di tutte le classi contro Android API 28 e controllo C del bridge JNI contro gli header SDL.

**Non eseguito localmente**: build APK completa con ROM/NDK, rendering del gioco e partita su due Android reali. I test di trasporto non dimostrano la correttezza dell'intero gameplay o della discovery radio. La workflow APK del contenitore Gradle resta il controllo della build completa.
