# Multiplayer locale Android

Sviluppo su `feature/local-multiplayer`, senza modificare `main`.

## Implementato

- All'avvio Android, dopo il logo iniziale, schermata con il logo originale del debug level select e scelta Single Player / Multiplayer.
- Due trasporti: rete Wi-Fi/hotspot con discovery UDP e **Wi-Fi Direct** con discovery del servizio SM64, scelta del telefono e richiesta di associazione gestita da Android. Nessun server Internet o dipendenza da Google Play Services.
- Host = Mario; chi entra = Luigi, anche sul proprio schermo. Luigi usa mesh e texture di `showfloor-nds/actors/player`, non una semplice palette verde. Provenienza in `actors/player/SOURCE.md`.
- Posizione e rotazioni interpolate, animazione e frame del compagno trasmessi in rete. Buffer animazioni separato da quello locale.
- Menu disegnato dal motore di SM64: font HUD colorato, caratteri dei dialoghi, cursore triangolare e riquadri della pausa originali. Nessun pannello Android sopra il gioco. Selezione animata, tocchi nelle righe, elenco Direct nello stesso stile e controlli da pad.
- Entrambi devono confermare «Sono pronto» o premere START nella lobby. Protocollo v3, nonce destinatario, heartbeat con timer monotono indipendente dagli FPS, rifiuto dei pacchetti vecchi/malformati e avviso di disconnessione.
- Le interruzioni di pacchetti mostrano «RECONNECTING» dopo 5 secondi; il peer della stessa sessione può riprendere entro 30 secondi, conservando stato pronto e progressi. Una perdita del gruppo Direct resta un errore da risolvere nella lobby. Discovery LAN su broadcast globale e delle singole interfacce.
- Feedback Shindou: attivati i suoi eventi originali di impatto, atterraggio, danno, nuoto e interazione anche con ROM US. Adattamento Android con impulsi di 24–100 ms, ampiezza massima 200/255 e intervallo minimo 150 ms; i contatti continui usano 16 ms ogni almeno 450 ms. I motori senza controllo ampiezza ricevono al massimo 24 ms. Destinazione: controller usato se supporta la vibrazione, altrimenti telefono. Scelte ON e OFF sempre disegnate e leggibili accanto a «Vibrazione», nel menu iniziale e in pausa, persistenti; OFF, pausa e background interrompono subito il motore. Nella pausa si cambia anche con L; dal menu con A o sinistra/destra sulla riga. ON produce un breve feedback di conferma. La sensazione effettiva dipende dal motore del dispositivo.
- Livelli, aree e missioni indipendenti: entrambi usano il normale selettore con A/START, anche quando il compagno è al castello o in un altro livello. Nessun teletrasporto automatico verso l'host. Gli avatar sono visibili nello stesso livello e area, anche scegliendo missioni diverse.
- Stelle e chiavi **raccolte durante la sessione** replicate con bitset monotoni e ritrasmissione. I progressi vengono aggiunti al file scelto su ciascun telefono; i file preesistenti non vengono uniti integralmente.

## Collisioni tra giocatori

Ogni telefono risolve il contatto del proprio giocatore, usando velocità e stato tangibile del compagno trasmessi nel protocollo v3. I cilindri sono separati sul piano orizzontale e la ripartizione della spinta dipende dalla direzione del movimento. Il controllo continuo limita l’attraversamento durante scatti; i controlli del terreno usano muri, pavimento e soffitto originali di SM64. Se il compagno è bloccato da una parete, il suo stato impedisce di continuare ad attraversarlo. Il modello interpolato resta esterno al cilindro locale.

Niente spinte tra aree/livelli diversi, su dati più vecchi di 300 ms, con un giocatore in pausa, nelle cutscene o con il cappello Vanish. La missione scelta non separa i giocatori. La latenza può ridurre la velocità della spinta: il comportamento radio reale resta da collaudare.

## Come provare

Installare la stessa **nuova APK con protocollo v3** su due telefoni; le APK v2 precedenti non si collegano.

1. Multiplayer → scegliere su entrambi `Wi-Fi / Hotspot` oppure `Wi-Fi Direct · Nelle vicinanze`.
2. Su A scegliere «Crea partita», su B «Cerca partita».
3. Per LAN: stessa rete o hotspot locale (dati mobili non necessari). Per Direct: Wi-Fi acceso, autorizzazione richiesta dall'app e, con il target SDK legacy corrente, localizzazione di sistema attiva. Su B confermare il telefono trovato, su A accettare l'associazione Android se richiesta.
4. Quando connessi, premere START su entrambi, poi scegliere un file su ciascun telefono.
5. Provare lo stesso livello e area con missioni differenti: Mario e Luigi devono essere visibili. Provare poi un cambio area e il ricongiungimento. Ogni giocatore sceglie liberamente livelli e missioni: non serve aspettare l'altro. Stelle e chiavi raccolte nella sessione vengono aggiunte ai file scelti. Il file **D** è predisposto a **120 stelle**, con cappelli, porte e cannoni sbloccati; A/B/C non vengono modificati dalla creazione del preset. I record monete di D superiori a 100 vengono conservati. Il preset viene ripristinato al caricamento dei salvataggi, anche dopo aver cancellato D.
6. Provare ON/OFF dal menu e dalla pausa: OFF deve arrestare subito il feedback e restare salvato al riavvio.
7. B annulla la lobby e libera il gruppo Wi-Fi Direct. Dopo un timeout definitivo la sessione non ammette automaticamente un altro peer: tornare al titolo o riavviare per crearne una nuova.

La modalità Direct non richiede router, Internet o dati mobili, **ma usa comunque la radio Wi-Fi**. Non è Bluetooth/NFC. La disponibilità dipende dal supporto Wi-Fi Direct del telefono. Per errori di discovery controllare Wi-Fi, permessi e localizzazione; gli errori del framework sono mostrati nella lobby.

## Limiti ancora aperti

Questo branch **non è ancora equivalente a sm64coopdx o a una cooperativa completa**:

- nemici, oggetti dinamici, monete, piattaforme, oggetti tenuti in mano e RNG simulano ancora localmente;
- collisioni tra cilindri e spinte reciproche implementate; non ci sono danni PvP, trasporto del compagno o piattaforme sulla testa;
- cutscene, morte/rientro e cambi di area restano locali: vanno collaudati su due telefoni per verificare la ricomparsa dell'avatar quando i giocatori si ritrovano;
- Luigi è la mesh near originale: effetti speciali dei cappelli, espressioni, oggetti tenuti e LOD non sono ancora adattati completamente;
- la LAN accetta il primo peer rilevato e non fornisce autenticazione crittografica; usare reti locali fidate;
- discovery LAN broadcast può essere bloccato dal router/hotspot. Direct non usa quel broadcast, ma la sua discovery dipende dal framework Android.

## Verifiche

Eseguito nell'ambiente di sviluppo:

- `bash tests/run_netplay.sh`: due processi indipendenti con socket UDP reali su loopback; handshake ready, animazioni diverse, warp, progressi e reset. Backend Direct Android sostituito da un gruppo simulato **solo nel test**.
- Test pacchetti: versione errata, dimensione eccessiva, destinatario errato, NaN, replay, interruzione/ripresa della stessa sessione con progressi conservati e timeout.
- Avatar: stesso livello/area con missioni diverse, nessuna ricreazione per il solo cambio missione, rimozione al cambio area, ricongiungimento e disconnessione.
- Contatti: spinte nei due versi, missioni diverse, arrivo contro un giocatore bloccato, scatti, altezza e stato non tangibile; simulazione di dati ritardati; integrazione con un mondo controllato per muri, assenza di pavimento, soffitto basso, dati vecchi, pausa e cutscene.
- Regressione selettore: il guest conferma con A/START una missione propria mentre l’host è al castello.
- Salvataggi: caricamento/scrittura EEPROM simulata, conteggio esatto di 120 stelle in D, cappelli e cannoni, conservazione di A/B/C e dei record migliori, backup/checksum e ricaricamento senza scritture ripetute.
- Compilazione del trasporto Android/non Android con `-Wall -Wextra -Werror`.
- Controllo sintattico C dei file modificati, del layout e della mesh Luigi con texture convertite.
- Compilazione Java 8 di tutte le classi contro Android API 28 e controllo C del bridge JNI contro gli header SDL.

**Non eseguito localmente**: build APK completa con ROM/NDK, rendering del gioco, sensazione del motore e partita su due Android reali. I test di trasporto non dimostrano la correttezza dell'intero gameplay o della discovery radio. La workflow APK del contenitore Gradle resta il controllo della build completa.
