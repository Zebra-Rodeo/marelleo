// Le WiFi, et le portail de configuration quand la boite ne connait aucun reseau.
//
// Les identifiants vivent dans la memoire que le pilote WiFi reserve a cet
// usage (NVS). Elle survit aux mises a jour du micrologiciel et a uploadfs,
// contrairement a l'ancien wifi.json qu'uploadfs effacait.
//
// Sans identifiants, la boite **ouvre son propre reseau WiFi**, Marelleo-XXXX,
// et sert une page de reglage. Un telephone qui s'y connecte l'ouvre tout seul,
// comme dans un hotel : c'est le role du serveur DNS, qui repond l'adresse de
// la boite a n'importe quelle question. Aucune application a installer, et ca
// marche depuis un iPhone, un Android ou un ordinateur.
//
// Le Bluetooth a fait ce travail jusqu'en octobre 2026. Il demandait une app
// dediee, ce qui est intenable pour un objet qu'on vend en kit, et sa pile
// pesait 300 Ko de micrologiciel, soit neuf points de la memoire disponible.
//
// Bouton maintenu trois secondes a l'allumage : les identifiants sont effaces
// et la boite rouvre son portail. C'est la porte de sortie d'une boite qui a
// change de maison.
//
// Et si le reseau enregistre a disparu, routeur remplace, demenagement, mot de
// passe change, la boite **rouvre son portail au bout d'une minute sans cesser
// de chercher**. C'est ce qui rend un boitier scelle acceptable : sans acces a
// l'USB, une boite qui reessaie indefiniment un reseau qui n'existe plus joue
// encore ses histoires mais ne se reconfigure ni ne se met plus a jour. Si le
// routeur revient, elle se raccroche et referme le portail toute seule.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class Reseau {
public:
  void begin(bool effacerIdentifiants);
  void loop();

  String description() const;
  bool   connecte() const;
  String ssid() const;
  int    rssi() const;
  String ip() const;
  String hote() const;        // marelleo.local, l'adresse a taper

  // Vrai tant que la boite attend qu'on lui donne un reseau. Le serveur s'en
  // sert pour repondre la page de reglage a n'importe quelle adresse demandee,
  // sans quoi le telephone ne saurait pas qu'il y a quelque chose a ouvrir.
  bool   enPortail() const;
  String nomDuPortail() const;    // le reseau qu'ouvre la boite

  // Scrute les reseaux a portee et les range du plus fort au plus faible.
  // Bloque deux secondes environ : c'est une page de reglage, pas une boucle
  // de lecture, et on ne la demande qu'au moment ou on en a besoin.
  void   reseauxAPortee(JsonDocument& d);

  // Change de reseau sans risque de perdre la boite : les nouveaux identifiants
  // sont d'abord essayes sans etre enregistres, et ne remplacent les anciens
  // qu'une fois la connexion obtenue. En cas d'echec, retour a l'ancien reseau,
  // ou au portail s'il n'y en avait pas.
  void changer(const String& ssid, const String& motDePasse);
  const char* modification() const;   // aucune, en cours, reussie, echouee

  // Outil de developpement : enregistre directement, sans essai prealable.
  void forcer(const String& ssid, const String& motDePasse);
};
