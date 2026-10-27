# The firmware

**English** · [Français](README.fr.md)

**Written for this exact build.** The pinout lives in the code, at the top of
`src/main.cpp`, and the two environments describe my printed board and my
module prototype. A build wired differently needs the matching constants
changed there: without that it will not work, and not always loudly. A silent
NFC reader and a missing SD card are the two most common symptoms of a pin
that is not where the code thinks it is.

It is also a weekend project, not a product. It runs every day at my house,
which is a sample of one box. Take it as a starting point that works.

Two environments, one body of code.

```sh
pio run -e boite -t upload     # modules on breadboard or perfboard
pio run -e carte -t upload     # printed board
pio device monitor
```

| | `boite` | `carte` |
|---|---|---|
| Module | ESP32-S3-DevKitC-1 N16R8 | ESP32-S3-WROOM-1-N8R2 |
| Flash | 16 MB | 8 MB |
| PSRAM | 8 MB octal | 2 MB quad |
| Serial console | CH343 chip, survives resets | the processor's native USB |

Picking the wrong environment gives you a board that does not start at all,
without a line on the console. Both traps are explained in `platformio.ini`,
above the settings concerned.

## Where things are

| File | Role |
|---|---|
| `main.cpp` | the loop, playback, volume, power off |
| `library.cpp` | the figurine to tracks table, resistant to power cuts |
| `serveur.cpp` | the API and the web page |
| `page.h` | the whole interface, in a single literal |
| `reseau.cpp` | WiFi, and the portal when no network is known |
| `maj.cpp` | updating over the network, with rollback |
| `pn532_hsu.cpp` | the NFC reader over serial |
| `batterie.cpp` | the voltage, turned into a gauge that never goes back up |
| `carillon.cpp` | the two notes, generated sample by sample |

The source is commented in French, which is the language I think in while
writing it. The identifiers follow: `biblio` is the library, `figurine` is a
figurine, `piste` a track, `molette` the volume knob, `veille` sleep, `maj` an
update.

The web page is compiled into the firmware: no external resources, since the
box has to answer without internet access.

## Working on the interface without the box

The private repository carries a fake box in Python that serves the page
extracted from `page.h` and answers its API. It is not published here: it
depends on fixture files that are only of interest to me. The page tests just
as well against a real box, which answers in a second.
