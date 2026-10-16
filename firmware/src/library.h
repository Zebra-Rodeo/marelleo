// La bibliotheque : quelle figurine joue quoi.
// Elle vit dans un fichier, jamais dans le code. La page de configuration ne
// fait que reecrire ce fichier ; la logique de lecture n'en sait rien.
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <functional>
#include <map>
#include <vector>

struct Figurine {
  String              nom;
  std::vector<String> pistes;
  // Lecture dans le desordre. L'ordre reste celui du fichier : c'est la lecture
  // qui tire une permutation, pour que la liste affichee ne bouge jamais sous
  // les yeux de celui qui la modifie.
  bool                aleatoire = false;
};

class Library {
public:
  bool charger(fs::FS& fs, const char* chemin = "/library.json");

  // Ecriture resistante aux coupures : un enfant peut eteindre la boite au
  // bouton en pleine sauvegarde. Voir la reprise dans charger().
  bool sauver(fs::FS& fs, const char* chemin = "/library.json");

  // nullptr si la figurine n'est ni connue ni couverte par le defaut
  const Figurine* pour(const String& uid) const;

  // la figurine explicitement associee, sans repli sur le defaut
  const Figurine* explicite(const String& uid) const;

  // Modifications en memoire seulement : l'appelant decide quand sauver().
  // Les pistes forment une liste ordonnee, jouee d'un bout a l'autre.
  void associer(const String& uid, const String& nom, const std::vector<String>& pistes,
                bool aleatoire = false);
  bool dissocier(const String& uid);
  void pourChaque(const std::function<void(const String&, const Figurine&)>& f) const;

  // Faux si library.json existait mais n'a pas pu etre lu. Dans ce cas sauver()
  // refuse d'ecrire : la bibliotheque en memoire est vide, et l'enregistrer
  // effacerait toutes les associations du fichier, qu'on peut encore reparer.
  bool modifiable() const { return _modifiable; }

  uint8_t volume()  const { return _volume; }

  // Faut-il que la figurine reste posee pour que la lecture continue ? Non par
  // defaut : la boite se comporte en pointeuse, on badge et on repart jouer.
  // A vrai, retirer la figurine met en pause, et la reposer reprend.
  bool poseRequise() const { return _poseRequise; }
  void poseRequise(bool v) { _poseRequise = v; }
  size_t  nombre()  const { return _figurines.size(); }
  const String& erreur() const { return _erreur; }

private:
  std::map<String, Figurine> _figurines;
  Figurine _defaut;
  bool     _aDefaut = false;
  bool     _modifiable = false;
  uint8_t  _volume  = 13;
  bool     _poseRequise = false;
  String   _erreur;
};
