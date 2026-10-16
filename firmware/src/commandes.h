// Potentiometre de volume et bouton poussoir.
//
// Le potentiometre est un modele a interrupteur recupere sur une Lunii. Son
// clic de butee basse ne va pas au microcontroleur : il commande la broche EN
// du Lipo Rider, donc l'alimentation de toute la boite. Le logiciel ne le voit
// jamais et n'a pas a s'en occuper, il ne lit que la piste resistive.
//
// C'est un capteur ABSOLU, contrairement a l'encodeur qu'il remplace. La
// position du bouton est le volume : il n'y a plus rien a memoriser entre deux
// allumages, et la boite demarre toujours au minimum puisque le clic se trouve
// en bas de course. Le fichier /volume a disparu avec l'encodeur.
#pragma once
#include <Arduino.h>

enum class Evenement { Rien, Volume, LecturePause, Suivant };

// Anti-rebond par integration : un changement d'etat n'est accepte que si le
// niveau brut est reste STABLE pendant toute la duree. Cela couvre le
// relachement autant que l'appui, ce qu'un simple delai apres front ne fait
// pas : ses rebonds de relachement passent pour de nouveaux appuis.
class Bouton {
public:
  enum class Appui { Aucun, Court, Long };
  void  begin(uint8_t pin);
  Appui lire();

private:
  uint8_t  _pin = 0;
  bool     _brut = true, _stable = true;
  uint32_t _depuis = 0, _appuyeDepuis = 0;
  bool     _longEnvoye = false;
};

class Potentiometre {
public:
  void     begin(uint8_t pin, uint8_t mini, uint8_t maxi);
  bool     lire();                          // true si le niveau vient de changer
  uint8_t  niveau() const { return _niveau; }
  uint16_t brut()   const { return _brut; }  // valeur ADC moyennee, pour le calibrage


private:
  uint8_t  versNiveau(uint16_t brut) const;

  uint8_t  _pin = 0, _mini = 0, _maxi = 0;
  uint8_t  _niveau = 0, _candidat = 0;
  uint16_t _brut = 0;
  uint32_t _depuis = 0, _dernierEch = 0;
};

class Commandes {
public:
  void      begin(uint8_t pinPot, uint8_t pinBouton, uint8_t volMin, uint8_t volMax);
  Evenement lire();                          // a appeler souvent, ne bloque jamais
  uint8_t   volume()  const { return _pot.niveau(); }
  uint16_t  potBrut() const { return _pot.brut(); }

private:
  Bouton        _bouton;
  Potentiometre _pot;
};
