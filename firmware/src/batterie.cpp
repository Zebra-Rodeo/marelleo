#include "batterie.h"
#include <Preferences.h>

namespace {
  constexpr uint32_t PERIODE_MS   = 2000;   // la charge ne bouge pas vite
  constexpr uint8_t  ECHANTILLONS = 16;

  // Une cellule lithium ne se vide pas lineairement : elle tient longtemps vers
  // 3,7 V puis s'effondre. Interpoler entre ces points donne une jauge honnete,
  // la ou une regle de trois annoncerait 50 % sur une batterie presque vide.
  struct Point { float volts; uint8_t pourcent; };
  // Les tensions sont celles de VSYS, pas celles de la cellule. Entre les deux
  // se trouve le transistor interne du chargeur, dont la resistance passante
  // fait chuter la mesure sous le courant que tire la boite. Calibre sur la
  // carte, batterie pleine : 4,20 V aux bornes de J6, 4,00 V vus par l'ADC.
  // La courbe d'une cellule lithium est donc decalee de 0,12 V vers le bas.
  //
  // Une seule mesure, et la chute depend du courant, donc l'echelle reste
  // approchee vers le bas, ou la boite consomme davantage a tension plus
  // basse. La vraie correction est materielle : mesurer VBAT plutot que VSYS,
  // c'est note dans pcb/schema.md.
  const Point COURBE[] = {
    {4.03f, 100}, {3.93f, 90}, {3.83f, 75}, {3.73f, 60}, {3.66f, 50},
    {3.58f, 38},  {3.50f, 25}, {3.40f, 15}, {3.28f,  7}, {3.08f,  2}, {2.88f, 0},
  };

  uint8_t versPourcent(float v) {
    if (v >= COURBE[0].volts) return 100;
    for (size_t i = 1; i < sizeof COURBE / sizeof *COURBE; i++) {
      const Point& haut = COURBE[i - 1];
      const Point& bas  = COURBE[i];
      if (v >= bas.volts) {
        float part = (v - bas.volts) / (haut.volts - bas.volts);
        return (uint8_t)(bas.pourcent + part * (haut.pourcent - bas.pourcent) + 0.5f);
      }
    }
    return 0;
  }
}

void Batterie::begin(uint8_t pinMesure, uint8_t pinUsb) {
  _pin = pinMesure;
  _pinUsb = pinUsb;
  if (_pin == 255) return;
  analogRead(_pin);                       // attache la broche au peripherique ADC
  analogSetPinAttenuation(_pin, ADC_11db);
  // PGOOD du chargeur est une sortie a collecteur ouvert, active a l'etat bas,
  // et la carte ne porte aucun rappel : il faut celui du processeur.
  if (_pinUsb != 255) pinMode(_pinUsb, INPUT_PULLUP);
  Preferences nvs;
  if (nvs.begin("batterie", true)) {
    _dernier = _grave = nvs.getUChar("dernier", 255);
    nvs.end();
  }
  _derniere = millis() - PERIODE_MS;
  lire();
}

void Batterie::lire() {
  if (_pin == 255 || millis() - _derniere < PERIODE_MS) return;
  _derniere = millis();

  uint32_t somme = 0;
  for (uint8_t i = 0; i < ECHANTILLONS; i++) somme += analogReadMilliVolts(_pin);
  // Le pont divise par deux, et analogReadMilliVolts applique l'etalonnage grave
  // en usine dans les fusibles : sans lui, l'ecart d'une puce a l'autre depasse
  // largement la precision qu'on cherche ici.
  float mesure = 2.0f * somme / ECHANTILLONS / 1000.0f;

  bool avant = _secteur;
  if (_pinUsb != 255) _secteur = digitalRead(_pinUsb) == LOW;

  // Au debranchement, la tension met un instant a tomber et le filtre la lisse
  // encore : sans cette remise a zero, la boite enregistrait 100 % comme dernier
  // niveau connu, mesure sur une cellule a 3,1 V.
  if (avant && !_secteur) { _volts = 0; _stables = 0; _affiche = 255; }
  if (_secteur) _affiche = 255;   // sur secteur, la jauge n'a rien a retenir

  _volts = _volts > 0 ? _volts + (mesure - _volts) * 0.3f : mesure;
  _pourcent = versPourcent(_volts);
  if (!_secteur && (_affiche == 255 || _pourcent < _affiche)) _affiche = _pourcent;

  // Une cellule au lithium seule ne depasse pas 4,25 V : au-dela, c'est que
  // l'entree est encore presente ou que la mesure n'a pas fini de descendre.
  if (!chargeConnue() || _volts > 4.3f) { _stables = 0; return; }
  if (++_stables < 3) return;          // trois mesures d'accord avant d'y croire
  _dernier = pourcent();
  retenir();
}

// La memoire non volatile s'use a l'ecriture. On n'y touche que par pas de
// cinq points : sur une decharge complete, cela fait une vingtaine d'ecritures,
// la ou enregistrer chaque mesure en ferait des milliers par soiree.
void Batterie::retenir() {
  if (_grave != 255 && abs((int)_dernier - (int)_grave) < 5) return;
  Preferences nvs;
  if (!nvs.begin("batterie", false)) return;
  nvs.putUChar("dernier", _dernier);
  nvs.end();
  _grave = _dernier;
}
