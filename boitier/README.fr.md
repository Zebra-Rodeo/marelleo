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

## La prise USB-C de panneau

`corps-prise-usb.scad` reprend le corps pour une prise USB-C femelle a
encliqueter, celle qui evite d'ouvrir la boite pour la recharger. L'ouverture
d'origine est un ovale de 9 x 3,5 mm taille pour laisser passer une fiche ; une
prise de panneau demande une decoupe cotee et une epaisseur de paroi precise,
celle que sa gorge vient pincer.

Le script comble la poche d'origine, ramenee a 1 mm, porte la paroi a
l'epaisseur voulue sur toute la surface du collet, et y perce la decoupe du
fabricant, 13,6 x 6,3 mm a coins R1,3.

```sh
openscad --export-format binstl -o marelleo-corps-usb.stl corps-prise-usb.scad
openscad -D GABARIT=true --export-format binstl -o gabarit.stl corps-prise-usb.scad
```

`EPAISSEUR` est le seul chiffre a revoir : c'est la largeur de la gorge, a
mesurer au pied a coulisse sur la prise recue. Le gabarit est une tranche de
34 x 18 mm prise autour de la decoupe, meme epaisseur et meme sens
d'impression : dix minutes de PLA pour savoir si la gorge prend, au lieu de dix
heures de boitier.
