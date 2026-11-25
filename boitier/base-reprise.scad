// La base, reprise pour des inserts en laiton.
//
// Les quatre trous du raft qui recoivent la carte font 0,95 mm de diametre,
// mesures dans le maillage. Un insert M2,5 fait 3,5 mm dehors : on lui demande
// donc de deplacer presque toute la matiere qu'il rencontre, et elle n'a nulle
// part ou aller. Elle remonte dans le pas de vis, qui se bouche.
//
// La regle est d'avoir un trou un peu plus petit que l'insert, pas dix fois
// plus petit : le laiton fond les trois ou quatre dixiemes qui restent et s'y
// ancre. On perce donc a 3,2 et on garde la profondeur d'origine, 6,5 mm pour
// un insert de 5, le millimetre et demi du fond servant de reserve a la
// matiere repoussee.
//
//   openscad --export-format binstl -o marelleo-base-usb.stl base-reprise.scad

INSERT_D = 3.5;       // diametre exterieur de l'insert, M2,5 x 5 x 3,5
JEU      = -0.3;      // le trou est plus petit que l'insert : c'est voulu
PERCAGE  = INSERT_D + JEU;

// Releve dans le maillage : quatre trous, fond a z = 22,5, dessus a z = 29.
TROUS = [[49.5, 91.5], [102.5, 91.5], [49.5, 28.5], [102.5, 28.5]];
Z_FOND = 22.4;
Z_HAUT = 29.1;

difference() {
  import("marelleo-base.stl", convexity = 10);
  for (t = TROUS)
    translate([t[0], t[1], Z_FOND])
      cylinder(h = Z_HAUT - Z_FOND, d = PERCAGE, $fn = 48);
}
