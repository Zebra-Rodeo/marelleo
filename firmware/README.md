# Le micrologiciel

**Ecrit pour ce montage-la.** Le brochage vit dans le code, en haut de
`src/main.cpp`, et les deux environnements decrivent ma carte imprimee et mon
prototype sur modules. Un montage cable autrement demande d'y changer les
constantes correspondantes : sans ca, il ne marchera pas, et pas toujours
bruyamment. Le lecteur NFC muet et la carte SD introuvable sont les deux
symptomes les plus frequents d'une broche qui n'est pas la ou le code la
croit.

C'est aussi un projet du dimanche, pas un produit. Il tourne tous les jours
chez moi, ce qui fait un echantillon d'une boite. Prenez-le comme un point de
depart qui marche.

Deux environnements, un seul code.

```sh
pio run -e boite -t upload     # modules sur breadboard ou plaque a pastilles
pio run -e carte -t upload     # carte imprimee
pio device monitor
```

| | `boite` | `carte` |
|---|---|---|
| Module | ESP32-S3-DevKitC-1 N16R8 | ESP32-S3-WROOM-1-N8R2 |
| Flash | 16 Mo | 8 Mo |
| PSRAM | 8 Mo octale | 2 Mo quad |
| Console serie | puce CH343, survit aux resets | USB natif du processeur |

Se tromper d'environnement donne une carte qui ne demarre pas du tout, sans
une ligne sur la console. Les deux pieges sont expliques dans
`platformio.ini`, au-dessus des reglages concernes.

## Ou se trouve quoi

| Fichier | Role |
|---|---|
| `main.cpp` | la boucle, la lecture, le volume, l'extinction |
| `library.cpp` | la table figurine vers pistes, resistante aux coupures |
| `serveur.cpp` | l'API et la page web |
| `page.h` | toute l'interface, dans un seul litteral |
| `reseau.cpp` | le WiFi, et le portail quand aucun reseau n'est connu |
| `maj.cpp` | la mise a jour par le reseau, avec retour arriere |
| `pn532_hsu.cpp` | le lecteur NFC en liaison serie |
| `batterie.cpp` | la tension, traduite en une jauge qui ne remonte pas |
| `carillon.cpp` | les deux notes, fabriquees echantillon par echantillon |

La page web est compilee dans le micrologiciel : aucune ressource exterieure,
puisque la boite doit savoir repondre sans acces a internet.

## Travailler l'interface sans la boite

Le depot prive porte une boite fictive en Python qui sert la page extraite de
`page.h` et repond a son API. Elle n'est pas publiee ici : elle depend de
fichiers d'essai qui n'ont d'interet que pour moi. La page se teste aussi bien
sur une vraie boite, qui repond en une seconde.
