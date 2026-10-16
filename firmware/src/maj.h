// Mise a jour du micrologiciel par le WiFi.
//
// La boite va chercher elle-meme sa nouvelle version a une adresse qu'on lui
// donne, plutot qu'on la lui envoie. Un bouton dans l'app, et c'est tout : elle
// est montee, vissee, et on ne veut plus jamais l'ouvrir pour un cable.
//
// A cette adresse se trouve un petit manifeste :
//
//   { "version": "0.6.0",
//     "url": "https://.../firmware.bin",
//     "taille": 2604823,
//     "md5": "9f8c...",
//     "notes": "figurine radio" }
//
// L'image se grave dans l'emplacement de programme inactif. Celui qui tourne
// n'est pas touche, donc une coupure de courant ou un WiFi qui tombe en plein
// transfert ne laisse jamais la boite muette : elle redemarre sur l'ancienne.
//
// Reste le cas mechant : une version qui demarre mais dont le reseau est casse.
// Le chargeur d'amorcage la trouve valide et bascule dessus, et il n'y a plus de
// chemin pour la corriger sans ouvrir la boite. D'ou la periode d'essai : une
// version fraichement gravee doit prouver qu'elle sait se connecter. Si elle
// n'y arrive pas en trois demarrages, on rebascule tout seul sur la precedente.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <atomic>

class Reseau;

class Maj {
public:
  // A appeler tot dans setup(), avant le reseau : dit si on tourne sur une
  // version en periode d'essai, et rebascule si elle a deja trop echoue.
  void begin();

  // A appeler quand le reseau est etabli : clot la periode d'essai.
  void valider();

  bool enEssai() const { return _enEssai; }

  // Interroge le manifeste. Rend false et remplit erreur si l'adresse ne
  // repond pas ou ne contient pas ce qu'on attend.
  bool consulter(String& erreur, JsonDocument& reponse);

  // Lance le telechargement et la gravure dans une tache dediee. La boite
  // redemarre d'elle-meme a la fin, si tout s'est bien passe.
  bool installer(String& erreur);

  void ajouterEtat(JsonDocument& d) const;

  String source() const;

  // Vrai tant qu'une gravure est en cours : la lecture doit rester a l'arret.
  bool occupee() const { return _active; }

private:
  static void tache(void* arg);
  void graver();

  std::atomic<bool>     _active{false};
  std::atomic<uint32_t> _recu{0}, _total{0};
  String                _erreur, _urlImage, _md5, _versionCible;
  uint32_t              _tailleAnnoncee = 0;   // celle du manifeste, en secours
  bool                  _enEssai = false;
};

extern Maj maj;
