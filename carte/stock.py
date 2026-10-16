#!/usr/bin/env python3
"""Demande au fabricant l'etat de chaque reference du catalogue : stock, prix,
famille, et le nom exact de la piece. Rien n'est plus penible qu'un ordre de
fabrication refuse parce qu'une piece a disparu entre le choix et la commande.

    python3 pcb/stock.py            verifie tout pcb/composants.csv
    python3 pcb/stock.py 470uF 10V  cherche une piece par mots-cles
"""

import csv
import json
import sys
import urllib.request
from pathlib import Path

ICI = Path(__file__).resolve().parent
CATALOGUE = ICI / "composants.csv"
URL = ("https://jlcpcb.com/api/overseas-pcb-order/v1/shoppingCart/smtGood/"
       "selectSmtComponentList")


def interroger(mot, taille=20):
    corps = json.dumps({"currentPage": 1, "pageSize": taille, "keyword": mot}).encode()
    req = urllib.request.Request(URL, data=corps, headers={
        "Content-Type": "application/json", "User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=30) as r:
        rep = json.load(r)
    return rep.get("data", {}).get("componentPageInfo", {}).get("list", []) or []


def prix(piece, quantite=50):
    """Le prix unitaire a la quantite qui nous interesse."""
    for p in piece.get("componentPrices") or []:
        fin = p["endNumber"]
        if p["startNumber"] <= quantite and (fin == -1 or quantite <= fin):
            return p["productPrice"]
    return None


def decrire(p):
    return " ".join(str(x) for x in [
        p.get("componentModelEn") or p.get("erpComponentName") or "",
        p.get("componentSpecificationEn") or ""]).strip()


def verifier():
    lignes = list(csv.DictReader(CATALOGUE.open(encoding="utf-8")))
    par_code = {}
    for l in lignes:
        if l["lcsc"] and l["pose"] != "non monte":
            par_code.setdefault(l["lcsc"], []).append(l["repere"])

    total, ennuis = 0.0, []
    print("%-14s %-9s %8s %8s  %s" % ("reference", "famille", "stock", "prix", "reperes"))
    for code, reperes in par_code.items():
        trouves = [p for p in interroger(code) if p.get("componentCode") == code]
        if not trouves:
            trouves = interroger(code)[:1]
        if not trouves:
            print("%-14s %-9s %8s %8s  %s" % (code, "INTROUVABLE", "-", "-", " ".join(reperes)))
            ennuis.append((code, reperes, "introuvable"))
            continue
        p = trouves[0]
        stock = p.get("stockCount", 0)
        u = prix(p)
        besoin = len(reperes) * 50
        famille = "base" if p.get("componentLibraryType") == "base" else "etendue"
        alerte = ""
        if stock < besoin:
            alerte = "  <-- manque, il en faut %d" % besoin
            ennuis.append((code, reperes, "stock %d pour %d" % (stock, besoin)))
        if u:
            total += u * besoin
        print("%-14s %-9s %8d %8s  %s%s" % (
            code, famille, stock, ("%.4f" % u) if u else "?", " ".join(reperes), alerte))

    print()
    print("  pieces pour 50 cartes : %.2f $, soit %.2f $ la carte" % (total, total / 50))
    if ennuis:
        print("  %d reference(s) a remplacer" % len(ennuis))
    return ennuis


def chercher(mots):
    for p in interroger(" ".join(mots), 30):
        stock = p.get("stockCount", 0)
        if stock <= 0:
            continue
        print("%-14s %-7s %8d  %8s  %s | %s" % (
            p.get("componentCode"), 
            "base" if p.get("componentLibraryType") == "base" else "etendue",
            stock, ("%.4f" % prix(p)) if prix(p) else "?",
            p.get("componentSpecificationEn") or "",
            decrire(p)[:70]))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        chercher(sys.argv[1:])
    else:
        verifier()
