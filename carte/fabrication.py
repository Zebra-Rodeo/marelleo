#!/usr/bin/env python3
"""Sort les deux fichiers que reclame JLCPCB pour monter les composants :

  pcb/fab/marelleo-bom.csv   ce qu'il faut acheter, et pour quels reperes
  pcb/fab/marelleo-cpl.csv   ou poser chaque piece, et dans quel sens

La liste des pieces ne vient pas d'ici : elle vit dans pcb/composants.csv, un
tableau qui se relit et se corrige a la main, une ligne par repere. Ce script
ne fait que le confronter a la carte et mettre les colonnes au format du
fabricant.

S'utilise avec le python du systeme, il n'a pas besoin de pcbnew :

    python3 pcb/fabrication.py
"""

import csv
import re
import subprocess
import sys
from pathlib import Path

ICI = Path(__file__).resolve().parent
CARTE = ICI / "marelleo.kicad_pcb"
CATALOGUE = ICI / "composants.csv"
SORTIE = ICI / "fab"
CLI = Path("/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli")

# Tarif d'assemblage releve chez JLCPCB, formule standard, une seule face.
PREPARATION = 25.56
POCHOIR = 8.21
PAR_TYPE = 1.53
PAR_SOUDURE = 0.0016


# --- lecture de la carte --------------------------------------------------

def empreintes():
    """Rend, pour chaque repere : empreinte, valeur, nombre de soudures, et si
    la piece est traversante ou non montee. Lu dans le fichier de la carte, qui
    reste la seule verite une fois le routage fait a la main."""
    texte = CARTE.read_text()
    trouvees = {}
    for bloc in texte.split("\n\t(footprint ")[1:]:
        nom = re.match(r'"([^"]+)"', bloc).group(1)
        ref = re.search(r'\(property "Reference" "([^"]*)"', bloc).group(1)
        val = re.search(r'\(property "Value" "([^"]*)"', bloc).group(1)
        attr = re.search(r"\n\t\t\(attr ([^)]*)\)", bloc)
        attr = attr.group(1) if attr else ""

        # Une soudure par broche, et non par pastille : l'empreinte du module
        # decoupe sa semelle en une dizaine de pastilles qui portent toutes le
        # meme numero, et y ajoute des vias thermiques traversants. Compter les
        # pastilles donnerait 62 soudures pour 41 broches, et ferait passer le
        # module pour une piece traversante.
        broches = {}
        for pad in re.findall(r"\n\t\t\(pad \"([^\"]*)\" (\w+) ", bloc):
            nom_broche, type_pad = pad
            if type_pad == "np_thru_hole" or not nom_broche:
                continue
            broches.setdefault(nom_broche, set()).add(type_pad)

        soudures = len(broches)
        traversant = any(types == {"thru_hole"} for types in broches.values())

        trouvees[ref] = {
            "empreinte": nom.split(":")[-1],
            "valeur": val,
            "soudures": soudures,
            "traversant": traversant,
            # KiCad efface le drapeau "do not populate" a chaque sauvegarde :
            # la valeur "NP" est ce qui reste de lisible sur la carte, et la
            # colonne "pose" du catalogue est ce qui fait foi ensuite.
            "non_monte": val.strip().upper() == "NP",
            "mecanique": "exclude_from_bom" in attr,
        }
    return trouvees


def positions():
    """Demande le fichier de placement a KiCad plutot que de le recalculer :
    lui seul connait la convention d'origine utilisee par les Gerbers."""
    if not CLI.exists():
        sys.exit("kicad-cli introuvable, KiCad n'est pas installe la ou je crois")
    sortie = SORTIE / "positions-kicad.csv"
    subprocess.run(
        [str(CLI), "pcb", "export", "pos", "--format", "csv", "--units", "mm",
         "--side", "both", "-o", str(sortie), str(CARTE)],
        check=True, capture_output=True)
    with sortie.open(encoding="utf-8") as f:
        return {l["Ref"]: l for l in csv.DictReader(f)}


# --- le catalogue, tenu a la main ----------------------------------------

COLONNES = ["repere", "designation", "empreinte", "lcsc", "famille", "pose", "note"]


def ebaucher(pieces):
    """Ecrit un catalogue de depart a partir de la carte. Ne s'execute qu'une
    fois : ensuite le fichier est a nous, le script ne le reecrit jamais."""
    lignes = []
    for ref, p in sorted(pieces.items(), key=cle_repere):
        if p["mecanique"]:
            continue
        pose = "non monte" if p["non_monte"] else ("traversant" if p["traversant"] else "cms")
        # La valeur porte parfois une description apres une virgule, et une
        # virgule decimale a la francaise : ni l'une ni l'autre n'ont leur place
        # dans un fichier lu a l'autre bout du monde.
        designation = p["valeur"].split(",")[0] if " " in p["valeur"].split(",")[0] else p["valeur"]
        designation = designation.replace(",", ".")
        lignes.append({"repere": ref, "designation": designation,
                       "empreinte": p["empreinte"], "lcsc": "",
                       "famille": "", "pose": pose, "note": ""})
    with CATALOGUE.open("w", newline="", encoding="utf-8") as f:
        e = csv.DictWriter(f, COLONNES)
        e.writeheader()
        e.writerows(lignes)
    print("catalogue de depart ecrit dans", CATALOGUE.name)
    print("a completer : la colonne lcsc, une reference par ligne")


def cle_repere(couple):
    """Trie R1, R2, R10 dans cet ordre, et non R1, R10, R2."""
    ref = couple[0] if isinstance(couple, tuple) else couple
    m = re.match(r"([A-Za-z]+)(\d+)", ref)
    return (m.group(1), int(m.group(2))) if m else (ref, 0)


def lire_catalogue():
    with CATALOGUE.open(encoding="utf-8") as f:
        return {l["repere"]: l for l in csv.DictReader(f)}


# --- les deux fichiers du fabricant --------------------------------------

def ecrire(pieces, catalogue, pos):
    SORTIE.mkdir(exist_ok=True)
    montees = [r for r, p in pieces.items()
               if not p["mecanique"] and catalogue[r]["pose"] != "non monte"]

    # Nomenclature : une ligne par piece identique, reperes regroupes.
    groupes = {}
    for ref in sorted(montees, key=cle_repere):
        c = catalogue.get(ref)
        if c is None:
            sys.exit("absent du catalogue : " + ref)
        cle = (c["designation"], c["empreinte"], c["lcsc"])
        groupes.setdefault(cle, []).append(ref)

    bom = SORTIE / "marelleo-bom.csv"
    with bom.open("w", newline="", encoding="utf-8") as f:
        e = csv.writer(f)
        e.writerow(["Comment", "Designator", "Footprint", "LCSC Part #"])
        for (designation, empreinte, lcsc), refs in sorted(
                groupes.items(), key=lambda kv: cle_repere(kv[1][0])):
            e.writerow([designation, ",".join(refs), empreinte, lcsc])

    # Placement : une ligne par piece, aux colonnes attendues par JLCPCB.
    cpl = SORTIE / "marelleo-cpl.csv"
    with cpl.open("w", newline="", encoding="utf-8") as f:
        e = csv.writer(f)
        e.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
        for ref in sorted(montees, key=cle_repere):
            p = pos.get(ref)
            if p is None:
                sys.exit("absent du fichier de placement : " + ref)
            e.writerow([ref, p["PosX"], p["PosY"],
                        "Top" if p["Side"] == "top" else "Bottom", p["Rot"]])

    return bom, cpl, groupes, montees


def rapport(pieces, catalogue, groupes, montees, quantite=50):
    soudures = sum(pieces[r]["soudures"] for r in montees)
    types = len(groupes)
    traversants = [r for r in montees if pieces[r]["traversant"]]

    print()
    print("  pieces a poser        %3d, en %d types" % (len(montees), types))
    print("  soudures par carte    %3d" % soudures)
    print("  dont traversantes     %3d pattes, sur %s"
          % (sum(pieces[r]["soudures"] for r in traversants),
             " ".join(sorted(traversants, key=cle_repere)) or "rien"))

    montage = PREPARATION + POCHOIR + PAR_TYPE * types + PAR_SOUDURE * soudures * quantite
    print()
    print("  montage de %d cartes  %6.2f $, soit %.2f $ la carte"
          % (quantite, montage, montage / quantite))
    print("     preparation        %6.2f $" % PREPARATION)
    print("     pochoir            %6.2f $" % POCHOIR)
    print("     %2d types           %6.2f $" % (types, PAR_TYPE * types))
    print("     %d soudures       %6.2f $" % (soudures * quantite,
                                              PAR_SOUDURE * soudures * quantite))
    print("  le supplement pour les pattes traversantes n'est pas dans ce calcul")

    familles = {}
    for (d, emp, lcsc), refs in groupes.items():
        familles[lcsc] = catalogue[refs[0]].get("famille", "")
    print("  dont pieces de base    %3d, et etendues %d"
          % (sum(1 for f in familles.values() if f == "base"),
             sum(1 for f in familles.values() if f == "etendue")))
    print("  le supplement par piece etendue n'est pas dans ce calcul non plus")

    sans = [r for r in montees if not catalogue[r]["lcsc"].strip()]
    print()
    if sans:
        print("  SANS REFERENCE LCSC, donc pas commandable :")
        for (d, emp, lcsc), refs in sorted(groupes.items(),
                                           key=lambda kv: cle_repere(kv[1][0])):
            if not lcsc.strip():
                print("     %-22s %-42s %s" % (d, emp, " ".join(refs)))
    else:
        print("  toutes les pieces ont une reference LCSC")


def main():
    pieces = empreintes()
    SORTIE.mkdir(exist_ok=True)
    if not CATALOGUE.exists():
        ebaucher(pieces)
        return
    catalogue = lire_catalogue()
    pos = positions()
    bom, cpl, groupes, montees = ecrire(pieces, catalogue, pos)
    print("ecrit", bom.relative_to(ICI.parent), "et", cpl.relative_to(ICI.parent))
    rapport(pieces, catalogue, groupes, montees)


if __name__ == "__main__":
    main()
