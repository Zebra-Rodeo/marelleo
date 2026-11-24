# The enclosure

**English** · [Français](README.fr.md)

Three parts, PLA, printed on a Bambu Lab A1.

| File | Size | Settings |
|---|---|---|
| `marelleo-corps.stl` | 120 × 120 × 80 mm | open face up, no supports |
| `marelleo-base.stl` | 120 × 120 × 29 mm | flat |
| `marelleo-molette.stl` | 24 × 24 × 12 mm | flat, 100 % infill |

0.4 mm nozzle, 0.2 mm layers, three walls, 15 % infill.

The circle engraved on the top says where to put the figurine, because the NFC
reader's antenna is right underneath. Its range goes through 3 mm of PLA
without difficulty.

**M3 brass inserts**, not self-tapping screws. Screwing into printed plastic
holds once or twice, then the material splits along the layers. The insert
goes on the hole, you come down on it with the tip of the soldering iron, the
brass heats up, melts the plastic around itself and sinks in. As it cools you
have a metal thread captured in the part.

The model was drawn in Fusion 360 on the free hobbyist licence.

## The panel-mount USB-C socket

`corps-prise-usb.scad` reworks the body for a snap-in USB-C female socket, the
one that saves opening the box to charge it. The original opening is a 9 x
3.5 mm oval sized to let a plug through; a panel socket needs a dimensioned
cutout and a precise wall thickness, the one its groove clamps onto.

The script fills the original pocket, which was down to 1 mm, brings the wall
to the wanted thickness across the whole bezel area, and cuts the
manufacturer's aperture, 13.6 x 6.3 mm with R1.3 corners.

```sh
openscad --export-format binstl -o marelleo-corps-usb.stl corps-prise-usb.scad
openscad -D GABARIT=true --export-format binstl -o gabarit.stl corps-prise-usb.scad
```

`EPAISSEUR` is the only number to revisit: it is the groove width, to be
measured with callipers on the socket you receive. The test coupon is a 34 x
18 mm slice taken around the cutout, same thickness and same print
orientation: ten minutes of PLA to find out whether the groove grips, instead
of ten hours of enclosure.
