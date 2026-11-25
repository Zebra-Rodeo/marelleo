# Le boitier

[English](README.md) · **Français**

Trois pieces, PLA, imprimees sur une Bambu Lab A1.

| Fichier | Taille | Reglages |
|---|---|---|
| `marelleo-corps.stl` | 120 x 120 x 80 mm | face ouverte vers le haut, aucun support |
| `marelleo-base.stl` | 120 x 120 x 29 mm | a plat |
| `marelleo-molette.stl` | 24 x 24 x 12 mm | a plat, 100 % de remplissage |

Buse 0,4 mm, couches de 0,2 mm, trois parois, 15 % de remplissage.

Le cercle grave sur le dessus dit ou poser la figurine, parce que l'antenne du
lecteur NFC est juste dessous. Sa portee traverse sans difficulte 3 mm de PLA.

**Inserts en laiton M3**, pas de vis autoforeuses. Visser dans du plastique
imprime tient une fois ou deux, puis la matiere se fend le long des couches.
L'insert se pose sur le trou, on vient dessus avec la panne du fer a souder, le
laiton chauffe, fond le plastique autour de lui et s'y noie. En refroidissant,
on a un pas de vis metal pris dans la piece.

Le modele a ete dessine sur Fusion 360 en licence loisir, qui est gratuite.

## Les reprises

Deux scripts OpenSCAD reprennent les pieces imprimees a partir des STL, sans
passer par Fusion. Les cotes qu'ils utilisent ont toutes ete relevees dans le
maillage, pas retrouvees de memoire.

```sh
openscad --export-format binstl -o marelleo-corps-v2.stl corps-reprise.scad
openscad --export-format binstl -o marelleo-base-v2.stl  base-reprise.scad
openscad -D GABARIT=true --export-format binstl -o gabarit.stl corps-reprise.scad
```

**`base-reprise.scad` : les quatre trous du raft.** Ils faisaient 0,95 mm pour
des inserts M2,5 de 3,5 mm de diametre exterieur. L'insert devait deplacer
presque toute la matiere qu'il rencontrait, et elle remontait dans le pas de
vis. Perces a 3,2 mm, profondeur d'origine conservee, 6,5 mm pour un insert de
5 : le millimetre et demi du fond sert de reserve a la matiere repoussee.

**`corps-reprise.scad` : la prise USB-C et les plots du lecteur NFC.**

L'ouverture USB d'origine est un ovale de 9 x 3,5 mm taille pour laisser passer
une fiche. Une prise de panneau demande une decoupe cotee, 13,6 x 6,3 a coins
R1,3, et une epaisseur de paroi precise, celle que sa gorge pince. Au droit de
l'ancienne ouverture la paroi avait ete creusee a 1 mm, ailleurs elle fait
4 mm : le script comble la poche, ramene toute la surface du collet a
`EPAISSEUR`, et perce la decoupe.

Les deux plots du lecteur NFC, 12 x 12 x 3 mm, etaient pleins : aucun
avant-trou, d'ou la matiere qui part partout quand on y fond un insert, et
3 mm de prise pour un insert qui en fait 5. Ils sont perces a 3,2 mm, et
allonges seulement si l'insert choisi l'exige. `INSERT_L` vaut 3 par defaut,
ce qui laisse le module exactement ou il est : allonger les plots eloigne
l'antenne de la figurine, et c'est la portee du lecteur qui paie.

`EPAISSEUR` et `INSERT_L` sont les deux chiffres a revoir selon les pieces que
tu as. Le gabarit est une tranche de 34 x 18 mm prise autour de la decoupe,
meme epaisseur et meme sens d'impression : dix minutes de PLA pour savoir si
la gorge prend, au lieu de dix heures de boitier.
