# La carte, pour une série

Cible : 50 exemplaires chez JLCPCB, carte 70 × 60 mm, 4 couches, composants sur
une seule face pour ne payer qu'une préparation et un pochoir.

**Le brochage est identique à celui du prototype.** Le micrologiciel actuel
tourne sans une ligne de changement, ce qui enlève une inconnue à un moment où
il y en a déjà assez.

## Les rails

| Rail | D'où il vient | Ce qu'il alimente |
|---|---|---|
| `VBUS` 5 V | prise USB-C | le chargeur, uniquement |
| `VSYS` 3,0 à 4,2 V | chargeur à chemin d'alimentation | l'abaisseur et l'élévateur |
| `+5V` | élévateur depuis `VSYS` | l'amplificateur, et l'abaisseur ci-dessous |
| `+3V3` | abaisseur **depuis le 5 V** | module, carte SD, lecteur NFC |

Le chemin d'alimentation compte : il permet de jouer pendant la charge sans
perturber la détection de fin de charge, ce que le montage actuel ne sait pas
faire proprement.

**Le 3,3 V se fabrique à partir du 5 V, et non de la batterie.** Une batterie
vide descend à 3,0 V, sous les 3,3 V voulus, et un abaisseur ne sait pas
remonter une tension : la boîte s'éteindrait avec de l'énergie encore dedans.
Partir du 5 V garantit une entrée toujours plus haute que la sortie. L'élévateur
tourne donc tant que la boîte est allumée, ce qui est sans importance ici.

## L'extinction, repensée

Aujourd'hui l'interrupteur coupe au milieu d'une écriture, d'où la sauvegarde
résistante aux coupures dans `library.cpp`. Sur la carte, un **verrou** :

1. l'interrupteur du potentiomètre met la broche `SYSOFF` du chargeur à la
   masse, ce qui ouvre son rail de sortie et démarre la boîte ;
2. le processeur démarre et maintient lui-même cette broche basse, non pas en
   la touchant, mais **par la grille de Q1** commandée par `IO21` ;
3. à l'extinction, il termine ses écritures, démonte la carte SD, puis lâche la
   grille. R12, 1 MΩ vers VBAT, relève alors `SYSOFF` et la boîte s'éteint.

C'est une fonction prévue par le chargeur, pas un montage de fortune.

**Le transistor n'est pas un ornement.** `SYSOFF` vit sur la tension batterie,
4,2 V, au-delà des 3,6 V que l'ESP32 accepte sur une broche. Et une broche non
alimentée ramène par ses diodes de protection le nœud à une diode de la masse,
c'est-à-dire à un niveau bas, c'est-à-dire à une boîte qui se rallume seule.
Q1 isole les deux mondes : le processeur ne voit que du 3,3 V, et lâcher la
grille éteint pour de bon.

Plus aucun fichier tronqué possible, et le bouton devient une demande plutôt
qu'un ordre.

**Rien de tout cela n'est encore écrit dans le micrologiciel.** Il n'utilise à
ce jour ni `IO21` ni `IO2`. Le matériel porte le verrou, le logiciel ne s'en
sert pas : la boîte s'allume et reste allumée tant que l'interrupteur est
fermé. À écrire, et cela se livrera par le réseau.

## Le module, broche par broche

| Broche | Va vers | Pourquoi |
|---|---|---|
| `IO4` | ampli `SD_MODE` | coupe l'ampli au repos |
| `IO5` `IO6` `IO7` | ampli `DIN` `BCLK` `LRC` | liaison audio I2S, ordre permuté |
| `IO8` `IO9` | connecteur NFC, 4 points | liaison série vers le PN532 déporté |
| `IO10` | curseur du potentiomètre | ADC1 obligatoire, l'ADC2 est muet en WiFi |
| `IO11` `IO12` `IO13` `IO14` | carte SD `CS` `MOSI` `CLK` `MISO` | SPI, sans tampon, `CLK` et `MOSI` permutés |

**Cinq signaux ne sont pas au même endroit que sur le prototype.** Leur ordre a
été choisi en essayant toutes les orientations de l'amplificateur et toutes les
permutations de broches, puis en comptant les croisements de pistes sur la
géométrie réelle : on passe de quatre croisements à zéro, sans allonger le
cuivre. Le micrologiciel porte les deux brochages, celui du prototype par
défaut, celui de la carte sous `-DCARTE_IMPRIMEE`, soit l'environnement `carte`
de `platformio.ini`.
| `IO18` | bouton | rappel interne, pas de résistance |
| `IO48` | LED d'état | déjà câblé ainsi |
| `IO1` | pont diviseur sur `VSYS` | niveau de batterie, ce qui manque aujourd'hui |
| `IO21` | `SYSOFF` du chargeur | le verrou d'extinction |
| `EN` | `R18` 10 kΩ vers le 3,3 V et `C13` 1 µF vers la masse | la remise à zéro du module |
| `IO0`, `RXD0`, `TXD0` | pastilles de test `TP1` à `TP3` | programmation en usine et console |
| `IO19` `IO20` | USB-C `D-` `D+` | USB natif, pour la mise à jour en usine |
| `IO43` `IO44` | pastilles de test | console série, sans puce dédiée |
| `IO0`, `EN` | pastilles de test | démarrage et remise à zéro |

## Les connecteurs, tous démontables

La carte est au fond du boîtier, le lecteur NFC sous le couvercle : tout ce qui
n'est pas sur la carte s'enfiche.

| Repère | Points | Vers |
|---|---|---|
| `J2` | 4 | module PN532 : `3V3`, masse, `RX`, `TX` |
| `J3` | 2 | haut-parleurs, 4 Ω |
| `J4` | 3 | potentiomètre de volume |
| `J5` | 2 | bouton |
| `J6` | 2 | batterie, JST-PH 2,0 |

## Ce que la carte SD gagne

Plus de module, plus de tampon `74LVC125`, plus de 5 V détourné : un connecteur
microSD en 3,3 V, des résistances de 22 Ω en série sur l'horloge et les données,
des rappels de 10 kΩ, et vingt millimètres de piste au lieu de vingt
centimètres de fil. C'est la cause racine de la moitié des pannes du prototype.

## Contraintes de placement, non négociables

- **Zone sans cuivre** sous l'antenne du module, au bord de la carte, sur les
  quatre couches.
- **Découpages et amplificateur** dans un coin, avec leurs condensateurs, loin
  du module et de la liaison NFC.
- **Condensateur de 470 µF collé à l'amplificateur**, la leçon mesurée au banc :
  cinq crans de seuil de distorsion gagnés.
- Prise USB-C **vissée à la paroi** et reliée par une nappe, pas accessible par
  un trou, sous peine d'arracher le connecteur à la première chute.

## Les valeurs, vérifiées une par une

Fiches techniques consultées le 23 septembre 2026. Les huit valeurs que j'avais
posées de tête portaient un point d'interrogation ; en voici le sort.

| Repère | Rôle | Valeur | Vérification |
|---|---|---|---|
| R1 / R2 | règlent le 5 V de l'élévateur | 75 k et 10 k | référence 0,6 V confirmée, 0,6 × (1 + 75/10) = **5,10 V** |
| R4 / R5 | réglaient le 3,3 V | **0 Ω et rien** | l'AP63203 est à sortie fixe, voir ci-dessous |
| R7 | courant de charge | 1 k | 890 / 1000 = **890 mA**, dans la plage 590 Ω à 5,9 k |
| R8 | courant tiré de l'USB | 1,5 k | 1550 / 1500 = **1,03 A**, jamais plus d'un ampère au chargeur |
| R9 | sonde de température inutilisée | 10 k à la masse | c'est la valeur que demande la fiche pour désactiver la sonde |
| R13 | gain de l'amplificateur | **rien** | voir ci-dessous |
| L1 / L2 | bobines des deux découpages | 4,7 µH | plage admise 2,2 à 10 µH, ondulation calculée à 28 % du courant |

**L'erreur qui aurait tué les cartes.** L'AP63203 existe en deux familles : les
AP63200 et AP63201 à tension réglable, et l'**AP63203 à sortie fixe de 3,3 V**.
J'avais choisi le second et câblé le premier. Ma broche FB aurait vu 0,79 V au
lieu des 3,3 V qu'elle attend, la boucle de régulation aurait conclu que la
sortie était trop basse et poussé jusqu'à **13,7 V** sur le rail 3,3 V : module,
carte SD et lecteur NFC détruits à la première mise sous tension, sur les
cinquante cartes. La correction ne coûte rien à la carte : R4 devient un fil de
0 Ω, R5 n'est pas montée, le cuivre ne bouge pas.

**Le gain de l'amplificateur.** La table du MAX98357A : 100 k vers la masse
donne 15 dB, un fil vers la masse 12 dB, **broche en l'air 9 dB**, un fil vers
le 3,3 V 6 dB. J'avais mis 100 k, c'est-à-dire le maximum. Le prototype, lui,
joue à 9 dB depuis le début : c'est ce volume-là qui a été réglé à l'oreille et
plafonné dans le micrologiciel. La carte garde donc 9 dB, en ne montant pas
R13. La pastille reste en place si on veut essayer plus fort un jour, mais pour
un objet posé près de la tête d'un enfant, monter de 6 dB ne se décide pas par
défaut.

**Trois valeurs changées après coup, pour une raison de catalogue.** Mes
73,2 k, 910 Ω et 1,02 k sont des valeurs de la série E96, justes au calcul mais
absentes du stock courant du fabricant : chacune aurait été facturée comme
pièce spéciale. Les remplaçantes sont des valeurs de stock, et le résultat ne
souffre pas : 5,10 V au lieu de 4,99, une charge à 890 mA au lieu de 978, et une
limite d'entrée ramenée de 1,52 A à **1,03 A**, ce qui est plutôt un gain. La
boîte ne demandera jamais plus d'un ampère à un chargeur, ce que le moindre
adaptateur qui traîne sait fournir, et le chargeur ralentit tout seul si
l'entrée faiblit.

**Le sens des pièces.** La table des pièces de base du fabricant donne une
correction de rotation de 180 degrés pour **deux** de nos composants, la SS34 et
le 2N7002 : leur zéro n'est pas celui de KiCad. C'est noté dans
`composants.csv`, et c'est à confirmer sur leur vue de contrôle avant
production. Une diode à l'envers sur l'élévateur, ce sont cinquante cartes qui
ne démarrent pas.

**Les bobines, et leur empreinte.** L1 voit passer le courant d'entrée de
l'élévateur, environ 0,6 A en moyenne et des pointes à 1,5 A quand le WiFi émet
pendant un coup de basse. La pièce retenue, `C206284`, est donnée pour **2 A de
saturation et 2,9 A efficaces**, avec 60 mΩ de résistance. La marge est là.

Restait son empreinte, la dernière incertitude de la nomenclature. Sa fiche
donne le plan de pose recommandé, à confronter au nôtre :

| | recommandé | notre empreinte | écart |
|---|---|---|---|
| longueur de pastille | 3,7 mm | 3,7 mm | aucun |
| largeur de pastille | 1,1 mm | 1,2 mm | +0,1 |
| écart entre pastilles | 1,9 mm | 1,6 mm | −0,3 |
| bord extérieur depuis le centre | 2,05 mm | 2,00 mm | −0,05 |

L'électrode de la pièce fait 0,95 mm de large et va du rayon 1,05 mm au bord du
corps, à 2,00 mm. Notre pastille couvre de 0,80 à 2,00 mm : **l'électrode repose
entièrement sur le cuivre**. Les 0,25 mm de pastille qui dépassent vers
l'intérieur passent sous la résine isolante du composant, sans conséquence, et
1,6 mm d'écart interdit tout pont de soudure. Sur le bord extérieur, celui qui
fait la soudure, l'écart au plan recommandé est de 0,05 mm.

**L'empreinte convient, elle ne sera pas modifiée.**

## La relecture de septembre, et ce qu'elle a corrigé

Trois défauts ont été trouvés à la relecture, dont deux empêchaient purement et
simplement la carte de fonctionner. Tous vérifiés contre le fichier et contre
les fiches techniques avant correction.

**La diode de l'élévateur était à l'envers.** Chez KiCad, la pastille 1 d'une
diode est la **cathode**, et le symbole dessiné sur la couche fabrication le
montre : la barre est du côté de la pastille 1, le triangle du côté de la
pastille 2. Le fichier portait `D1: (SW_5V, +5V)`, donc cathode sur le nœud de
commutation. Une diode dans ce sens ne laisse jamais monter la sortie : pas de
5 V, donc pas de 3,3 V puisqu'il en descend, donc rien ne démarre, et le MT3608
prend la surtension de son propre nœud. D1 a été tournée de 180 degrés, ce qui
échange la position des deux pastilles sans déplacer un seul millimètre de
cuivre, les pastilles étant symétriques à ±2 mm.

**La boîte ne pouvait pas s'éteindre.** La fiche TI, table 7-1 : « SYSOFF is
internally pulled up to V_BAT through a large resistor (approximately 5 MΩ) ».
Le rappel interne va donc vers le **haut**. Notre R12 de 100 k était branchée
sur VSYS, c'est-à-dire sur le rail que SYSOFF coupe : dès l'extinction, VSYS
tombe à zéro et R12 tire SYSOFF vers la masse contre 5 MΩ, soit 0,08 V, soit un
niveau bas franc, soit la boîte rallumée. Et IO21, posée directement sur ce
nœud, y voyait 4,2 V là où l'ESP32 en admet 3,6.

La correction tient en deux pièces :

- **R12 passe à 1 MΩ vers VBAT**, le seul rail présent quand la boîte est
  éteinte. 4 µA de fuite, et un rappel franc.
- **Q1, un 2N7002**, s'intercale : grille sur `ARRET_CMD` que commande IO21,
  drain sur SYSOFF, source à la masse, avec R19 de 100 k en rappel de grille.
  Le processeur ne voit plus jamais la tension batterie, et lâcher la grille
  éteint la boîte.

**La charge était limitée à 500 mA.** Table 7-2 de la même fiche : `EN2=0,
EN1=1` est le mode **USB500**, et la résistance ILIM n'est lue que pour
`EN2=1, EN1=0`. R8, choisie pour 1,03 A, ne servait à rien. Les deux broches ont
été échangées.

**Un quatrième défaut, dans l'outillage.** Le projet portait `min_clearance` à
**zéro** : le contrôle des règles ne vérifiait aucun isolement. La classe de
réseau imposait 0,2 mm et c'est elle qui a fait le travail, vérifié en refaisant
le contrôle à 0,2 puis 0,25. La règle est maintenant à 0,15 mm.

**Ce qui reste, et qui n'est pas corrigé ici :** VSYS court sur 71 mm en 0,2 mm
sans aucun plan pour l'aider, alors qu'il porte le courant d'entrée de
l'élévateur. Le calcul IPC-2221 donne +50 °C et 0,26 V de chute à 1,5 A. À
élargir à 0,8 mm au routage, comme `SW_5V` et `SW_3V3` qui sont des nœuds de
commutation.

## Nomenclature

**Les références vivent dans `composants.csv`**, une ligne par repère, avec pour
chacune sa famille chez le fabricant. Douze pièces de base, douze spéciales.
`pcb/stock.py` interroge leur catalogue et dit, pour chaque référence, le stock
restant et le prix à la quantité qui nous intéresse.

Prix relevés le 23 septembre 2026, pour 50 cartes :

| Poste | Par carte |
|---|---|
| les cinq puces | 6,42 $ |
| les huit connecteurs | 2,70 $ |
| tout le reste, passifs et bobines | 1,35 $ |
| **total des composants** | **10,48 $** |

Deux surprises dans ces chiffres. Le module coûte **3,65 $** à lui seul, plus du
tiers du total. Et les deux connecteurs J1 et J7, la prise USB-C et le porte-carte
microSD, coûtent ensemble **2,44 $**, plus cher que les quatre autres puces
réunies. Si un jour il faut gagner deux dollars par carte, c'est là qu'ils sont,
pas dans les résistances.

Ce montant ne couvre que **ce qui est posé sur la carte**. La batterie, le
haut-parleur, le potentiomètre, le bouton, le lecteur NFC, la carte SD, les
figurines et leurs pastilles s'ajoutent, comme le boîtier imprimé.

Les cinq puces ont été retenues parce que **KiCad possède leur symbole** : leur
brochage se lit dans la bibliothèque au lieu d'être recopié, et aucun numéro de
broche n'est écrit à la main dans `generer.py`.

Un point à vérifier sur la fiche technique avant de produire : la pastille
thermique sous le BQ24075 mesure 1,1 × 1,1 mm dans l'empreinte de KiCad, à
confronter à la cote donnée par le fabricant.

## Deux ruptures, et ce qu'elles apprennent

Au premier contrôle de stock, deux références sur vingt-quatre étaient à zéro :
le condensateur de 470 µF et le connecteur JST à quatre points. Remplacées par
`C82004`, même boîtier de 6,3 × 7,7 mm mais **tenue 10 V au lieu de 6,3**, ce qui
est la bonne marge sur un rail à 5 V, et par `C157926`, le même JST d'origine
sous une autre fiche.

La leçon tient en une ligne : **une référence n'est pas un acquis**. Le stock se
revérifie le jour de la commande, `python3 pcb/stock.py`, et pas avant. La plus
juste aujourd'hui est justement ce condensateur, avec 2 956 pièces pour les 50
qu'il nous faut.

## Validée au banc, le 15 octobre 2026

Une des cinq cartes reçues a été montée avec ses six faisceaux et **tout
fonctionne** : les rails 3,3 V et 5 V, le chemin d'alimentation du chargeur, la
carte SD à 29,8 Go, le lecteur PN532 en liaison série, le potentiomètre sur
toute sa course, le bouton, le décodage AAC dans le haut-parleur, le WiFi, le
portail captif, la mise à jour depuis `maj.marelleo.org`, le fonctionnement sur
batterie et l'extinction au clic de butée basse.

Trois incertitudes de ce document sont donc levées par la mesure plutôt que par
la lecture d'une fiche :

- **Les deux corrections de rotation étaient bonnes.** La SS34 et le 2N7002
  avaient un zéro différent de celui de KiCad. Si D1 avait été posée à l'envers,
  l'élévateur ne sortirait pas de 5 V et rien ne démarrerait ; si Q1 l'avait
  été, le verrou d'extinction ne tiendrait pas. Les deux marchent.
- **La pastille thermique du BQ24075 suffit.** La charge à 890 mA tient sans
  que la puce se mette en repli thermique.
- **L'empreinte de L1 tient**, sous les pointes à 1,5 A de l'élévateur.

Et une absence confirmée : **les résistances de 22 Ω en série sur le bus de la
carte SD ne sont pas nécessaires.** Elles n'ont pas été posées, et la carte
monte et se lit sans une erreur.

**Ce qui reste ouvert.** Le micrologiciel ne pilote jamais `IO21`, donc
l'extinction *commandée par le logiciel* à travers Q1 existe en matériel et
n'est pas éprouvée ; seul le clic du potentiomètre l'est. Et LittleFS n'est pas
formaté, donc une carte livrée sans carte SD n'aurait aucun stockage.

## Deux pièges de mise en route, qui ne sont pas des défauts de la carte

Les deux seules pannes de la mise en route venaient du câblage, et chacune
coûte une heure à qui ne les connaît pas.

**Le cordon du lecteur NFC se croise.** Sur le module V3, les broches partagées
portent `SDA/TXD` et `SCL/RXD` : `SDA` est ce que le module **émet**, donc il va
sur `NFC_RX` de la carte, et `SCL` sur `NFC_TX`. Branché droit, RX sur RX, le
module s'allume, sa LED clignote, et il ne répond jamais.

**Et `nfc.begin()` ne tourne qu'une fois, dans `setup()`.** Réparer le cordon ne
sert à rien sans un vrai redémarrage. Or **l'interrupteur du potentiomètre ne
coupe pas la carte quand l'USB est branché** : `SYSOFF` ne commande que le
transistor de la batterie, et l'entrée alimente VSYS de son côté. Pour
redémarrer il faut retirer l'USB, pas tourner le bouton.

## Deux corrections pour la prochaine carte

**Le pont de mesure est sur le mauvais rail.** `IO1` lit VSYS, c'est-à-dire la
sortie du chargeur. Or le chemin d'alimentation relie l'entrée à la sortie dès
qu'une source externe est présente : mesuré sur la carte, USB branché, VSYS vaut
**5,01 V**. La tension de la cellule devient donc invisible pendant toute la
charge, et c'est précisément le moment où l'on regarde la jauge. Le logiciel s'en
tire en affichant « sur secteur » et le dernier niveau connu, mais c'est un
pansement.

La correction est d'un trait : **déplacer le pont de VSYS vers VBAT**. La cellule
devient alors lisible en permanence. Deux précautions : des résistances de 1 MΩ
plutôt que 100 kΩ, pour que le pont ne vide pas la batterie quand la boîte est
éteinte, et se souvenir qu'une cellule en charge lit haut, comme sur n'importe
quel téléphone.

**`~CHG` n'est relié à rien.** C'est la seule broche du chargeur laissée libre.
Routée vers une entrée du processeur, elle dirait si la charge est en cours ou
terminée, ce qu'aucune mesure de tension ne donne. `IO15`, `IO16` et `IO17` sont
disponibles.

## SYSOFF suspend aussi la charge

Mesuré sur la carte, USB branché, aux bornes de J6 :

| Position | Tension | Ce qui se passe |
|---|---|---|
| molette haute | 3,29 V | la boîte charge, et consomme en même temps |
| molette basse | 3,11 V | **la charge est suspendue** |
| molette basse, verrou tenu par `IO21` | 3,47 V | elle charge sans rien consommer |

`SYSOFF` ne se contente donc pas de déconnecter la batterie du système : il
suspend la charge. Une boîte éteinte à l'interrupteur ne se recharge pas, ce que
personne n'attend d'un objet qu'on pose sur son chargeur avant d'aller dormir.

La carte portait déjà la réponse sans qu'on s'en serve : **`Q1`, grille sur
`IO21`, tire `SYSOFF` à la masse en parallèle de l'interrupteur**. Le
micrologiciel tient donc le verrou lui-même tant que l'USB est présent, et le
relâche dès qu'il disparaît, sans quoi la boîte ne saurait plus s'éteindre du
tout et viderait sa cellule.

C'est la première utilisation du verrou d'extinction, posé au schéma dès le
début et resté inerte jusqu'au 15 octobre 2026.

### Le verrou sert aussi à dire au revoir

Sur batterie, le déclic de l'interrupteur coupe le courant immédiatement : la
boîte n'a pas le temps de sonner, et jusqu'au 16 octobre elle mourait au milieu
d'une phrase, sans rien dire. Le même verrou donne la fenêtre qui manquait.

Le micrologiciel **tient le verrou dès que la molette touche le bas**, avant le
déclic, qui arrive quelques degrés plus loin. `SYSOFF` est alors déjà à la masse
par `Q1` : l'interrupteur peut s'ouvrir, rien ne bouge. La boîte joue son
carillon à l'envers, relâche le verrou, et c'est là que le courant tombe, si
l'interrupteur est bien ouvert. Sinon la molette n'était qu'au minimum, et la
boîte redémarre proprement.

La fenêtre est courte à dessein, une seconde et demie plus la durée du
carillon. Un micrologiciel bloqué pendant qu'elle est ouverte garderait la
batterie connectée alors que la molette est éteinte, et viderait la cellule en
une nuit. Hors de cette fenêtre, le verrou est relâché et le matériel reste seul
maître de l'alimentation, comme avant.

## Le chargeur chauffe, et c'est réglable

Mesuré au doigt sur la carte, batterie à 3,65 V, USB branché : le BQ24075 est
brûlant. Rien d'anormal, c'est un chargeur **linéaire**, qui dissipe la
différence entre l'entrée et la cellule :

```
(5,01 V − 3,65 V) × 0,89 A = 1,21 W
```

Soit 60 à 70 °C en surface sur un boîtier de 3 × 3 mm. C'est le pire moment,
l'écart étant maximal quand la batterie est basse ; à 4,2 V la dissipation tombe
à 0,7 W. La puce se protège seule en réduisant son courant pour rester sous
125 °C.

**Correction proposée : `R7` de 1 kΩ à 2 kΩ**, ce qui ramène le courant de charge
de 890 mA à 445 mA. La chaleur est divisée par deux et la charge passe de trois
à six heures. Pour un objet qu'on pose sur son chargeur le soir, six heures ne
coûtent rien, et une puce tiède plutôt que brûlante dans un boîtier fermé, à
côté d'une cellule lithium, dans une chambre d'enfant, les vaut largement.

À noter : avant que le micrologiciel ne tienne `SYSOFF` par `IO21`, la puce ne
chauffait pas parce que la charge était simplement suspendue. La chaleur est la
contrepartie du fait qu'elle se recharge enfin.
