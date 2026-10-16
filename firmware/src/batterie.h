// Le niveau de la batterie, lu sur le pont diviseur de la carte imprimee.
//
// Le prototype n'a pas ce pont : presente() rend alors faux et l'interface
// n'affiche rien, plutot qu'un chiffre invente.
//
// Ce qu'on mesure est VSYS, la sortie du chargeur, et non la cellule
// elle-meme. Les deux se confondent sur batterie ; des que l'USB est branche,
// VSYS monte a la tension d'entree et ne dit plus rien de la charge. D'ou la
// broche PGOOD, qui dit si une source externe est presente.
#pragma once
#include <Arduino.h>

class Batterie {
public:
  // pinUsb a 255 si la carte ne porte pas PGOOD.
  void begin(uint8_t pinMesure, uint8_t pinUsb);
  void lire();                    // moyenne glissante, a appeler dans la boucle

  bool    presente()   const { return _pin != 255; }
  // Sur secteur, VSYS vaut la tension d'entree et ne dit plus rien de la
  // cellule : mesure a 5,01 V sur la carte, USB branche. Annoncer 100 % dans ce
  // cas serait faux sur une batterie a plat.
  bool    chargeConnue() const { return presente() && !_secteur; }

  // Le dernier niveau mesure sur batterie, garde en memoire non volatile.
  // Sur secteur, c'est tout ce qu'on peut honnetement montrer : la cellule
  // n'est reliee a aucune broche de mesure, et VSYS ne parle que de l'entree.
  // 255 tant qu'aucune mesure sur batterie n'a eu lieu.
  uint8_t dernierConnu() const { return _dernier; }
  bool    surSecteur() const { return _secteur; }
  float   volts()      const { return _volts; }
  // Le pourcentage montre. Il ne remonte jamais sur batterie : la tension d'une
  // cellule oscille d'un point au gre de ce que la boite consomme, et une jauge
  // qui monte et descend sous les yeux n'inspire rien de bon. Elle repart de la
  // mesure vraie des que le secteur est la.
  uint8_t pourcent()   const { return _affiche != 255 ? _affiche : _pourcent; }

private:
  uint8_t  _pin = 255, _pinUsb = 255;
  bool     _secteur = false;
  float    _volts = 0;
  uint8_t  _pourcent = 0;
  uint8_t  _affiche = 255;
  uint8_t  _dernier = 255;
  uint8_t  _stables = 0;
  uint8_t  _grave = 255;
  uint32_t _derniere = 0;
  void     retenir();
};
