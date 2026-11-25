// Le corps, repris : la prise USB-C de panneau, et les plots du lecteur NFC.
//
// L'ouverture d'origine est un ovale de 9 x 3,5 mm, taille pour laisser passer
// une fiche. Une prise de panneau demande autre chose : une decoupe cotee par
// le fabricant, et surtout une epaisseur de paroi precise, celle que sa gorge
// vient pincer. Au droit de l'ancienne ouverture la paroi avait ete creusee a
// 1 mm, trop mince pour tenir quoi que ce soit ; ailleurs elle fait 4 mm, trop
// epais pour que les clips atteignent le fond.
//
// Ce fichier remet donc la paroi a une epaisseur choisie sur la surface que
// couvre le collet, et y perce la decoupe du fabricant.
//
//   openscad -o marelleo-corps-usb.stl corps-prise-usb.scad
//
// EPAISSEUR est le seul chiffre a revoir : c'est la largeur de la gorge de la
// prise, a mesurer au pied a coulisse sur la piece recue. La fiche produit ne
// la donne pas clairement.

EPAISSEUR = 2.0;      // paroi au droit de la gorge

DECOUPE_L = 13.6;     // decoupe cotee par le fabricant, coins R1,3
DECOUPE_H = 6.3;
DECOUPE_R = 1.3;
JEU       = 0.3;      // le PLA imprime se retracte, une decoupe nue serre

// Centre de l'ouverture. Releve sur le maillage : l'ancienne fenetre est
// centree sur x = 230, et montee a z = 7 pour que le collet de 10,3 mm de haut
// tombe entierement sur la paroi, sans mordre sur le bas du corps.
CX = 230;
CZ = 7.0;

Y_EXT = 120;          // face exterieure de la paroi arriere

PANNEAU_L = 19;       // surface ramenee a l'epaisseur voulue, un peu plus
PANNEAU_H = 12;       // grande que le collet de 16,7 x 10,3

// L'ancienne poche, relevee sur le maillage : 24 x 9 mm ramenes a 1 mm. Elle
// deborde du nouveau panneau, et ce qui depasse resterait a 1 mm, une bande
// fragile juste a cote de la prise. On la comble entierement.
POCHE_X = [216, 243];
POCHE_Z = [0, 9.5];

// Prisme a coins arrondis, couche le long de Y.
module prisme_arrondi(l, h, r, prof) {
  hull()
    for (sx = [-1, 1]) for (sz = [-1, 1])
      translate([sx * (l / 2 - r), 0, sz * (h / 2 - r)])
        rotate([-90, 0, 0])
          cylinder(h = prof, r = r, $fn = 64);
}

module corps_repris() {
  difference() {
    union() {
      import("marelleo-corps.stl", convexity = 10);
      // Combler la poche d'origine, et la surface du collet, a l'epaisseur voulue.
      translate([POCHE_X[0], Y_EXT - EPAISSEUR, POCHE_Z[0]])
        cube([POCHE_X[1] - POCHE_X[0], EPAISSEUR, POCHE_Z[1] - POCHE_Z[0]]);
      translate([CX - PANNEAU_L / 2, Y_EXT - EPAISSEUR, CZ - PANNEAU_H / 2])
        cube([PANNEAU_L, EPAISSEUR, PANNEAU_H]);
      plots_nfc();     // allonges si l'insert choisi est plus long que 3 mm
    }

    // Degager l'interieur pour que la paroi fasse exactement EPAISSEUR sur toute
    // la surface du collet. Six millimetres suffisent : au-dela c'est deja creux,
    // et creuser plus loin risquerait de manger une nervure qu'on ne voit pas.
    translate([CX - PANNEAU_L / 2, Y_EXT - 6, CZ - PANNEAU_H / 2])
      cube([PANNEAU_L, 6 - EPAISSEUR, PANNEAU_H]);

    // La decoupe, traversante.
    translate([CX, Y_EXT - 20, CZ])
      prisme_arrondi(DECOUPE_L + JEU, DECOUPE_H + JEU, DECOUPE_R, 25);

    percages_nfc();
  }
}

// Le gabarit est une tranche du corps repris, prise autour de la decoupe : meme
// epaisseur, meme congés, meme sens d'impression. Dix minutes de PLA pour
// savoir si la gorge prend, au lieu de dix heures de boitier.
//
//   openscad -D GABARIT=true --export-format binstl -o gabarit.stl corps-prise-usb.scad
GABARIT = false;

// --- les plots du lecteur NFC
//
// Releves dans le maillage : deux paves de 12 x 12 mm qui pendent du plafond,
// hauts de 3 mm, et **pleins**. Aucun avant-trou, d'ou la matiere qui part
// partout quand on y fond un insert, et 3 mm de prise pour un insert qui en
// fait 5.
//
// On perce donc, et on allonge le plot juste ce qu'il faut pour que l'insert
// tienne entier dedans. Attention : allonger le plot descend le module
// d'autant, donc eloigne l'antenne de la figurine. Avec INSERT_L = 3 on garde
// la geometrie d'aujourd'hui, celle dont on sait qu'elle lit a travers les
// 4 mm du plafond. Avec 5, le module descend de 3,5 mm et il faut verifier la
// portee avant de refermer.
INSERT_L  = 3;                       // longueur de l'insert choisi
INSERT_D  = 3.5;                     // diametre exterieur
PERCAGE_D = INSERT_D - 0.3;          // un peu plus petit : le laiton s'y ancre

PLOTS_NFC = [[197, 48], [223, 72]];  // centres releves dans le maillage
PLOT_Z    = 73;                      // dessous des plots d'origine
PLAFOND_Z = 76;                      // dessous du plafond
PERCAGE_P = INSERT_L + 1;            // la reserve recoit la matiere repoussee
// Le percage a le droit de mordre 1,5 mm dans le plafond, qui en fait 4 : au
// dela on allonge le plot plutot que d'amincir la zone ou la figurine se pose.
PLOT_H    = max(3, PERCAGE_P - 1.5);

module plots_nfc() {
  for (p = PLOTS_NFC)
    translate([p[0] - 6, p[1] - 6, PLAFOND_Z - PLOT_H])
      cube([12, 12, PLOT_H]);
}

module percages_nfc() {
  for (p = PLOTS_NFC)
    translate([p[0], p[1], PLAFOND_Z - PLOT_H - 0.1])
      cylinder(h = PERCAGE_P + 0.1, d = PERCAGE_D, $fn = 48);
}

if (GABARIT)
  intersection() {
    corps_repris();
    translate([CX - 17, Y_EXT - 7, CZ - 11]) cube([34, 8, 22]);
  }
else
  corps_repris();
