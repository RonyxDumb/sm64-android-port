# Super Mario 64 Android Port

Port Android di **Super Mario 64**, basato sul progetto decompilato di SM64 e adattato per dispositivi Android tramite **SDL2** e **OpenGL ES**.

Questa versione include modifiche specifiche per Android, controlli touch e correzioni dedicate al rendering, al frame pacing e alla compatibilità con dispositivi moderni.

## Funzionalità

- Port nativo Android di Super Mario 64
- Rendering tramite OpenGL ES
- SDL2 per finestra, input e audio
- Controlli touchscreen
- Supporto controller
- Supporto fullscreen
- Salvataggi su memoria Android
- Correzioni al frame pacing e al VSync
- Miglioramenti specifici per dispositivi Android moderni
- Compatibilità con ARM e ARM64

## Requisiti

Per compilare il progetto è necessaria una copia originale della ROM di **Super Mario 64 USA**.

Il repository non include ROM, asset proprietari o altri file protetti da copyright.

Il file richiesto deve essere chiamato:

```text
baserom.us.z64
```

## Compilazione APK da Windows

Il codice di questo repository e il progetto Gradle Android sono separati.
Utilizza [sm64-port-android-base](https://github.com/RonyxDumb/sm64-port-android-base)
per generare l'APK con SDK/NDK, JDK 11 e Gradle 7.1.

```sh
git clone --recursive https://github.com/RonyxDumb/sm64-port-android-base.git
cd sm64-port-android-base
git -C app/jni/src fetch https://github.com/RonyxDumb/sm64-android-port.git main
git -C app/jni/src checkout --detach FETCH_HEAD
```

Inserisci una ROM USA compatibile, ottenuta legalmente, in
`app/jni/src/baserom.us.z64`. Segui la [guida Windows del progetto base](https://github.com/RonyxDumb/sm64-port-android-base/blob/master/SETUP.md)
per SDL2, MSYS2, NDK e Gradle, poi genera l'APK con `gradlew.bat assembleDebug`.

**Nota:** il submodule predefinito di `sm64-port-android-base` punta a
`sm64-port-android`, non a questo fork. I due comandi `git -C` qui sopra
servono a selezionare esplicitamente i sorgenti di `sm64-android-port`.

## Compilazione APK direttamente su Android con Termux

Utilizza [Termux da F-Droid](https://f-droid.org/packages/com.termux/)
e mantieni il progetto nella home di Termux (non in `/sdcard`, che non
consente la normale esecuzione dei programmi compilati).

### Dipendenze

```sh
pkg update && pkg upgrade -y
pkg install -y git wget unzip make python zip apksigner clang libglvnd libglvnd-dev
```

Gli header OpenGL ES devono essere disponibili, in particolare
`$PREFIX/include/GLES2/gl2platform.h`; altrimenti il compilatore segnala
`GLES2/gl2platform.h file not found`.

Il Makefile non richiede più `which` o `getconf` per la build Termux:
riconosce `aarch64` come `arm64-v8a` e usa direttamente `python3` per
l'estrazione degli asset.

### Sorgenti, ROM e SDL2

```sh
git clone https://github.com/RonyxDumb/sm64-android-port.git
cd sm64-android-port
termux-setup-storage
cp ~/storage/downloads/baserom.us.z64 ./baserom.us.z64
bash getSDL.sh
```

La ROM deve corrispondere alla regione selezionata (`VERSION=us`) e
superare la verifica SHA-1 del progetto. Non caricarla su GitHub.

### Compilazione consigliata (memoria limitata)

```sh
bash tools/build-termux.sh
```

Lo script verifica le dipendenze principali, prepara SDL2 se necessario,
esegue `make -j1 VERSION=us` per ridurre il consumo di RAM, e verifica la
firma APK. Si può anche eseguire direttamente:

```sh
make -j1 VERSION=us
```

Il Makefile sceglie automaticamente `arm64-v8a` sui dispositivi Termux
`aarch64`. In caso di toolchain personalizzate puoi passare
`ARCH_APK=arm64-v8a`. Non è necessario eseguire `make clean` per riprendere
una compilazione interrotta.

Se termina correttamente, il file firmato è:

```text
build/us_pc/sm64.us.f3dex2e.apk
```

Per verificarlo e copiarlo nei Download:

```sh
apksigner verify --verbose build/us_pc/sm64.us.f3dex2e.apk
cp build/us_pc/sm64.us.f3dex2e.apk ~/storage/downloads/SM64-Android.apk
```

### Errori affrontati

| Errore | Soluzione |
| --- | --- |
| `which: No such file or directory` | Il Makefile usa `command -v`, disponibile nella shell. |
| `extract_assets.py: Permission denied` | L'estrazione ora richiama `python3` direttamente. |
| `stat` / `mkdir` non dichiarate | Header POSIX e uso di `struct stat` corretti. |
| `GLES2/gl2platform.h file not found` | Installa `libglvnd-dev` e verifica l'header. |
| Librerie `armeabi-v7a` incompatibili con `aarch64` | Rilevamento ABI corretto; override `ARCH_APK` disponibile. |
| `apksigner: No such file or directory` | Installa `apksigner`; il Makefile ora dà un errore esplicito e verifica la firma. |
| `getconf: No such file or directory` | Il rilevamento Android non dipende più da `getconf`. |
| `Killed` durante `mario.c` | Possibile esaurimento RAM: usa `-j1` e chiudi le app pesanti. |

**Limite:** `Killed` può dipendere dal limite di memoria imposto da Android,
anche con un singolo processo; non è un errore C risolvibile garantendo solo
`-j1`. Una firma APK valida non garantisce che il gioco funzioni in runtime.

## Struttura del progetto

Le parti principali del sorgente sono:

```text
sm64-android-port
├── actors
│   └── modelli, animazioni e display list
├── assets
│   └── dati estratti dalla ROM
├── data
│   └── script e dati di gioco
├── include
│   └── header principali
├── levels
│   └── livelli e geo layout
├── sound
│   └── sequenze e sistema audio
├── src
│   ├── audio
│   │   └── motore audio
│   ├── engine
│   │   └── rendering, collisioni e sistemi interni
│   ├── game
│   │   └── gameplay e comportamenti
│   ├── menu
│   │   └── title screen e menu
│   └── pc
│       ├── audio
│       ├── controller
│       └── gfx
└── tools
    └── strumenti di build ed estrazione
```

## Multiplayer locale Android (prototipo)

Il port include ora un **menu Single Player / Multiplayer** in stile N64,
con opzioni Host / Join per due telefoni sulla stessa rete Wi-Fi o tramite
hotspot locale (non servono dati mobili). La discovery utilizza pacchetti
broadcast UDP sulla porta `45565`, quindi alcuni hotspot possono bloccarla.

Il modulo di rete e un avatar remoto sperimentale sono presenti nei sorgenti.
**Non è ancora un multiplayer cooperativo completo né verificato su due telefoni**:
mancano sincronizzazione di nemici, stelle, salvataggi, warp e animazioni remote;
anche Wi-Fi Direct e Nearby Connections non sono ancora implementati.

Per l'architettura, le limitazioni e la procedura di prova consulta
[Multiplayer locale Android](docs/android-local-multiplayer.md).

## Android

Le modifiche Android più importanti si trovano principalmente in:

```text
src/pc/controller/
src/pc/gfx/
src/pc/pc_main.c
```

### Controller

Il port include un sistema di input specifico per Android con supporto per:

```text
controller_sdl.c
controller_sdl.h
controller_touchscreen.c
```

I controlli touch vengono gestiti attraverso gli eventi SDL:

```text
SDL_FINGERDOWN
SDL_FINGERMOTION
SDL_FINGERUP
```

### Rendering

Il backend grafico Android utilizza:

- SDL2
- OpenGL ES

Il file principale per la gestione della finestra e del VSync è:

```text
src/pc/gfx/gfx_sdl2.c
```

Il renderer principale si trova invece in:

```text
src/pc/gfx/gfx_pc.c
src/pc/gfx/gfx_opengl.c
```

Il port contiene modifiche specifiche per rendere più stabile il frame pacing su Android e ridurre problemi di stuttering durante l'avvio.

## Frame rate

Super Mario 64 utilizza originariamente una simulazione di gioco a:

```text
30 Hz
```

Portare semplicemente il game loop a 60 o 120 Hz aumenta anche la velocità della simulazione.

Per frame rate superiori è quindi necessario separare la **game logic** dal **rendering**.

L'obiettivo del progetto è mantenere la logica originale a 30 Hz e utilizzare interpolazione grafica per ottenere un rendering più fluido sui display moderni.

```text
30 Hz logic
      │
      ├── stato precedente
      └── stato corrente
               │
               ▼
        interpolazione
               │
               ▼
        60 / 90 / 120 FPS
```

## Troubleshooting

### Build NDK fallisce

Esegui prima:

```powershell
.\gradlew clean
```

e, se necessario, elimina le directory generate:

```text
app/build
app/.cxx
app/obj
```

Non eliminare i sorgenti.

### `baserom.us.z64` non trovata

Assicurati che la ROM:

```text
baserom.us.z64
```

sia nella directory prevista dal Makefile.

### Errori dopo modifiche native

Il progetto contiene codice C compilato tramite NDK.

Dopo modifiche importanti a:

```text
src/pc/
src/game/
src/engine/
```

è consigliabile eseguire una build pulita.

### Gioco accelerato

Se il gioco gira a 2× o più velocemente, non aumentare direttamente il numero di chiamate a:

```c
game_loop_one_iteration();
```

La simulazione deve rimanere a circa 30 Hz.

### Rendering corrotto

Non modificare arbitrariamente:

```c
glViewport()
```

o le dimensioni del framebuffer Android.

Viewport, aspect ratio e dimensioni logiche vengono già gestiti dal renderer del port.

## Crediti

Basato sul lavoro di:

- n64decomp/sm64
- sm64-port/sm64-port
- VDavid003/sm64-port-android
- VDavid003/sm64-port-android-base

Questo repository contiene modifiche e adattamenti aggiuntivi specifici per Android.

## Disclaimer

Questo progetto non distribuisce ROM o asset proprietari di Nintendo.

Per compilare il gioco è necessaria una copia legalmente ottenuta di Super Mario 64.
