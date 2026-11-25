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

## The reworks

Two OpenSCAD scripts rework the printed parts from the STLs, without going
back to Fusion. Every dimension they use was measured in the mesh, not
remembered.

```sh
openscad --export-format binstl -o marelleo-corps-v2.stl corps-reprise.scad
openscad --export-format binstl -o marelleo-base-v2.stl  base-reprise.scad
openscad -D GABARIT=true --export-format binstl -o gabarit.stl corps-reprise.scad
```

**`base-reprise.scad`: the four raft holes.** They were 0.95 mm across, for
M2.5 inserts 3.5 mm in outer diameter. The insert had to displace almost all
the material it met, and that material came back up into the thread. Now
drilled at 3.2 mm, original depth kept, 6.5 mm for a 5 mm insert: the last
millimetre and a half is where the displaced material goes.

**`corps-reprise.scad`: the USB-C socket and the NFC reader's pads.**

The original USB opening is a 9 x 3.5 mm oval sized to let a plug through. A
panel socket needs a dimensioned cutout, 13.6 x 6.3 with R1.3 corners, and a
precise wall thickness, the one its groove clamps. Around the old opening the
wall had been hollowed to 1 mm, elsewhere it is 4 mm: the script fills the
pocket, brings the whole bezel area to `EPAISSEUR`, and cuts the aperture.

The NFC reader's two pads, 12 x 12 x 3 mm, were solid: no pilot hole, hence
material going everywhere when an insert is melted in, and 3 mm of grip for an
insert 5 mm long. They are now bored at 3.2 mm, and made taller only if the
chosen insert requires it. `INSERT_L` defaults to 3, which leaves the module
exactly where it is: longer pads push the antenna away from the figurine, and
read range pays for it.

`EPAISSEUR` and `INSERT_L` are the two numbers to revisit for the parts you
have. The test coupon is a 34 x 18 mm slice taken around the cutout, same
thickness and same print orientation: ten minutes of PLA to find out whether
the groove grips, instead of ten hours of enclosure.
