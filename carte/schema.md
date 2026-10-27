# The board, for a production run

**English** · [Français](schema.fr.md)

Target: 50 units from JLCPCB, 70 × 60 mm board, 4 layers, components on a
single side so that only one setup and one stencil get paid for.

**The pinout is identical to the prototype's.** The current firmware runs
without a single line changed, which removes one unknown at a moment when
there are already enough of them.

## The rails

| Rail | Where it comes from | What it feeds |
|---|---|---|
| `VBUS` 5 V | USB-C socket | the charger, and nothing else |
| `VSYS` 3.0 to 4.2 V | power path charger | the buck and the boost converter |
| `+5V` | boost from `VSYS` | the amplifier, and the buck below |
| `+3V3` | buck **from the 5 V** | module, SD card, NFC reader |

The power path matters: it allows playback while charging without upsetting
end-of-charge detection, which the current build cannot do cleanly.

**The 3.3 V is made from the 5 V, not from the battery.** An empty battery
drops to 3.0 V, below the 3.3 V wanted, and a buck converter cannot raise a
voltage: the box would shut down with energy still in it. Starting from the
5 V guarantees an input always higher than the output. The boost converter
therefore runs the whole time the box is on, which is of no consequence here.

## Power off, rethought

Today the switch cuts in the middle of a write, hence the power-cut resistant
save in `library.cpp`. On the board, a **latch**:

1. the potentiometer's switch pulls the charger's `SYSOFF` pin to ground,
   which opens its output rail and starts the box;
2. the processor boots and holds that pin low itself, not by touching it, but
   **through Q1's gate** driven by `IO21`;
3. at shutdown it finishes its writes, unmounts the SD card, then releases the
   gate. R12, 1 MΩ to VBAT, pulls `SYSOFF` back up and the box turns off.

This is a function the charger provides, not a makeshift circuit.

**The transistor is not decoration.** `SYSOFF` lives on battery voltage, 4.2 V,
beyond the 3.6 V the ESP32 accepts on a pin. And an unpowered pin drags the
node to within a diode of ground through its protection diodes, that is to say
to a low level, that is to say a box that turns itself back on. Q1 isolates the
two worlds: the processor only ever sees 3.3 V, and releasing the gate turns
the box off for good.

No more truncated files, and the button becomes a request rather than an order.

**None of this is written in the firmware yet.** As of today it uses neither
`IO21` nor `IO2`. The hardware carries the latch, the software does not use it:
the box turns on and stays on as long as the switch is closed. To be written,
and it will ship over the network.

## The module, pin by pin

| Pin | Goes to | Why |
|---|---|---|
| `IO4` | amplifier `SD_MODE` | mutes the amplifier at idle |
| `IO5` `IO6` `IO7` | amplifier `DIN` `BCLK` `LRC` | I2S audio link, order permuted |
| `IO8` `IO9` | NFC connector, 4 ways | serial link to the remote PN532 |
| `IO10` | potentiometer wiper | ADC1 mandatory, ADC2 is mute under WiFi |
| `IO11` `IO12` `IO13` `IO14` | SD card `CS` `MOSI` `CLK` `MISO` | SPI, unbuffered, `CLK` and `MOSI` permuted |

**Five signals are not where they were on the prototype.** Their order was
chosen by trying every orientation of the amplifier and every pin permutation,
then counting track crossings on the real geometry: it goes from four crossings
to zero, without lengthening the copper. The firmware carries both pinouts, the
prototype's by default, the board's under `-DCARTE_IMPRIMEE`, which is the
`carte` environment in `platformio.ini`.

| `IO18` | button | internal pull-up, no resistor |
| `IO48` | status LED | already wired that way |
| `IO1` | divider on `VSYS` | battery level, which is missing today |
| `IO21` | charger `SYSOFF` | the shutdown latch |
| `EN` | `R18` 10 kΩ to 3.3 V and `C13` 1 µF to ground | the module's reset |
| `IO0`, `RXD0`, `TXD0` | test pads `TP1` to `TP3` | factory programming and console |
| `IO19` `IO20` | USB-C `D-` `D+` | native USB, for factory flashing |
| `IO43` `IO44` | test pads | serial console, without a dedicated chip |
| `IO0`, `EN` | test pads | boot and reset |

## The connectors, all removable

The board sits in the bottom of the enclosure, the NFC reader under the lid:
everything that is not on the board plugs in.

| Reference | Ways | To |
|---|---|---|
| `J2` | 4 | PN532 module: `3V3`, ground, `RX`, `TX` |
| `J3` | 2 | speaker, 4 Ω |
| `J4` | 3 | volume potentiometer |
| `J5` | 2 | button |
| `J6` | 2 | battery, JST-PH 2.0 |

## What the SD card gains

No more module, no more `74LVC125` buffer, no more diverted 5 V: a microSD
connector on 3.3 V, 22 Ω series resistors on clock and data, 10 kΩ pull-ups,
and twenty millimetres of track instead of twenty centimetres of wire. That is
the root cause of half the prototype's failures.

## Placement constraints, non-negotiable

- **Copper keep-out** under the module's antenna, at the edge of the board, on
  all four layers.
- **Switching converters and amplifier** in one corner, with their capacitors,
  far from the module and from the NFC link.
- **470 µF capacitor right against the amplifier**, the lesson measured at the
  bench: five steps of distortion threshold gained.
- USB-C socket **screwed to the wall** and connected by a lead, not reachable
  through a hole, on pain of tearing the connector off at the first drop.

## The values, checked one by one

Datasheets consulted on 23 September 2026. The eight values I had put down from
memory carried a question mark; here is what became of them.

| Reference | Role | Value | Check |
|---|---|---|---|
| R1 / R2 | set the boost's 5 V | 75 k and 10 k | 0.6 V reference confirmed, 0.6 × (1 + 75/10) = **5.10 V** |
| R4 / R5 | used to set the 3.3 V | **0 Ω and nothing** | the AP63203 is fixed output, see below |
| R7 | charge current | 1 k | 890 / 1000 = **890 mA**, within the 590 Ω to 5.9 k range |
| R8 | current drawn from USB | 1.5 k | 1550 / 1500 = **1.03 A**, never more than an amp from the charger |
| R9 | unused temperature sense | 10 k to ground | that is the value the datasheet asks for to disable the sense |
| R13 | amplifier gain | **nothing** | see below |
| L1 / L2 | inductors for both converters | 4.7 µH | allowed range 2.2 to 10 µH, ripple calculated at 28 % of the current |

**The mistake that would have killed the boards.** The AP63203 comes in two
families: the AP63200 and AP63201 with adjustable voltage, and the **AP63203
with a fixed 3.3 V output**. I had chosen the second and wired the first. My FB
pin would have seen 0.79 V instead of the 3.3 V it expects, the regulation loop
would have concluded the output was too low and pushed up to **13.7 V** on the
3.3 V rail: module, SD card and NFC reader destroyed at first power-up, across
all fifty boards. The fix costs the board nothing: R4 becomes a 0 Ω link, R5 is
not fitted, the copper does not move.

**The amplifier's gain.** The MAX98357A table: 100 k to ground gives 15 dB, a
link to ground 12 dB, **pin floating 9 dB**, a link to 3.3 V 6 dB. I had put
100 k, that is to say the maximum. The prototype has been playing at 9 dB from
the start: that is the volume that was set by ear and capped in the firmware.
The board therefore keeps 9 dB, by not fitting R13. The pad stays in place in
case anyone wants to try louder one day, but for an object sitting near a
child's head, going up 6 dB is not something you decide by default.

**Three values changed afterwards, for a catalogue reason.** My 73.2 k, 910 Ω
and 1.02 k are E96 series values, correct by calculation but absent from the
manufacturer's regular stock: each would have been billed as a special part.
The replacements are stock values, and the result does not suffer: 5.10 V
instead of 4.99, charging at 890 mA instead of 978, and an input limit brought
down from 1.52 A to **1.03 A**, which is rather a gain. The box will never ask
more than an amp from a charger, which any adapter lying around can supply, and
the charger slows down on its own if the input sags.

**Component orientation.** The manufacturer's basic parts table gives a 180
degree rotation correction for **two** of our components, the SS34 and the
2N7002: their zero is not KiCad's. It is noted in `composants.csv`, and it is
to be confirmed on their review rendering before production. A reversed diode
on the boost converter means fifty boards that do not start.

**The inductors, and their footprint.** L1 carries the boost converter's input
current, around 0.6 A on average with peaks at 1.5 A when the WiFi transmits
during a bass note. The part chosen, `C206284`, is rated for **2 A saturation
and 2.9 A RMS**, with 60 mΩ of resistance. The margin is there.

That left its footprint, the last uncertainty in the bill of materials. Its
datasheet gives the recommended land pattern, to be compared with ours:

| | recommended | our footprint | difference |
|---|---|---|---|
| pad length | 3.7 mm | 3.7 mm | none |
| pad width | 1.1 mm | 1.2 mm | +0.1 |
| gap between pads | 1.9 mm | 1.6 mm | −0.3 |
| outer edge from centre | 2.05 mm | 2.00 mm | −0.05 |

The part's electrode is 0.95 mm wide and runs from radius 1.05 mm to the edge
of the body, at 2.00 mm. Our pad covers 0.80 to 2.00 mm: **the electrode sits
entirely on copper**. The 0.25 mm of pad sticking out inwards passes under the
component's insulating resin, with no consequence, and a 1.6 mm gap rules out
any solder bridge. On the outer edge, the one that makes the joint, the
difference from the recommended pattern is 0.05 mm.

**The footprint is fine, it will not be changed.**

## The September review, and what it corrected

Three faults were found at review, two of which simply stopped the board
working. All verified against the file and against the datasheets before
correction.

**The boost converter's diode was the wrong way round.** In KiCad, pad 1 of a
diode is the **cathode**, and the symbol drawn on the fabrication layer shows
it: the bar is on the pad 1 side, the triangle on the pad 2 side. The file had
`D1: (SW_5V, +5V)`, so cathode on the switching node. A diode that way round
never lets the output rise: no 5 V, therefore no 3.3 V since it comes down from
it, therefore nothing starts, and the MT3608 takes the overvoltage from its own
node. D1 was rotated 180 degrees, which swaps the position of the two pads
without moving a single millimetre of copper, the pads being symmetrical to
within ±2 mm.

**The box could not turn off.** TI's datasheet, table 7-1: "SYSOFF is
internally pulled up to V_BAT through a large resistor (approximately 5 MΩ)".
The internal pull therefore goes **up**. Our 100 k R12 was connected to VSYS,
that is to say to the rail SYSOFF cuts: as soon as it shuts down, VSYS falls to
zero and R12 pulls SYSOFF to ground against 5 MΩ, meaning 0.08 V, meaning a
clean low level, meaning the box back on. And IO21, sitting directly on that
node, saw 4.2 V there where the ESP32 accepts 3.6.

The fix comes down to two parts:

- **R12 becomes 1 MΩ to VBAT**, the only rail present when the box is off. 4 µA
  of leakage, and a clean pull-up.
- **Q1, a 2N7002**, goes in between: gate on `ARRET_CMD` driven by IO21, drain
  on SYSOFF, source to ground, with R19 of 100 k as gate pull-down. The
  processor never sees battery voltage again, and releasing the gate turns the
  box off.

**Charging was limited to 500 mA.** Table 7-2 of the same datasheet: `EN2=0,
EN1=1` is **USB500** mode, and the ILIM resistor is only read for `EN2=1,
EN1=0`. R8, chosen for 1.03 A, was doing nothing. The two pins were swapped.

**A fourth fault, in the tooling.** The project had `min_clearance` at **zero**:
the design rule check verified no clearance at all. The net class imposed
0.2 mm and that is what did the work, verified by rerunning the check at 0.2
then 0.25. The rule is now at 0.15 mm.

**What remains, and is not fixed here:** VSYS runs 71 mm at 0.2 mm with no
plane to help it, while carrying the boost converter's input current. The
IPC-2221 calculation gives +50 °C and 0.26 V of drop at 1.5 A. To be widened to
0.8 mm when routing, like `SW_5V` and `SW_3V3` which are switching nodes.

## Bill of materials

**The part numbers live in `composants.csv`**, one line per reference, each
with its family at the manufacturer. Twelve basic parts, twelve extended.
`pcb/stock.py` queries their catalogue and reports, for each part number, the
remaining stock and the price at the quantity we care about.

Prices recorded on 23 September 2026, for 50 boards:

| Item | Per board |
|---|---|
| the five chips | $6.42 |
| the eight connectors | $2.70 |
| everything else, passives and inductors | $1.35 |
| **component total** | **$10.48** |

Two surprises in those figures. The module costs **$3.65** on its own, more
than a third of the total. And the two connectors J1 and J7, the USB-C socket
and the microSD holder, cost **$2.44** together, more than the other four chips
combined. If two dollars a board ever need to be saved, that is where they are,
not in the resistors.

That amount only covers **what is placed on the board**. The battery, the
speaker, the potentiometer, the button, the NFC reader, the SD card, the
figurines and their tags come on top, as does the printed enclosure.

The five chips were chosen because **KiCad has their symbol**: their pinout is
read from the library instead of being copied out, and no pin number is written
by hand in `generer.py`.

One point to check on the datasheet before producing: the thermal pad under the
BQ24075 measures 1.1 × 1.1 mm in KiCad's footprint, to be compared with the
dimension the manufacturer gives.

## Two stockouts, and what they teach

At the first stock check, two references out of twenty-four were at zero: the
470 µF capacitor and the four-way JST connector. Replaced by `C82004`, same
6.3 × 7.7 mm package but **rated 10 V instead of 6.3**, which is the right
margin on a 5 V rail, and by `C157926`, the same original JST under another
listing.

The lesson fits on one line: **a part number is not a given**. Stock gets
rechecked on the day of the order, `python3 pcb/stock.py`, and not before. The
tightest one today is precisely that capacitor, with 2,956 pieces for the 50
we need.

## Validated at the bench, 15 October 2026

One of the five boards received was assembled with its six harnesses and
**everything works**: the 3.3 V and 5 V rails, the charger's power path, the SD
card at 29.8 GB, the PN532 reader over serial, the potentiometer across its
whole travel, the button, AAC decoding into the speaker, WiFi, the captive
portal, updating from `maj.marelleo.org`, running on battery and powering off
at the click at the bottom of the knob's travel.

Three uncertainties in this document are therefore settled by measurement
rather than by reading a datasheet:

- **Both rotation corrections were right.** The SS34 and the 2N7002 had a zero
  different from KiCad's. Had D1 been placed the wrong way round, the boost
  converter would not put out 5 V and nothing would start; had Q1 been, the
  shutdown latch would not hold. Both work.
- **The BQ24075's thermal pad is enough.** Charging at 890 mA holds without the
  chip going into thermal foldback.
- **L1's footprint holds**, under the boost converter's 1.5 A peaks.

And one absence confirmed: **the 22 Ω series resistors on the SD card bus are
not necessary.** They were not fitted, and the card mounts and reads without a
single error.

**What is still open.** The firmware never drives `IO21`, so shutdown
*commanded by software* through Q1 exists in hardware and is not proven; only
the potentiometer's click is. And LittleFS is not formatted, so a board shipped
without an SD card would have no storage at all.

## Two start-up traps, which are not board faults

The only two failures at start-up came from the wiring, and each costs an hour
to anyone who does not know them.

**The NFC reader's lead crosses over.** On the V3 module, the shared pins are
labelled `SDA/TXD` and `SCL/RXD`: `SDA` is what the module **transmits**, so it
goes to `NFC_RX` on the board, and `SCL` to `NFC_TX`. Wired straight, RX to RX,
the module powers up, its LED blinks, and it never answers.

**And `nfc.begin()` only runs once, in `setup()`.** Fixing the lead achieves
nothing without a real restart. Except that **the potentiometer's switch does
not cut the board while USB is plugged in**: `SYSOFF` only commands the
battery's transistor, and the input feeds VSYS from its own side. To restart
you have to pull the USB, not turn the knob.

## Two corrections for the next board

**The sense divider is on the wrong rail.** `IO1` reads VSYS, that is to say
the charger's output. But the power path connects input to output as soon as an
external source is present: measured on the board, USB plugged in, VSYS reads
**5.01 V**. The cell's voltage therefore becomes invisible for the whole of
charging, and that is precisely when people look at the gauge. The software
gets out of it by showing "on mains" and the last known level, but that is a
plaster.

The fix is one stroke: **move the divider from VSYS to VBAT**. The cell then
becomes readable at all times. Two precautions: 1 MΩ resistors rather than
100 kΩ, so the divider does not drain the battery when the box is off, and
remembering that a charging cell reads high, as on any phone.

**`~CHG` is connected to nothing.** It is the only charger pin left free.
Routed to a processor input, it would say whether charging is in progress or
finished, which no voltage measurement gives. `IO15`, `IO16` and `IO17` are
available.

## SYSOFF suspends charging too

Measured on the board, USB plugged in, across J6:

| Position | Voltage | What happens |
|---|---|---|
| knob up | 3.29 V | the box charges, and draws current at the same time |
| knob down | 3.11 V | **charging is suspended** |
| knob down, latch held by `IO21` | 3.47 V | it charges without drawing anything |

`SYSOFF` therefore does not merely disconnect the battery from the system: it
suspends charging. A box switched off at the knob does not recharge, which is
not what anyone expects from an object they put on its charger before going to
bed.

The board already carried the answer without anyone using it: **`Q1`, gate on
`IO21`, pulls `SYSOFF` to ground in parallel with the switch**. The firmware
therefore holds the latch itself while USB is present, and releases it as soon
as it goes away, without which the box would no longer be able to turn off at
all and would drain its cell.

This is the first use of the shutdown latch, drawn into the schematic from the
start and left inert until 15 October 2026.

### The latch is also how it says goodbye

On battery, the switch's click cuts the power immediately: the box has no time
to chime, and until 16 October it died mid-sentence, without saying anything.
The same latch provides the window that was missing.

The firmware **holds the latch as soon as the knob touches the bottom**, before
the click, which comes a few degrees further on. `SYSOFF` is already at ground
through `Q1` by then: the switch can open, nothing moves. The box plays its
chime in reverse, releases the latch, and that is where the power falls, if the
switch really is open. Otherwise the knob was only at minimum, and the box
restarts cleanly.

The window is deliberately short, a second and a half plus the length of the
chime. Firmware hung while it is open would keep the battery connected with the
knob switched off, and would drain the cell overnight. Outside that window the
latch is released and the hardware is sole master of the power again, as
before.

## The charger runs hot, and that is adjustable

Measured by finger on the board, battery at 3.65 V, USB plugged in: the BQ24075
is burning hot. Nothing abnormal, it is a **linear** charger, which dissipates
the difference between the input and the cell:

```
(5.01 V − 3.65 V) × 0.89 A = 1.21 W
```

That is 60 to 70 °C on the surface of a 3 × 3 mm package. It is the worst
moment, the difference being greatest when the battery is low; at 4.2 V the
dissipation falls to 0.7 W. The chip protects itself by reducing its current to
stay under 125 °C.

**Proposed fix: `R7` from 1 kΩ to 2 kΩ**, which brings the charge current down
from 890 mA to 445 mA. The heat is halved and charging goes from three hours to
six. For an object you put on its charger in the evening, six hours cost
nothing, and a chip that is warm rather than burning hot, in a closed
enclosure, next to a lithium cell, in a child's bedroom, is well worth them.

Worth noting: before the firmware held `SYSOFF` through `IO21`, the chip did
not get hot because charging was simply suspended. The heat is the price of it
finally recharging.
