// La page de configuration, servie par la boite elle-meme.
//
// Visiter la page n'a aucun effet sur la boite : elle continue de jouer, et un
// badge lance toujours son histoire. La page observe l'etat et envoie des
// commandes explicites, rien de plus.
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <atomic>
#include <functional>
#include "library.h"
#include "reseau.h"

// Ce que le serveur peut demander au lecteur. Le serveur ne sait rien de la
// lecture ni du materiel : main.cpp remplit ces fonctions.
struct Lecteur {
  // l'etat a publier dans /api/etat
  std::function<void(JsonDocument&)> etat;

  // fermer un fichier s'il est ouvert : LittleFS refuse d'effacer ou d'ecraser
  // un fichier ouvert, meme en pause
  std::function<void(const String& chemin)> liberer;

  // octets libres sur le stockage des histoires : distingue une carte pleine
  // d'une ecriture refusee
  std::function<uint64_t()> espaceLibre;

  // la liste d'une figurine vient de changer ou de disparaitre
  std::function<void(const String& uid)> modifiee;

  // pause, lecture, suivante, arreter, ou lire une figurine donnee ; faux avec
  // un message si l'action n'a pas de sens dans l'etat courant
  std::function<bool(const String& action, const String& uid, String& erreur)> commander;

  // remonter le stockage apres une ecriture refusee : une seule erreur SPI
  // desynchronise le pilote et la carte, et tout echoue ensuite jusqu'au
  // remontage. Arrete la lecture en cours, qui tient un fichier ouvert.
  std::function<void()> reparer;
};

class Serveur {
public:
  // contenu peut etre nul si aucun stockage n'a monte : les routes qui en
  // dependent repondent alors une erreur au lieu de planter.
  void begin(fs::FS* contenu, Library& biblio, const Lecteur& lecteur, Reseau& reseau);
  void loop();

private:
  void recevoirMorceau();
  void relayer();
  void recuperer();
  void ajouterRecuperation(JsonDocument& d) const;
  static void tacheRecuperation(void* arg);
  void telecharger();
  bool tenterRecuperation(String& erreur, uint32_t depuis);
  void demanderReparation();
  void terminerEnvoi();
  void associer();
  void dissocier();
  void supprimer();
  void commander();
  void reglages();
  void publierInfos() const;
  void publierWifi() const;
  void changerWifi();
  void publierBibliotheque() const;
  void listerHistoires(const char* dossier, JsonArray sortie) const;

  fs::FS*  _contenu = nullptr;
  Library* _biblio  = nullptr;
  Lecteur  _lecteur;
  Reseau*  _reseau  = nullptr;

  // envoi de fichier en cours
  //
  // Les paquets arrivent par 1436 octets, la charge utile d'une trame Ethernet.
  // Ecrire directement chacun d'eux fait travailler la carte par fractions de
  // secteur : elle doit lire, modifier, reecrire. On les regroupe en blocs de
  // 8 Ko, un multiple de la taille de secteur, avant d'ecrire.
  static constexpr size_t TAMPON = 8192;
  File     _fichier;
  uint8_t* _tampon = nullptr;
  size_t   _dansTampon = 0;
  bool     ecrireTampon();

  String _chemin, _erreur;
  size_t _ecrits = 0;
  int    _code = 200;

  // Episode en cours de recuperation. La tache de telechargement ecrit la
  // progression, la boucle principale la lit pour la publier dans /api/etat :
  // d'ou les atomiques. Les chaines sont ecrites avant le demarrage de la
  // tache, sauf _recupErreur, posee juste avant que _recupActive retombe.
  std::atomic<bool>     _recupActive{false};
  // La tache ne remonte pas la carte elle-meme : reparer() arrete la lecture,
  // donc touche a l'objet audio, qui appartient a la boucle principale. Elle
  // pose ce drapeau, que loop() consomme.
  std::atomic<bool>     _reparerDemande{false};
  std::atomic<uint32_t> _recupRecu{0};
  std::atomic<uint32_t> _recupTotal{0};
  String   _recupUrl, _recupNom, _recupChemin, _recupErreur;
  // Mesuree par la boucle principale avant de lancer la tache : interroger le
  // systeme de fichiers depuis la tache rend zero.
  uint64_t _recupLibre = 0;
};
