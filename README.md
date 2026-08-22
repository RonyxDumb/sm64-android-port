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

## Compilazione su PC

Per compilare l'APK da Windows è consigliato usare il progetto Android completo.

### Requisiti

Installa:

- Android Studio oppure Android SDK
- Android NDK
- Java JDK compatibile con la versione di Gradle del progetto
- Git

Il progetto utilizza:

- SDL2
- OpenGL ES
- NDK Build
- Gradle

### Clonazione

```bash
git clone <URL_DEL_TUO_REPOSITORY>
cd sm64-port-android-base
```

Se `app/jni/src` è configurato come submodule:

```bash
git submodule update --init --recursive
```

Inserisci quindi:

```text
baserom.us.z64
```

nella directory richiesta dal sorgente SM64.

### Build APK

Su Windows PowerShell:

```powershell
.\gradlew assembleDebug
```

Per una build completamente pulita:

```powershell
.\gradlew clean
.\gradlew assembleDebug
```

L'APK verrà generato normalmente in:

```text
app/build/outputs/apk/debug/
```

## Compilazione da Termux

È possibile compilare anche direttamente su Android usando **Termux**.

È consigliata la versione di Termux distribuita tramite F-Droid.

### Installazione dipendenze

```sh
pkg update
pkg install git wget make python getconf zip apksigner clang
```

### Clonazione

```sh
git clone <URL_DEL_TUO_REPOSITORY>
cd sm64-port-android
```

### Copia della ROM

Concedi a Termux l'accesso alla memoria:

```sh
termux-setup-storage
```

Poi copia la ROM:

```sh
cp /sdcard/percorso/della/baserom.us.z64 ./baserom.us.z64
```

### SDL

Se necessario:

```sh
./getSDL.sh
```

### Compilazione

```sh
make --jobs 4
```

Il numero di job può essere aumentato sui dispositivi con più core CPU.

L'APK risultante viene generato nella cartella di build, ad esempio:

```text
build/us_pc/sm64.us.f3dex2e.apk
```

## Struttura del progetto

Le parti principali del sorgente sono:

```text
sm64-port-android
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
