# La carte

Quatre couches, 1,6 mm, 60 x 70 mm. Les fichiers sont prets a commander.

| Fichier | Role |
|---|---|
| `marelleo-gerbers.zip` | a deposer tel quel chez le fabricant |
| `fab/marelleo-bom.csv` | la nomenclature, au format JLCPCB |
| `fab/marelleo-cpl.csv` | les positions, au format JLCPCB |
| `composants.csv` | la meme liste, lisible, avec les references LCSC |
| `marelleo.kicad_pcb` | la carte elle-meme, pour la modifier |
| `generer.py` | le script qui la dessine : la carte est generee, pas cliquee |
| `stock.py` | verifie le stock LCSC le jour de la commande |
| `schema.md` | **a lire avant de commander** |

`schema.md` porte les choix, les mesures prises au banc et les pieges qui ont
coute des soirees : le chargeur qui suspend la charge quand on eteint la boite,
le module NFC qui clignote au lieu de demarrer, la diode dont la rotation est a
corriger de 180 degres dans la previsualisation de JLCPCB.

Cinq cartes montees sont revenues a environ 160 euros, soit 32 euros la carte,
composants poses.

![La carte, vue du dessus](apercu.png)
