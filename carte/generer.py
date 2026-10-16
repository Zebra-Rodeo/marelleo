#!/usr/bin/env python3
"""Construit la carte Marelleo : contour, fixations, placement des composants.

Le fichier .kicad_pcb est engendre par ce script plutot que dessine a la main,
pour qu'on puisse relire le placement comme du code, le rejouer, et le faire
evoluer sans cliquer. Le routage, lui, se fera dans l'editeur : c'est la partie
qui demande un oeil.

    /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/\
Versions/3.9/bin/python3 pcb/generer.py

Repere : origine en haut a gauche, X vers la droite, Y vers le bas, comme dans
KiCad. La carte fait 70 x 60 mm et se pose au fond du boitier, dont la cavite
mesure 106 x 106 mm.
"""

import os
import sys

import pcbnew

LARGEUR, HAUTEUR = 70.0, 60.0
RAYON_COIN = 3.0
LIBS = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"

# L'antenne du module deborde vers le haut de la carte. Rien ne doit se trouver
# dessous, sur aucune des quatre couches : ni cuivre, ni piste, ni plan de masse.
GARDE_ANTENNE = 8.0

# Reference, empreinte, position en mm, rotation, role
PIECES = [
    # --- le module, antenne contre le bord haut, centre
    ("U1", "RF_Module", "ESP32-S3-WROOM-1", 35.0, 14.0, 0, "ESP32-S3-WROOM-1-N8R2"),

    # --- l'audio, a gauche du module : ses signaux sortent du flanc gauche
    ("U5", "Package_DFN_QFN", "TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm",
     19.0, 15.0, 0, "MAX98357A"),
    ("J3", "Connector_JST", "JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal",
     5.0, 19.0, 270, "haut-parleurs"),

    # --- les liaisons vers le dessus de la boite, sur le bord gauche
    ("J2", "Connector_JST", "JST_PH_S4B-PH-K_1x04_P2.00mm_Horizontal",
     5.0, 28.0, 270, "vers le lecteur NFC deporte"),
    ("J4", "Connector_JST", "JST_PH_S3B-PH-K_1x03_P2.00mm_Horizontal",
     5.0, 39.0, 270, "potentiometre de volume"),
    ("J5", "Connector_JST", "JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal",
     5.0, 48.0, 270, "bouton"),
    ("J8", "Connector_JST", "JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal",
     44.0, 56.0, 0, "interrupteur du potentiometre"),

    # --- la carte SD sur le bord droit, en face de la rangee basse du module
    ("J7", "Connector_Card", "microSD_HC_Hirose_DM3AT-SF-PEJM5",
     57.0, 22.0, 90, "connecteur microSD"),

    # --- l'alimentation, en bande basse : charge a gauche, conversion a droite
    ("U2", "Package_DFN_QFN", "VQFN-16-1EP_3x3mm_P0.5mm_EP1.1x1.1mm",
     14.0, 44.0, 0, "BQ24075RGT, chargeur a chemin d'alimentation"),
    ("J6", "Connector_JST", "JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal",
     12.0, 56.0, 0, "batterie"),
    ("U4", "Package_TO_SOT_SMD", "SOT-23-6", 33.0, 44.0, 0, "MT3608, elevateur 5V"),
    ("U3", "Package_TO_SOT_SMD", "TSOT-23-6", 50.0, 44.0, 0, "AP63203WU, abaisseur 3V3"),
    # Le verrou d'extinction : IO21 commande cette grille au lieu de toucher
    # SYSOFF. 2N7002, brochage G-S-D en 1-2-3.
    ("Q1", "Package_TO_SOT_SMD", "SOT-23", 42.45, 50.5625, 90, "2N7002, verrou d'extinction"),
    ("J1", "Connector_USB", "USB_C_Receptacle_Amphenol_12401610E4-2A",
     30.0, 56.0, 0, "USB-C, charge et mise a jour"),
]

# Les passifs. Deux pastilles chacun, donc un brochage sans ambiguite : "1" et
# "2". Toutes les valeurs ont ete confrontees a la fiche technique de la puce
# concernee le 23 septembre 2026 ; "NP" veut dire pastilles laissees vides, la
# place est gardee au cas ou.
#
# R, C, L, D suivent la convention : resistance, condensateur, bobine, diode.
# Les pieces laissees vides a l'assemblage portent la valeur "NP". Le
# drapeau "do not populate" de KiCad a ete essaye : l'editeur de carte
# l'efface a la premiere sauvegarde. C'est donc pcb/composants.csv qui
# fait foi, colonne "pose".
PASSIFS = [
    # --- autour de l'amplificateur, colles a lui
    ("C11", "Capacitor_SMD", "CP_Elec_6.3x7.7", "470 uF", 19.0, 22.0, 0),
    ("C12", "Capacitor_SMD", "C_0603_1608Metric", "100 nF", 23.0, 13.0, 0),
    # GAIN en l'air = 9 dB, le reglage du prototype. Une 100 k vers la masse
    # monterait a 15 dB, six de plus, soit presque le double a l'oreille.
    ("R13", "Resistor_SMD", "R_0603_1608Metric", "NP", 23.5, 15.5, 0),

    # --- decouplage du module, sous sa rangee basse
    ("C9", "Capacitor_SMD", "C_0603_1608Metric", "100 nF", 38.0, 31.0, 0),
    ("C10", "Capacitor_SMD", "C_0805_2012Metric", "10 uF", 42.0, 31.0, 0),
    # --- rappels de la carte SD, entre le module et elle
    ("R16", "Resistor_SMD", "R_0603_1608Metric", "10 k", 48.0, 31.0, 0),
    ("R17", "Resistor_SMD", "R_0603_1608Metric", "10 k", 51.5, 31.0, 0),

    # --- le chargeur
    ("C6", "Capacitor_SMD", "C_0805_2012Metric", "10 uF", 10.0, 39.0, 0),
    ("C7", "Capacitor_SMD", "C_0805_2012Metric", "10 uF", 15.0, 39.0, 0),
    ("C8", "Capacitor_SMD", "C_0805_2012Metric", "10 uF", 20.0, 39.0, 0),
    ("R7", "Resistor_SMD", "R_0603_1608Metric", "1 k", 9.0, 48.0, 0),
    ("R8", "Resistor_SMD", "R_0603_1608Metric", "1,5 k", 13.0, 48.0, 0),
    ("R9", "Resistor_SMD", "R_0603_1608Metric", "10 k", 17.0, 48.0, 0),
    ("R10", "Resistor_SMD", "R_0603_1608Metric", "100 k", 9.0, 52.0, 0),
    ("R11", "Resistor_SMD", "R_0603_1608Metric", "100 k", 13.0, 52.0, 0),
    ("R12", "Resistor_SMD", "R_0603_1608Metric", "1 M", 17.0, 52.0, 0),
    ("R19", "Resistor_SMD", "R_0603_1608Metric", "100 k", 38.5, 51.5, 180),

    # --- l'elevateur 5 V : bobine, diode et condensateurs serres autour
    ("L1", "Inductor_SMD", "L_Taiyo-Yuden_MD-4040", "4,7 uH", 29.0, 39.0, 0),
    ("D1", "Diode_SMD", "D_SMA", "SS34", 36.0, 39.0, 0),
    ("C1", "Capacitor_SMD", "C_0805_2012Metric", "22 uF", 28.0, 48.0, 0),
    ("C2", "Capacitor_SMD", "C_0805_2012Metric", "22 uF", 32.0, 48.0, 0),
    ("R1", "Resistor_SMD", "R_0603_1608Metric", "75 k", 38.0, 44.0, 0),
    ("R2", "Resistor_SMD", "R_0603_1608Metric", "10 k", 36.0, 47.0, 0),
    ("R3", "Resistor_SMD", "R_0603_1608Metric", "100 k", 28.0, 44.0, 0),

    # --- l'abaisseur 3,3 V
    ("L2", "Inductor_SMD", "L_Taiyo-Yuden_MD-4040", "4,7 uH", 46.0, 39.0, 0),
    ("C3", "Capacitor_SMD", "C_0805_2012Metric", "10 uF", 53.0, 39.0, 0),
    ("C4", "Capacitor_SMD", "C_0805_2012Metric", "22 uF", 58.0, 39.0, 0),
    ("C5", "Capacitor_SMD", "C_0603_1608Metric", "100 nF", 53.0, 48.0, 0),
    # L'AP63203 sort 3,3 V par construction : sa broche FB veut la sortie
    # elle-meme, pas un pont diviseur. R4 devient un simple fil, R5 disparait.
    ("R4", "Resistor_SMD", "R_0603_1608Metric", "0 R", 47.0, 48.0, 0),
    ("R5", "Resistor_SMD", "R_0603_1608Metric", "NP", 50.0, 48.0, 0),
    ("R6", "Resistor_SMD", "R_0603_1608Metric", "100 k", 44.0, 44.0, 0),

    # --- les deux resistances que reclame une prise USB-C
    ("R14", "Resistor_SMD", "R_0603_1608Metric", "5,1 k", 23.0, 52.0, 0),
    ("R15", "Resistor_SMD", "R_0603_1608Metric", "5,1 k", 23.0, 49.0, 0),

    # --- la remise a zero du module. Sans ce couple, il demarre au hasard :
    # c'est la panne classique de ce composant, et elle avait ete oubliee.
    ("R18", "Resistor_SMD", "R_0603_1608Metric", "10 k", 22.0, 9.0, 0),
    ("C13", "Capacitor_SMD", "C_0603_1608Metric", "1 uF", 18.0, 9.0, 0),
]

# Pastilles de test, pour programmer une carte en usine et ecouter celle qui
# refuse de demarrer. Elles ne coutent rien et se revelent precieuses.
TESTS = [
    ("TP1", 47.5, 9.0, "TEST_IO0"),
    ("TP2", 51.5, 9.0, "TEST_RX"),
    ("TP3", 55.5, 9.0, "TEST_TX"),
    ("TP4", 59.5, 9.0, "GND"),
]

# L'ilot de 5 V, sur la couche interne, autour de l'amplificateur et de ses deux
# condensateurs. Il s'arrete avant la colonne gauche du module, dont la pastille
# 3,3 V est a x = 26,2.
ILOT_5V = [(14.0, 10.0), (25.5, 10.0), (25.5, 26.5), (14.0, 26.5)]

FIXATIONS = [(3.5, 3.5), (LARGEUR - 3.5, 3.5), (3.5, HAUTEUR - 3.5), (LARGEUR - 3.5, HAUTEUR - 3.5)]

SYMBOLES = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols"

# Le symbole de chaque puce, d'ou l'on lit son brochage. Aucun numero de broche
# n'est ecrit a la main dans ce fichier : ils sont tous resolus depuis ces
# symboles, qui font foi.
SYMBOLE_DE = {
    "U1": "ESP32-S3-WROOM-1",
    "U2": "BQ24075RGT",
    "Q1": "2N7002",
    "U3": "AP63203WU",
    "U4": "MT3608",
    "U5": "MAX98357A",
}

# Brochage des connecteurs, decide ici une fois pour toutes. Celui du module est
# lu dans la bibliotheque de KiCad, jamais recopie de memoire.
#
# J7, la carte SD : broches 1 a 8 selon la norme SD. Les broches 9 et 10 sont le
# contacteur de presence de carte, dont le brochage varie d'un fabricant a
# l'autre et que le micrologiciel n'utilise pas : laissees libres.
#
# J1, la prise USB-C : les noms des pastilles suivent la norme, A1 a A12 d'un
# cote, B1 a B12 de l'autre, SH pour le blindage.
# Le brochage de l'audio et de la carte SD n'est pas celui du prototype : cinq
# signaux ont ete permutes pour que leurs pistes ne se croisent pas, ce qui a ete
# calcule en essayant les orientations et les permutations possibles. Le
# micrologiciel a le brochage correspondant sous -DCARTE_IMPRIMEE.
LIAISONS = {
    "GND": [("U1", "GND"), ("U2", "VSS"), ("U3", "GND"), ("U4", "GND"),
            ("U5", "GND"), ("U5", "PAD"), ("J1", "A1"), ("J1", "A12"), ("J1", "B1"), ("J1", "B12"),
            ("J1", "SH"), ("J2", "4"), ("J4", "3"), ("J5", "2"), ("J6", "2"),
            ("J7", "6"), ("J7", "SH"), ("J8", "2"), ("U2", "EN1"), ("Q1", "S"), ("TP4", "1")],
    "+3V3": [("U1", "3V3"), ("J2", "1"), ("J4", "1"), ("J7", "4")],
    "VBUS": [("U2", "IN"), ("J1", "A4"), ("J1", "A9"), ("J1", "B4"), ("J1", "B9")],
    "VBAT": [("U2", "BAT"), ("J6", "1")],
    "USB_CC1": [("J1", "A5")],
    "USB_CC2": [("J1", "B5")],
    # Sur le module, ces deux broches portent le nom de leur fonction et non
    # celui d'une entree-sortie : ce sont IO19 et IO20 de la puce.
    "USB_D+": [("U1", "USB_D+"), ("J1", "A6"), ("J1", "B6")],
    "USB_D-": [("U1", "USB_D-"), ("J1", "A7"), ("J1", "B7")],
    "NFC_RX":  [("U1", "IO8"), ("J2", "2")],
    "NFC_TX":  [("U1", "IO9"), ("J2", "3")],
    "VOLUME":  [("U1", "IO10"), ("J4", "2")],
    "BOUTON":  [("U1", "IO18"), ("J5", "1")],
    "SD_CS":   [("U1", "IO11"), ("J7", "2")],
    "SD_CLK":  [("U1", "IO13"), ("J7", "5")],
    "SD_MOSI": [("U1", "IO12"), ("J7", "3")],
    "SD_MISO": [("U1", "IO14"), ("J7", "7")],

    # --- l'alimentation
    # Table 7-2 de la fiche, verifiee : EN2=1 et EN1=0 est le seul reglage qui
    # fait lire la resistance ILIM. EN2=0 avec EN1=1, ce que la carte portait,
    # c'est le mode USB500 a 500 mA et R8 ne sert alors a rien.
    "VSYS":  [("U2", "OUT"), ("U4", "IN"), ("U2", "EN2")],
    "+5V":   [("U3", "IN"), ("U5", "VDD")],
    # L'allumage : le bouton met SYSOFF a la masse, la boite demarre, puis le
    # processeur l'y maintient lui-meme par IO21. Il peut donc terminer ses
    # ecritures avant de se couper, ce que l'interrupteur actuel ne permet pas.
    # L'interrupteur du potentiometre tire cette broche a la masse : c'est lui
    # qui demande l'allumage. Il avait ete pose sur la carte sans etre declare,
    # donc ni le chevelu ni le controle ne pouvaient s'en plaindre.
    # IO21 ne touche plus SYSOFF : ce noeud monte a 4,2 V, au-dela des 3,6 V
    # admis par le processeur, et une broche non alimentee y ramenait la tension
    # a une diode de la masse, ce qui rallumait la boite qu'on venait d'eteindre.
    # Elle commande maintenant une grille, et lacher la grille eteint.
    "ARRET":        [("U2", "SYSOFF"), ("J8", "1"), ("Q1", "D")],
    "ARRET_CMD":    [("U1", "IO21"), ("Q1", "G")],
    "USB_PRESENT":  [("U2", "~{PGOOD}"), ("U1", "IO2")],
    "CHARGE_EN":    [("U2", "~{CE}")],
    "NIVEAU_BATTERIE": [("U1", "IO1")],

    # --- l'audio
    "I2S_BCLK":  [("U1", "IO6"), ("U5", "BCLK")],
    "I2S_LRCLK": [("U1", "IO7"), ("U5", "LRCLK")],
    "I2S_DIN":   [("U1", "IO5"), ("U5", "DIN")],
    "AMPLI_ARRET": [("U1", "IO4"), ("U5", "~{SD_MODE}")],
    "HP_PLUS":   [("U5", "OUTP"), ("J3", "1")],
    "HP_MOINS":  [("U5", "OUTN"), ("J3", "2")],
    "AMPLI_GAIN": [("U5", "GAIN_SLOT")],

    # --- les broches qui attendent leur resistance ou leur bobine
    "EN_5V":   [("U4", "EN")],
    "FB_5V":   [("U4", "FB")],
    "SW_5V":   [("U4", "SW")],
    "EN_3V3":  [("U3", "EN")],
    "FB_3V3":  [("U3", "FB")],
    "SW_3V3":  [("U3", "SW")],
    "BST_3V3": [("U3", "BST")],
    "TS":   [("U2", "TS")],
    "TMR":  [("U2", "TMR")],
    "ISET": [("U2", "ISET")],
    "ILIM": [("U2", "ILIM")],
    "RESET": [("U1", "EN"), ("R18", "1"), ("C13", "1")],
    "TEST_IO0": [("U1", "IO0"), ("TP1", "1")],
    "TEST_RX":  [("U1", "RXD0"), ("TP2", "1")],
    "TEST_TX":  [("U1", "TXD0"), ("TP3", "1")],
}

# Ce que chaque passif relie. Un passif a deux pastilles, "1" et "2".
LIAISONS_PASSIFS = {
    "L1": ("VSYS", "SW_5V"),
    # La pastille 1 d'une diode est la CATHODE chez KiCad : la barre est dessinee
    # de ce cote sur la couche fabrication. Dans un elevateur elle va a la sortie,
    # l'anode au noeud de commutation. L'inverse ne fabrique aucun 5 V, donc aucun
    # 3,3 V, donc rien ne demarre.
    "D1": ("+5V", "SW_5V"),
    "C1": ("VSYS", "GND"),            "C2": ("+5V", "GND"),
    "R1": ("+5V", "FB_5V"),           "R2": ("FB_5V", "GND"),
    "R3": ("EN_5V", "VSYS"),
    "L2": ("SW_3V3", "+3V3"),
    "C3": ("+5V", "GND"),             "C4": ("+3V3", "GND"),
    "C5": ("BST_3V3", "SW_3V3"),
    "R4": ("+3V3", "FB_3V3"),         "R5": ("FB_3V3", "GND"),
    "R6": ("EN_3V3", "+5V"),
    "C6": ("VBUS", "GND"),            "C7": ("VSYS", "GND"),
    "C8": ("VBAT", "GND"),
    "R7": ("ISET", "GND"),            "R8": ("ILIM", "GND"),
    "R9": ("TS", "GND"),
    "R10": ("VSYS", "NIVEAU_BATTERIE"), "R11": ("NIVEAU_BATTERIE", "GND"),
    # SYSOFF possede deja un rappel interne de 5 M vers VBAT. Le notre doit
    # venir de la meme source : branche sur VSYS, il tirait SYSOFF vers la masse
    # des que la boite s'eteignait, et la rallumait aussitot.
    "R12": ("ARRET", "VBAT"),
    # Rappel de grille : sans lui, lacher IO21 laisserait la grille en l'air.
    "R19": ("ARRET_CMD", "GND"),
    "C9": ("+3V3", "GND"),            "C10": ("+3V3", "GND"),
    "C11": ("+5V", "GND"),            "C12": ("+5V", "GND"),
    "R13": ("AMPLI_GAIN", "GND"),
    "R14": ("USB_CC1", "GND"),        "R15": ("USB_CC2", "GND"),
    "R18": ("RESET", "+3V3"),         "C13": ("RESET", "GND"),
    "R16": ("SD_MISO", "+3V3"),       "R17": ("SD_CS", "+3V3"),
}

# Ce qui reste a declarer, et pourquoi : l'amplificateur et les trois puces
# d'alimentation n'ont pas encore de reference precise, donc pas de brochage
# connu. Les cabler maintenant reviendrait a inventer des numeros de broches.
# Ce qui manque encore : les composants passifs. Bobines des deux convertisseurs,
# diode de l'elevateur, ponts diviseurs qui fixent 5 V et 3,3 V, resistances qui
# reglent le courant de charge, condensateurs de decouplage, et le 470 uF colle a
# l'amplificateur. Leurs valeurs viennent des fiches techniques.
# Tout est declare. Restent les valeurs marquees d'un ? dans PASSIFS, a
# confirmer sur les fiches techniques, et le routage.
A_DECLARER = ["valeurs marquees ?", "routage"]


def _bloc(texte, symbole):
    marque = '(symbol "%s"' % symbole
    if marque not in texte:
        return None
    debut = texte.index(marque)
    fin = texte.find('\n\t(symbol "', debut + 10)
    return texte[debut: fin if fin > 0 else len(texte)]


def broches_du_symbole(symbole):
    """Rend {nom de broche: [numeros]}, lu dans la bibliotheque de KiCad.

    Un symbole peut heriter d'un autre, ses broches sont alors chez le parent.
    """
    import glob
    import re
    for fichier in glob.glob(os.path.join(SYMBOLES, "*.kicad_sym")):
        texte = open(fichier, encoding="utf8").read()
        bloc = _bloc(texte, symbole)
        if bloc is None:
            continue
        parent = re.search(r'\(extends\s+"([^"]+)"', bloc)
        if parent:
            bloc = _bloc(texte, parent.group(1)) or bloc
        table = {}
        for nom, num in re.findall(r'\(name\s+"([^"]+)".*?\(number\s+"([^"]+)"', bloc, re.S):
            table.setdefault(nom, []).append(num)
        if table:
            return table
    return {}


def relier(board):
    """Pose les noms de liaisons sur les pastilles concernees.

    Rien n'est route a ce stade : KiCad affichera un trait fin par liaison a
    faire, le chevelu, qui disparait au fur et a mesure du routage.
    """
    tables = {ref: broches_du_symbole(sym) for ref, sym in SYMBOLE_DE.items()}
    poses, introuvables = 0, []
    complet = dict(LIAISONS)
    for ref, (reseau1, reseau2) in LIAISONS_PASSIFS.items():
        complet.setdefault(reseau1, []).append((ref, "1"))
        complet.setdefault(reseau2, []).append((ref, "2"))

    for nom_reseau, points in complet.items():
        reseau = pcbnew.NETINFO_ITEM(board, nom_reseau)
        board.Add(reseau)
        for ref, broche in points:
            fp = board.FindFootprintByReference(ref)
            if fp is None:
                introuvables.append("%s (composant absent)" % ref)
                continue
            # Un nom de broche, "IO11" ou "OUTP", se traduit en numeros de
            # pastilles. Un connecteur simple est designe par son numero.
            numeros = tables.get(ref, {}).get(broche, [broche])
            for numero in numeros:
                # Toutes les pastilles portant ce numero, et pas seulement la
                # premiere : la masse sous le module en compte dix-sept, et n'en
                # relier qu'une laisse les seize autres flottantes, ce que KiCad
                # signale ensuite comme autant de courts-circuits.
                trouvees = [pad for pad in fp.Pads() if pad.GetNumber() == numero]
                if not trouvees:
                    introuvables.append("%s broche %s" % (ref, numero))
                    continue
                for pad in trouvees:
                    pad.SetNet(reseau)
                    poses += 1
    return poses, introuvables, len(complet)


def mm(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def trait(board, x1, y1, x2, y2, couche=pcbnew.Edge_Cuts, epaisseur=0.1):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(mm(x1, y1))
    s.SetEnd(mm(x2, y2))
    s.SetLayer(couche)
    s.SetWidth(pcbnew.FromMM(epaisseur))
    board.Add(s)


def arc(board, depart, milieu, fin):
    """Par trois points : un arc defini par un angle part une fois sur deux du
    mauvais cote, et le contour se recroise sans qu'on le voie."""
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_ARC)
    s.SetArcGeometry(mm(*depart), mm(*milieu), mm(*fin))
    s.SetLayer(pcbnew.Edge_Cuts)
    s.SetWidth(pcbnew.FromMM(0.1))
    board.Add(s)


def contour(board):
    """Rectangle a coins arrondis : un coin vif dans un jouet, c'est une echarde."""
    import math
    r, L, H = RAYON_COIN, LARGEUR, HAUTEUR
    d = r * (1 - math.sqrt(2) / 2)          # recul du point milieu sur la diagonale
    trait(board, r, 0, L - r, 0)
    trait(board, L, r, L, H - r)
    trait(board, L - r, H, r, H)
    trait(board, 0, H - r, 0, r)
    arc(board, (r, 0), (d, d), (0, r))                       # coin haut gauche
    arc(board, (L, r), (L - d, d), (L - r, 0))               # coin haut droit
    arc(board, (L - r, H), (L - d, H - d), (L, H - r))       # coin bas droit
    arc(board, (0, H - r), (d, H - d), (r, H))               # coin bas gauche


def texte(board, x, y, contenu, taille=1.2):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(contenu)
    t.SetPosition(mm(x, y))
    t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(taille), pcbnew.FromMM(taille)))
    t.SetTextThickness(pcbnew.FromMM(taille / 6))
    board.Add(t)


def poser(board, ref, lib, nom, x, y, rot, role):
    chemin = os.path.join(LIBS, lib + ".pretty")
    fp = pcbnew.FootprintLoad(chemin, nom)
    if fp is None:
        print("  EMPREINTE INTROUVABLE :", lib, nom)
        return False
    fp.SetPosition(mm(x, y))
    fp.SetOrientationDegrees(rot)
    fp.SetReference(ref)
    fp.SetValue(role)
    # Les vias thermiques de certaines empreintes descendent a 0,2 mm, ce que le
    # fabricant facture en supplement. On les remonte a 0,3, sa valeur courante.
    for pad in fp.Pads():
        d = pad.GetDrillSizeX()
        if 0 < d < pcbnew.FromMM(0.3):
            pad.SetDrillSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.3), pcbnew.FromMM(0.3)))
    board.Add(fp)
    return True


def polygone(points):
    v = pcbnew.VECTOR_VECTOR2I()
    for x, y in points:
        v.append(mm(x, y))
    return v


def plan(board, nom_reseau, couche, contour_points, priorite=0):
    """Une couche entiere, ou une portion, reliee a un seul reseau.

    Le cuivre inutilise ne se jette pas : on le relie a la masse ou a une
    alimentation. Le courant qui part par une piste revient alors par le chemin
    le plus court, juste en dessous, au lieu de faire un detour qui parasite ses
    voisins.
    """
    reseau = board.FindNet(nom_reseau)
    if reseau is None:
        return None
    z = pcbnew.ZONE(board)
    z.SetLayer(couche)
    z.SetNetCode(reseau.GetNetCode())
    z.SetAssignedPriority(priorite)
    # Sans distance de garde explicite, le cuivre vient lecher les trous de
    # fixation et les pattes du connecteur USB.
    z.SetLocalClearance(pcbnew.FromMM(0.35))
    z.SetMinThickness(pcbnew.FromMM(0.25))
    z.AddPolygon(polygone(contour_points))
    z.SetIsFilled(False)
    board.Add(z)
    return z


def piste(board, depart, arrivee, reseau, couche=None, largeur=0.4):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(depart)
    t.SetEnd(arrivee)
    t.SetWidth(pcbnew.FromMM(largeur))
    t.SetLayer(pcbnew.F_Cu if couche is None else couche)
    t.SetNetCode(reseau.GetNetCode())
    board.Add(t)
    return t


def _obstacles(board):
    """Tout ce qu'une via doit eviter : les trous existants et les pastilles."""
    points = []
    for fp in board.Footprints():
        for pad in fp.Pads():
            r = max(pad.GetSize().x, pad.GetSize().y) / 2
            if pad.GetDrillSizeX() > 0:
                r = max(r, pad.GetDrillSizeX() / 2 + pcbnew.FromMM(0.25))
            points.append((pad.GetPosition(), r))
    return points


def garder_libre(board):
    """Interdit le cuivre autour des trous non metallises.

    Un trou de fixation ou une patte de maintien n'a pas de metal dans sa paroi.
    Si un plan vient jusqu'au bord du trou, le foret coupe dans le cuivre et le
    met a nu sur la tranche. On dessine donc une zone d'exclusion autour de
    chacun.
    """
    import math
    posees = 0
    for fp in board.Footprints():
        for pad in fp.Pads():
            if pad.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                continue
            pos = pad.GetPosition()
            rayon = pad.GetDrillSizeX() / 2 + pcbnew.FromMM(0.4)
            points = pcbnew.VECTOR_VECTOR2I()
            for i in range(8):
                a = math.radians(22.5 + i * 45)
                points.append(pcbnew.VECTOR2I(int(pos.x + rayon * math.cos(a)),
                                              int(pos.y + rayon * math.sin(a))))
            z = pcbnew.ZONE(board)
            z.SetLayerSet(pcbnew.LSET.AllCuMask())
            z.SetIsRuleArea(True)
            # Interdire le remplissage, et rien d'autre : sinon la zone bannit
            # aussi la pastille du trou qu'elle entoure.
            z.SetDoNotAllowZoneFills(True)
            for interdit in ("SetDoNotAllowTracks", "SetDoNotAllowVias",
                             "SetDoNotAllowPads", "SetDoNotAllowFootprints"):
                if hasattr(z, interdit):
                    getattr(z, interdit)(False)
            z.AddPolygon(points)
            board.Add(z)
            posees += 1
    return posees


def coudre(board):
    """Une via a cote de chaque pastille de masse, pour descendre au plan.

    Sans elles, les pastilles de masse des composants montes en surface
    resteraient seules sur leur couche, sans rien atteindre. On essaie huit
    directions autour de la pastille et on garde la premiere qui ne frole ni un
    autre trou ni le bord.
    """
    import math
    masse = board.FindNet("GND")
    if masse is None:
        return 0
    obstacles = _obstacles(board)
    rayon_via = pcbnew.FromMM(0.3)
    garde = pcbnew.FromMM(0.3)
    marge_bord = pcbnew.FromMM(1.2)
    posees = 0
    for fp in board.Footprints():
        for pad in fp.Pads():
            if pad.GetNetname() != "GND" or pad.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
                continue
            pos = pad.GetPosition()
            # Certains composants declarent une zone interdite, comme le
            # connecteur SD sous l'emplacement de la carte : on ne pose jamais
            # une via dans l'emprise d'un autre composant.
            # L'emprise reelle du composant, son contour de placement, et non
            # sa boite englobante qui couvrirait la moitie de la carte.
            emprises = []
            for f in board.Footprints():
                if f is fp:
                    continue
                c = f.GetCourtyard(pcbnew.F_Cu)
                if c and c.OutlineCount():
                    emprises.append(c)
            place = None
            for distance_mm in (1.0, 1.4, 1.8):
                for angle in range(0, 360, 45):
                    d = pcbnew.FromMM(distance_mm)
                    x = pos.x + int(d * math.cos(math.radians(angle)))
                    y = pos.y + int(d * math.sin(math.radians(angle)))
                    if not (marge_bord < x < pcbnew.FromMM(LARGEUR) - marge_bord
                            and marge_bord < y < pcbnew.FromMM(HAUTEUR) - marge_bord):
                        continue
                    # Rien sous l'antenne, sur aucune couche.
                    if y < pcbnew.FromMM(GARDE_ANTENNE + 0.5):
                        continue
                    libre = True
                    for contour in emprises:
                        if contour.Collide(pcbnew.VECTOR2I(x, y), 0):
                            libre = False
                            break
                    if not libre:
                        continue
                    # La via, mais aussi le trajet qui y mene : on echantillonne
                    # le segment, sinon la piste passe au ras d'une pastille
                    # voisine sans que la via, elle, ne gene personne.
                    etapes = [(pos.x + (x - pos.x) * k / 6, pos.y + (y - pos.y) * k / 6)
                              for k in range(1, 7)]
                    for autre, r in obstacles:
                        if autre == pos:
                            continue
                        for ex, ey in etapes:
                            if math.hypot(ex - autre.x, ey - autre.y) < r + rayon_via + garde:
                                libre = False
                                break
                        if not libre:
                            break
                    if libre:
                        place = (x, y)
                        break
                if place:
                    break
            if not place:
                continue
            v = pcbnew.PCB_VIA(board)
            v.SetPosition(pcbnew.VECTOR2I(*place))
            v.SetDrill(pcbnew.FromMM(0.3))
            v.SetWidth(pcbnew.FromMM(0.6))
            v.SetNetCode(masse.GetNetCode())
            v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
            board.Add(v)
            # Le bout de piste qui relie la pastille a sa via : sans lui, la via
            # est a cote sans rien toucher.
            piste(board, pos, v.GetPosition(), masse)
            obstacles.append((v.GetPosition(), rayon_via + pcbnew.FromMM(0.15)))
            posees += 1
    return posees


# Le routage des signaux se fait dans l'editeur, a la main. Une tentative de
# routage par script a ete ecrite puis retiree : elle ne passait qu'une liaison
# sur trente-cinq. Une piste doit d'abord s'echapper de sa pastille par un
# degagement calcule, puis contourner les obstacles ; tout trajet droit ou en
# equerre frole la pastille voisine du meme composant. C'est un algorithme a
# part entiere, et le routeur interactif de KiCad le fait bien.

def main():
    sortie = os.path.join(os.path.dirname(os.path.abspath(__file__)), "marelleo.kicad_pcb")
    board = pcbnew.NewBoard(sortie)
    board.SetCopperLayerCount(4)

    # Regles alignees sur ce que le fabricant sait faire sans supplement.
    reglages = board.GetDesignSettings()
    for attribut, valeur in (("m_CopperEdgeClearance", 0.3), ("m_TrackMinWidth", 0.127),
                             ("m_MinThroughDrill", 0.3), ("m_ViasMinSize", 0.45)):
        if hasattr(reglages, attribut):
            setattr(reglages, attribut, pcbnew.FromMM(valeur))

    contour(board)

    manquantes = 0
    for ref, lib, nom, x, y, rot, role in PIECES:
        if not poser(board, ref, lib, nom, x, y, rot, role):
            manquantes += 1

    for ref, lib, nom, valeur, x, y, rot in PASSIFS:
        if not poser(board, ref, lib, nom, x, y, rot, valeur):
            manquantes += 1

    for i, (x, y) in enumerate(FIXATIONS, 1):
        poser(board, "H%d" % i, "MountingHole", "MountingHole_2.7mm_M2.5", x, y, 0, "M2.5")

    # La zone interdite sous l'antenne, tracee pour qu'on ne l'oublie pas en routant.
    trait(board, 0, GARDE_ANTENNE, LARGEUR, GARDE_ANTENNE, pcbnew.Cmts_User, 0.15)
    texte(board, 26.0, 30.0, "ANTENNE : aucun cuivre au-dessus de ce trait", 1.0)
    texte(board, 52.0, 57.5, "Marelleo", 2.0)

    poses, introuvables, reseaux = relier(board)

    # Les plans. La masse occupe une couche interne entiere et le dessous ;
    # les alimentations se partagent l'autre couche interne. Aucun cuivre ne
    # monte au-dessus du trait de l'antenne.
    # Le contour des plans suit les coins arrondis de la carte : un rectangle
    # deborderait dans les arrondis, et son cuivre affleurerait la tranche.
    marge, bas, c = 1.0, GARDE_ANTENNE + 0.5, RAYON_COIN + 1.0
    L, H = LARGEUR - marge, HAUTEUR - marge
    contour_plein = [(marge + c, bas), (L - c, bas), (L, bas + c),
                     (L, H - c), (L - c, H), (marge + c, H), (marge, H - c), (marge, bas + c)]
    plan(board, "GND", pcbnew.In1_Cu, contour_plein)
    plan(board, "GND", pcbnew.B_Cu, contour_plein)
    # Couche 3 : le 3,3 V sur la moitie haute, ou vivent le module et la carte
    # SD, le 5 V sur la moitie basse, ou vivent l'ampli et les convertisseurs.
    # Le 3,3 V prend toute la couche, le 5 V garde un ilot autour de
    # l'amplificateur. Une frontiere horizontale avait d'abord ete tracee au
    # jugé : elle laissait plusieurs pastilles au-dessus du mauvais plan, une
    # via ne pouvant atteindre que le cuivre qui se trouve juste sous elle.
    plan(board, "+3V3", pcbnew.In2_Cu,
         [(marge + c, bas), (L - c, bas), (L, bas + c), (L, H - c),
          (L - c, H), (marge + c, H), (marge, H - c), (marge, bas + c)], 1)
    plan(board, "+5V", pcbnew.In2_Cu, ILOT_5V, 2)

    exclusions = garder_libre(board)
    vias = coudre(board)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())

    pcbnew.SaveBoard(sortie, board)
    print("carte ecrite :", sortie)
    print("empreintes manquantes :", manquantes)
    print("liaisons declarees : %d reseaux, %d pastilles reliees" % (reseaux, poses))
    for x in introuvables:
        print("  PASTILLE INTROUVABLE :", x)
    print("plans : %d, exclusions autour des trous : %d, vias de masse : %d"
          % (len(board.Zones()) - exclusions, exclusions, vias))
    print("reste a declarer :", ", ".join(A_DECLARER))
    return 1 if (manquantes or introuvables) else 0


if __name__ == "__main__":
    sys.exit(main())
